#include "can_adapter.h"
#include "board_config.h"
#include "relay_driver.h"
#include "led_driver.h"
#include "flash_driver.h"
#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define PRINTLN(fmt, ...) printf(fmt "\r\n", ##__VA_ARGS__)

/* --- SDO Segmented Transfer State --- */
static struct {
    bool in_progress;
    const uint8_t *data;
    uint32_t size;
    uint32_t offset;
    uint8_t toggle;
} sdo_segmented_upload_state = {false, NULL, 0, 0, 0};

/* --- Внутреннее состояние --- */
static uint8_t can_node_id = 0; /* Node ID устройства (1-127), 0 = unconfigured */
static uint8_t relay_states = 0; /* Битовая маска 8 реле */

/* ====== Вспомогательные ====== */

/** Сбросить состояние сегментированной передачи */
static void sdo_reset_segmented_transfer() {
    sdo_segmented_upload_state.in_progress = false;
}

/** Отправить CAN-фрейм */
static int can_tx(uint32_t std_id, const uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef hdr = {0};
    uint32_t mb;
    hdr.StdId = std_id;
    hdr.IDE = CAN_ID_STD;
    hdr.RTR = CAN_RTR_DATA;
    hdr.DLC = len;

    #if ENABLE_USART_DEBUG
    printf("CAN_TX ID=0x%03X len=%d data=",(unsigned)std_id, (unsigned)len);
    for (size_t i = 0; i < len; i++)
    {

        printf("%02X ", data[i]);

    }

    printf("\r\n");
    #endif
    

    if (HAL_CAN_AddTxMessage(&hcan, &hdr, (uint8_t *)data, &mb) != HAL_OK)
    {
        PRINTLN("CAN_TX ERROR: HAL_CAN_AddTxMessage failed!");
        return -1;
    }
    return 0;
}

/** Запустить сегментированную SDO-передачу */
static void sdo_start_segmented_upload(uint16_t index, uint8_t subindex, const char *data_str) {
    sdo_segmented_upload_state.in_progress = true;
    sdo_segmented_upload_state.data = (const uint8_t *)data_str;
    sdo_segmented_upload_state.size = strlen(data_str);
    sdo_segmented_upload_state.offset = 0;
    sdo_segmented_upload_state.toggle = 0;

    uint8_t resp[CAN_DATA_LENGTH] = {0};
    // Initiate Segmented Upload Response (Command 0x41)
    resp[0] = 0x41; 
    resp[1] = index & 0xFF;
    resp[2] = (index >> 8) & 0xFF;
    resp[3] = subindex;
    // Copy the total size into bytes 4-7 of the response
    memcpy(&resp[4], &sdo_segmented_upload_state.size, 4); 
    can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
}

/** Отправить SDO Abort-сообщение */
static void can_send_sdo_abort(uint16_t index, uint8_t subindex, uint32_t abort_code)
{
    uint8_t resp[CAN_DATA_LENGTH] = {0};
    resp[0] = SDO_ABORT;
    resp[1] = index & 0xFF;
    resp[2] = (index >> 8) & 0xFF;
    resp[3] = subindex;
    memcpy(&resp[4], &abort_code, 4);
    can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
}

/** Отправить Boot-up сообщение (при старте) */
static void can_send_bootup(void)
{
    uint8_t bootup_data = 0x00;
    can_tx(COB_ID_NMT + can_node_id, &bootup_data, 1);
}

/* ====== PUBLIC API ====== */

void can_send_heartbeat(void)
{
    uint8_t heartbeat_data = 0x05; /* 0x05 = Operational state */
    can_tx(COB_ID_HEARTBEAT + can_node_id, &heartbeat_data, 1);
}

void can_publish_relay_changed(uint8_t relay_mask, uint8_t relay_index, uint8_t state)
{
    uint8_t all_states_payload[1] = {relay_mask};
    uint32_t tpdo_cob_id = COB_ID_PDO_TX + can_node_id;
    can_tx(tpdo_cob_id, all_states_payload, 1);
}

