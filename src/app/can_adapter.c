#include "can_adapter.h"
#include "board_config.h"
#include "relay_driver.h"
#include "led_driver.h"
#include "flash_driver.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <stdbool.h>

/* --- Внутреннее состояние --- */
static uint8_t can_node_id = 1;
static uint8_t relay_states = 0;  /* Битовая маска 8 реле */

/* ====== Вспомогательные ====== */

/** Отправить CAN-фрейм */
static int can_tx(uint32_t std_id, const uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef hdr = {0};
    uint32_t mb;

    hdr.StdId = std_id;
    hdr.IDE   = CAN_ID_STD;
    hdr.RTR   = CAN_RTR_DATA;
    hdr.DLC   = len;

    return (HAL_CAN_AddTxMessage(&hcan, &hdr, (uint8_t *)data, &mb) == HAL_OK) ? 0 : -1;
}

/* ====== PUBLIC API ====== */

/**
 * @brief ПУБЛИКАЦИЯ: реле изменилось.
 * Вызывается из relay_driver при каждом изменении.
 */
void can_publish_relay_changed(uint8_t relay_mask, uint8_t relay_index, uint8_t state)
{
    uint8_t d[CAN_DATA_LENGTH] = {0};
    d[0] = relay_mask;         /* Полная маска всех реле */
    d[1] = relay_index;        /* Какое реле изменилось (0-7) */
    d[2] = state;              /* 0 = off, 1 = on */

    can_tx(COB_ID_PDO_TX + can_node_id, d, CAN_DATA_LENGTH);
}

/**
 * @brief Обработка входящего CAN-фрейма.
 */
void can_process_rx(uint32_t cob_id, const uint8_t *data, uint8_t len)
{
    uint32_t func = cob_id & 0x7F0;  /* Тип сообщения */
    uint8_t  node = cob_id & 0x7F;   /* Node ID */

    (void)len;

    /* --- RPDO: команда на реле --- */
    if (func == COB_ID_PDO_RX && node == can_node_id) {
        uint8_t new_mask = data[0];

        if (new_mask != relay_states) {
            for (int i = 0; i < 8; i++) {
                if ((new_mask & (1 << i)) != (relay_states & (1 << i))) {
                    if (new_mask & (1 << i)) {
                        relay_on(i);
                    } else {
                        relay_off(i);
                    }
                }
            }
            relay_states = new_mask;
            led_signal_ack();
        }
        return;
    }

    /* --- SDO Request --- */
    if (func == COB_ID_SDO_RX && node == can_node_id) {
        uint8_t cmd = data[0];
        uint16_t idx = (uint16_t)(data[2] << 8) | data[1];
        uint8_t sub = data[3];
        uint8_t resp[CAN_DATA_LENGTH] = {0};

        if (cmd == SDO_UPLOAD_REQ) {
            switch (idx) {
                case SDO_IDX_NODE_ID:
                    resp[0] = SDO_UPLOAD_RESP;
                    resp[1] = idx & 0xFF; resp[2] = (idx >> 8) & 0xFF;
                    resp[3] = sub; resp[4] = can_node_id;
                    break;
                case SDO_IDX_RELAY_STATE:
                    resp[0] = SDO_UPLOAD_RESP;
                    resp[1] = idx & 0xFF; resp[2] = (idx >> 8) & 0xFF;
                    resp[3] = sub; resp[4] = relay_states;
                    break;
                default:
                    resp[0] = SDO_ABORT;
                    resp[1] = idx & 0xFF; resp[2] = (idx >> 8) & 0xFF;
                    resp[3] = sub; resp[4] = 0x06; resp[5] = 0x02;
                    break;
            }
            can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
        }
        else if (cmd == SDO_DOWNLOAD_EXP) {
            switch (idx) {
                case SDO_IDX_NODE_ID:
                    can_adapter_set_node_id(data[4]);
                    break;
                case SDO_IDX_RELAY_STATE:
                    relay_states = data[4];
                    for (int i = 0; i < 8; i++) {
                        if (relay_states & (1 << i)) relay_on(i);
                        else relay_off(i);
                    }
                    break;
                default:
                    break;
            }
            resp[0] = SDO_DOWNLOAD_RESP;
            resp[1] = idx & 0xFF; resp[2] = (idx >> 8) & 0xFF;
            resp[3] = sub;
            can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
        }
        return;
    }
}

void can_adapter_set_node_id(uint8_t id)
{
    if (id >= 1 && id <= 127) {
        can_node_id = id;
        Flash_Write(CAN_NODE_ID_ADDR, (uint32_t)id);
    }
}

uint8_t can_adapter_get_node_id(void)
{
    return can_node_id;
}

/* ====== Инициализация ====== */

void can_adapter_init(void)
{
    /* Загрузить Node ID из Flash */
    uint32_t saved = Flash_Read(CAN_NODE_ID_ADDR);
    if (saved >= 1 && saved <= 127) {
        can_node_id = (uint8_t)saved;
    } else {
        can_node_id = 1;
        Flash_Write(CAN_NODE_ID_ADDR, 1);
    }

    /* Инициализировать relay_states из актуального состояния */
    for (int i = 0; i < 8; i++) {
        if (Relay_GetState(i)) relay_states |= (1 << i);
    }

    /* Настроить CAN фильтры — принимаем всё (broadcast + наш узел) */
    CAN_FilterTypeDef flt = {0};
    flt.FilterBank          = 0;
    flt.FilterMode          = CAN_FILTERMODE_IDMASK;
    flt.FilterScale         = CAN_FILTERSCALE_32BIT;
    flt.FilterIdHigh        = 0x0000 << 5;
    flt.FilterIdLow         = 0x0000;
    flt.FilterMaskIdHigh    = 0x0000 << 5;
    flt.FilterMaskIdLow     = 0x0000;
    flt.FilterFIFOAssignment = CAN_RX_FIFO0;
    flt.FilterActivation    = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &flt);

    /* Запустить CAN */
    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

/* ====== HAL Callback'и ====== */

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx;
    uint8_t data[CAN_DATA_LENGTH] = {0};

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx, data) == HAL_OK) {
        can_process_rx(rx.StdId, data, rx.DLC);
    }
}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef *hcan)
{
    (void)hcan;
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    uint32_t err = HAL_CAN_GetError(hcan);
    printf("[CAN] Error: 0x%lX\r\n", err);
}
