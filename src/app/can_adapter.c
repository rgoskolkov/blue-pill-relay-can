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

/* --- Внутреннее состояние --- */
static uint8_t can_node_id = 1;
static uint8_t relay_states = 0; /* Битовая маска 8 реле */

/* ====== Вспомогательные ====== */

/** Отправить CAN-фрейм */
static int can_tx(uint32_t std_id, const uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef hdr = {0};
    uint32_t mb;
    hdr.StdId = std_id;
    hdr.IDE = CAN_ID_STD;
    hdr.RTR = CAN_RTR_DATA;
    hdr.DLC = len;

    printf("CAN_TX ID=0x%03X len=%d data=",(unsigned)std_id, (unsigned)len);
    for (size_t i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\r\n");
    

    if (HAL_CAN_AddTxMessage(&hcan, &hdr, (uint8_t *)data, &mb) != HAL_OK)
    {
        PRINTLN("CAN_TX ERROR: HAL_CAN_AddTxMessage failed!");
        return -1;
    }
    return 0;
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
    uint8_t single_byte_payload[1] = {state & 0x01};
    uint32_t tpdo_cob_id = COB_ID_PDO_TX + can_node_id + relay_index + 1; /* entity index starts from 1 */
    can_tx(tpdo_cob_id, single_byte_payload, 1);
}

void can_process_rx(uint32_t cob_id, const uint8_t *data, uint8_t len)
{
    uint32_t func = cob_id & 0x780;
    uint8_t node = cob_id & 0x7F;

    printf("CAN_RX COB_ID=0x%03X func=0x%03X node=%d data=",
            (unsigned)cob_id, (unsigned)func, node);

    for (size_t i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\r\n");

    /* Индикация приёма CAN-фрейма */
    led_signal_can_rx_from_isr();

    /* --- NMT --- */
    if (func == COB_ID_NMT && node == 0)
    {
        PRINTLN("CAN_RX NMT command received and skipped: cmd=0x%02X", data[0]);
        return;
    }
    /* --- SDO Request --- */
    if (func == COB_ID_SDO_RX && node == can_node_id)
    {
        uint8_t cmd = data[0];
        uint16_t idx = (uint16_t)(data[2] << 8) | data[1];
        uint8_t sub = data[3];
        uint8_t resp[CAN_DATA_LENGTH] = {0};

        resp[1] = idx & 0xFF;
        resp[2] = (idx >> 8) & 0xFF;
        resp[3] = sub;
        PRINTLN("CAN_RX SDO: cmd=0x%02X id=0x%04X sub=%d", cmd, idx, sub);

        if (cmd == SDO_UPLOAD_REQ) // --- READ ---
        {
            const uint16_t entity_metadata_start_idx = OD_ENTITY_BASE + OD_ENTITY_METADATA_OFFSET;
            const uint16_t entity_state_start_idx = OD_ENTITY_BASE + OD_ENTITY_STATE_OFFSET;

            // Handle switch state reads (e.g., 0x2011, 0x2021, ...)
            if (idx >= entity_state_start_idx && idx < entity_state_start_idx + NUM_RELAYS * OD_ENTITY_BLOCK_SIZE)
            {
                if (((idx - entity_state_start_idx) % OD_ENTITY_BLOCK_SIZE == 0) && sub == 1)
                {
                    uint8_t entity_index = (idx - entity_state_start_idx) / OD_ENTITY_BLOCK_SIZE;
                    if (entity_index < NUM_RELAYS)
                    {
                        resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                        resp[4] = (relay_states >> entity_index) & 0x01;
                        can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                        return;
                    }
                }
            }

            // Handle entity metadata base indexes (e.g., 0x2010, 0x2020, ...)
            if (idx >= entity_metadata_start_idx && idx < entity_metadata_start_idx + NUM_RELAYS * OD_ENTITY_BLOCK_SIZE && (idx % OD_ENTITY_BLOCK_SIZE == 0))
            {
                if (sub == 0) {
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 0; // No metadata properties
                } else {
                   can_send_sdo_abort(idx, sub, 0x06090011); // Sub-index does not exist
                   return;
                }
                can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                return;
            }

            // TPDO Communication Parameters (0x1800 to 0x1800 + NUM_RELAYS - 1)
            if (idx >= OD_TPDO_COM_PARAM_BASE && idx < OD_TPDO_COM_PARAM_BASE + NUM_RELAYS)
            {
                uint8_t pdo_index = idx - OD_TPDO_COM_PARAM_BASE;
                if (sub == 0) { // Highest sub-index supported
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 2;
                } else if (sub == 1) { // COB-ID
                    resp[0] = SDO_UPLOAD_RESP_4_BYTES;
                    *((uint32_t *)&resp[4]) = COB_ID_PDO_TX + can_node_id + pdo_index + 1;
                } else if (sub == 2) { // Transmission type
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 254; // Event-driven
                } else {
                    can_send_sdo_abort(idx, sub, 0x06090011);
                    return;
                }
                can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                return;
            }

            // RPDO Communication Parameters (0x1400 to 0x1400 + NUM_RELAYS - 1)
            if (idx >= OD_RPDO_COM_PARAM_BASE && idx < OD_RPDO_COM_PARAM_BASE + NUM_RELAYS)
            {
                uint8_t pdo_index = idx - OD_RPDO_COM_PARAM_BASE;
                if (sub == 0) { // Highest sub-index supported
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 2;
                } else if (sub == 1) { // COB-ID
                    resp[0] = SDO_UPLOAD_RESP_4_BYTES;
                    *((uint32_t *)&resp[4]) = COB_ID_PDO_RX + can_node_id + pdo_index + 1;
                } else if (sub == 2) { // Transmission type
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 254; // Event-driven
                } else {
                    can_send_sdo_abort(idx, sub, 0x06090011);
                    return;
                }
                can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                return;
            }

            // TPDO Mapping Parameters (0x1A00 to 0x1A00 + NUM_RELAYS - 1)
            if (idx >= OD_TPDO_MAPPING_PARAM_BASE && idx < OD_TPDO_MAPPING_PARAM_BASE + NUM_RELAYS)
            {
                uint8_t pdo_index = idx - OD_TPDO_MAPPING_PARAM_BASE;
                if (sub == 0) { // Number of mapped objects
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 1; // We map 1 object: the switch state
                } else if (sub == 1) { // Mapping entry for the first (and only) object
                    resp[0] = SDO_UPLOAD_RESP_4_BYTES;
                    uint32_t mapped_object_index = entity_state_start_idx + pdo_index * OD_ENTITY_BLOCK_SIZE;
                    uint32_t mapping = (mapped_object_index << 16) | (1 << 8) | 8; // Index, Subindex (1), Length (8 bits)
                    *((uint32_t *)&resp[4]) = mapping;
                } else {
                    can_send_sdo_abort(idx, sub, 0x06090011);
                    return;
                }
                can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                return;
            }

            switch (idx)
            {
            case SDO_IDX_DEVICE_TYPE:
                PRINTLN("SDO Dev Type (idx=0x%04X)", idx);
                resp[0] = SDO_UPLOAD_RESP_4_BYTES;
                *((uint32_t *)&resp[4]) = 0; // Generic device
                break;
            case SDO_IDX_DEVICE_NAME:
                PRINTLN("SDO Dev Name (idx=0x%04X)", idx);
                resp[0] = SDO_UPLOAD_RESP_4_BYTES;
                strncpy((char *)&resp[4], "Relay", 4);
                break;
            case SDO_IDX_SW_VERSION:
                PRINTLN("SDO SW Ver (idx=0x%04X)", idx);
                const char* sw_version = "1.0";
                size_t len = strlen(sw_version);
                resp[0] = 0x4F - ((len - 1) * 4); // Expedited transfer command
                memcpy(&resp[4], sw_version, len);
                break;
            case SDO_IDX_ERROR_REGISTER:
                PRINTLN("SDO Error Reg (idx=0x%04X)", idx);
                resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                resp[4] = 0x00;
                break;
            case SDO_IDX_PRODUCER_HEARTBEAT_TIME:
                PRINTLN("SDO Heartbeat Time (idx=0x%04X)", idx);
                resp[0] = SDO_UPLOAD_RESP_2_BYTES;
                *((uint16_t *)&resp[4]) = HEARTBEAT_TIME_MS;
                break;
            case SDO_IDX_IDENTITY_OBJECT:
                // If node ID is 127 (unconfigured), pretend this object doesn't exist
                // to prevent can2mqtt from trying to register it.
                if (can_node_id == 127) {
                    can_send_sdo_abort(idx, sub, 0x06020000); // Object does not exist
                    return;
                }

                PRINTLN("SDO Ident Obj (idx=0x%04X, sub=%d) ", idx, sub);
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
                    else
                    {
                        can_send_sdo_abort(idx, sub, 0x06090011);
                        return;
                    }
                }
                break;
            case OD_ENTITY_TYPES_REV1: // 0x2001
                PRINTLN("SDO Entity Types (idx=0x%04X, sub=%d)", idx, sub);
                if (sub == 0) {
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = NUM_RELAYS;
                } else if (sub > 0 && sub <= NUM_RELAYS) {
                    resp[0] = SDO_UPLOAD_RESP_4_BYTES; 
                    *((uint32_t *)&resp[4]) = 3;       // Entity Type ID 3 = Switch
                } else {
                    can_send_sdo_abort(idx, sub, 0x06090011);
                    return;
                }
                break;
            default:
                PRINTLN("SDO Abort: unknown id=0x%04X", idx);
                can_send_sdo_abort(idx, sub, 0x06020000); // Object does not exist
                return;
            }
            can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
        }
        else if (cmd == SDO_DOWNLOAD_EXP) // --- WRITE ---
        {
            const uint16_t entity_cmd_start_idx = OD_ENTITY_BASE + OD_ENTITY_COMMAND_OFFSET;
            // Handle switch commands
            if (idx >= entity_cmd_start_idx && idx < entity_cmd_start_idx + NUM_RELAYS * OD_ENTITY_BLOCK_SIZE)
            {
                if (((idx - entity_cmd_start_idx) % OD_ENTITY_BLOCK_SIZE == 0) && sub == 1)
                {
                    uint8_t entity_index = (idx - entity_cmd_start_idx) / OD_ENTITY_BLOCK_SIZE;
                    if (entity_index < NUM_RELAYS)
                    {
                        uint8_t new_state = data[4] & 0x01;
                        if (new_state)
                            relay_on(entity_index);
                        else
                            relay_off(entity_index);
                        if (new_state)
                            relay_states |= (1 << entity_index);
                        else
                            relay_states &= ~(1 << entity_index);

                        resp[0] = SDO_DOWNLOAD_RESP;
                        can_tx(COB_ID_SDO_TX + can_node_id, resp, 3);
                        return;
                    }
                }
            }

            uint32_t abort_code = 0;
            switch (idx)
            {
            case SDO_IDX_NODE_ID: // Custom writable object.
                if (sub == 0)
                {
                    PRINTLN("SDO Node ID (idx=0x%04X, data=0x%02X)", idx, data[4]);
                    can_adapter_set_node_id(data[4]);
                }
                else
                {
                    abort_code = 0x06090011;
                } // Sub-index error
                break;
            default:
                abort_code = 0x06010002; // Attempt to write a read-only object
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
        can_node_id = 127; // Unconfigured node ID
        Flash_Write(CAN_NODE_ID_ADDR, 127);
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