void can_process_rx(uint32_t cob_id, const uint8_t *data, uint8_t len)
{
    uint32_t func = cob_id & 0x780;
    uint8_t node = cob_id & 0x7F;

#if ENABLE_USART_DEBUG
    printf("CAN_RX COB_ID=0x%03X func=0x%03X node=%d data=",
           (unsigned)cob_id, (unsigned)func, node);
    for (size_t i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\r\n");
#endif
    /* Индикация приёма CAN-фрейма */
    led_signal_can_rx_from_isr();

    /* --- NMT --- */
    if (func == COB_ID_NMT && node == 0)
    {
        PRINTLN("CAN_RX NMT command received and skipped: cmd=0x%02X", data[0]);
        sdo_reset_segmented_transfer();
        return;
    }
    /* --- RPDO1 for Relay Control --- */
    if (cob_id == (COB_ID_PDO_RX + can_node_id))
    {
        if (len >= 2)
        {
            uint8_t relay_index = data[0];
            uint8_t new_state = data[1];

            if (relay_index < NUM_RELAYS)
            {
                if (new_state)
                    relay_on(relay_index);
                else
                    relay_off(relay_index);

                // Update internal state cache
                if (new_state)
                    relay_states |= (1 << relay_index);
                else
                    relay_states &= ~(1 << relay_index);
            }
        }
        return;
    }
    /* --- SDO Request --- */
    if (func == COB_ID_SDO_RX && node == can_node_id)
    {
        uint8_t cmd = data[0];
        uint16_t idx = (uint16_t)(data[2] << 8) | data[1];
        uint8_t sub = data[3];
        uint8_t resp[CAN_DATA_LENGTH] = {0};

        // --- SDO Segmented Upload: Client requesting next segment ---
        if ((cmd & 0xE0) == 0x60)
        {
            if (!sdo_segmented_upload_state.in_progress) {
                can_send_sdo_abort(idx, sub, 0x05040001); // SDO command specifier invalid or unknown
                return;
            }

            uint8_t client_toggle = (cmd >> 4) & 1;
            if (client_toggle != sdo_segmented_upload_state.toggle) {
                can_send_sdo_abort(idx, sub, 0x05030000); // Toggle bit not alternated
                sdo_reset_segmented_transfer();
                return;
            }

            uint32_t remaining_bytes = sdo_segmented_upload_state.size - sdo_segmented_upload_state.offset;
            uint8_t bytes_to_send = (remaining_bytes > 7) ? 7 : remaining_bytes;
            bool is_last_segment = (bytes_to_send == remaining_bytes);

            // Build response: [t|n|c] + data
            resp[0] = (sdo_segmented_upload_state.toggle << 4) | ((7 - bytes_to_send) << 1) | (is_last_segment ? 1 : 0);
            memcpy(&resp[1], sdo_segmented_upload_state.data + sdo_segmented_upload_state.offset, bytes_to_send);

            can_tx(COB_ID_SDO_TX + can_node_id, resp, bytes_to_send + 1);

            sdo_segmented_upload_state.offset += bytes_to_send;
            sdo_segmented_upload_state.toggle = !sdo_segmented_upload_state.toggle;

            if (is_last_segment) {
                sdo_reset_segmented_transfer();
            }
            return;
        }

        // If a new request comes in while a transfer is in progress, abort the old one.
        if (sdo_segmented_upload_state.in_progress && cmd != SDO_ABORT) {
            sdo_reset_segmented_transfer();
        }

        resp[1] = idx & 0xFF;
        resp[2] = (idx >> 8) & 0xFF;
        resp[3] = sub;
#if ENABLE_USART_DEBUG
        PRINTLN("CAN_RX SDO: cmd=0x%02X id=0x%04X sub=%d", cmd, idx, sub);
#endif
        if (cmd == SDO_UPLOAD_REQ) // --- READ ---
        {
            switch (idx)
            {
            case SDO_IDX_DEVICE_TYPE:
                resp[0] = SDO_UPLOAD_RESP_4_BYTES;
                *((uint32_t *)&resp[4]) = 0;
                break;

            case SDO_IDX_DEVICE_NAME:
            {
                static const char* device_name = "BluePill Relay Controller";
                size_t name_len = strlen(device_name);
                if (name_len <= 4) {
                    resp[0] = 0x43 | ((4 - name_len) << 2);
                    memcpy(&resp[4], device_name, name_len);
                    can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                } else {
                    sdo_start_segmented_upload(idx, sub, device_name);
                }
                return; // MUST return here
            }
            case SDO_IDX_PRODUCER_HEARTBEAT_TIME:
                resp[0] = SDO_UPLOAD_RESP_2_BYTES;
                *((uint16_t *)&resp[4]) = HEARTBEAT_TIME_MS;
                break; // This break is OK

            case SDO_IDX_IDENTITY_OBJECT:
                if (sub == 0) {
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 4;
                } else {
                    resp[0] = SDO_UPLOAD_RESP_4_BYTES;
                    if (sub == 1)
                        *((uint32_t *)&resp[4]) = CANOPEN_VENDOR_ID;
                    else if (sub == 2)
                        *((uint32_t *)&resp[4]) = CANOPEN_PRODUCT_CODE;
                    else if (sub == 3)
                        *((uint32_t *)&resp[4]) = CANOPEN_REVISION_NUMBER;
                    else if (sub == 4)
                        *((uint32_t *)&resp[4]) = CANOPEN_SERIAL_NUMBER;
                    else {
                        can_send_sdo_abort(idx, sub, 0x06090011);
                        return;
                    }
                }
                break; // This break is OK

            default:
                PRINTLN("SDO Abort: unknown id=0x%04X", idx);
                can_send_sdo_abort(idx, sub, 0x06020000);
                return;
            }
            can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
        }
        else if (cmd == SDO_DOWNLOAD_EXP) // --- WRITE ---
        {
            uint32_t abort_code = 0;
            switch (idx)
            {
            case SDO_IDX_NODE_ID:
                if (sub == 0)
                {
                    can_adapter_set_node_id(data[4]);
                }
                else
                {
                    abort_code = 0x06090011;
                }
                break;
            default:
                abort_code = 0x06010002;
                break;
            }

            if (abort_code)
            {
                can_send_sdo_abort(idx, sub, abort_code);
            }
            else
            {
                resp[0] = SDO_DOWNLOAD_RESP;
                can_tx(COB_ID_SDO_TX + can_node_id, resp, 3);
            }
        }
    }
}

void can_adapter_set_node_id(uint8_t id)
{
    if (id >= 1 && id <= 127)
    {
        can_node_id = id;
        Flash_Write(CAN_NODE_ID_ADDR, (uint32_t)id);
    }
}

uint8_t can_adapter_get_node_id(void)
{
    return can_node_id;
}

void can_adapter_init(void)
{
    PRINTLN("[CAN_INIT] Starting...");

    uint32_t saved = Flash_Read(CAN_NODE_ID_ADDR);
    if (saved >= 1 && saved < 127)
    {
        can_node_id = (uint8_t)saved;
    }
    else
    {
        can_node_id = CAN_NODE_ID_DEFAULT; // Unconfigured node ID
        Flash_Write(CAN_NODE_ID_ADDR, can_node_id);
    }
    PRINTLN("[CAN_INIT] Node ID = %d", can_node_id);
    for (int i = 0; i < 8; i++)
    {
        if (relay_getState(i))
            relay_states |= (1 << i);
    }
    PRINTLN("[CAN_INIT] Relay states = 0x%02X", relay_states);

    CAN_FilterTypeDef flt = {0};
    flt.FilterBank = 0;
    flt.FilterMode = CAN_FILTERMODE_IDMASK;
    flt.FilterScale = CAN_FILTERSCALE_32BIT;
    flt.FilterIdHigh = 0x0000;
    flt.FilterIdLow = 0x0000;
    flt.FilterMaskIdHigh = 0x0000;
    flt.FilterMaskIdLow = 0x0000;
    flt.FilterFIFOAssignment = CAN_RX_FIFO0;
    flt.FilterActivation = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &flt);
    PRINTLN("[CAN_INIT] Filter accept all");

    HAL_CAN_ResetError(&hcan);
    if (HAL_CAN_Start(&hcan) != HAL_OK)
    {
        PRINTLN("[CAN_INIT] ERROR: HAL_CAN_Start failed!");
    }
    else
    {
        PRINTLN("[CAN_INIT] CAN started OK");
    }

    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_TX_MAILBOX_EMPTY);
    PRINTLN("[CAN_INIT] Notifications (RX, TX, ERROR)");

    can_send_bootup();
    PRINTLN("[CAN_INIT]Init complete");
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx;
    uint8_t data[CAN_DATA_LENGTH] = {0};
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx, data) == HAL_OK)
    {
        can_process_rx(rx.StdId, data, rx.DLC);
    }
}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef *hcan)
{
    (void)hcan;
}
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    (void)hcan;
    uint32_t err = HAL_CAN_GetError(hcan);
    PRINTLN("[CAN_ERR] Error callback, err=0x%08lX", (unsigned long)err);
}
