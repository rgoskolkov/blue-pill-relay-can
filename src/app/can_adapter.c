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
static uint8_t can_node_id = 0; /* Node ID устройства (1-127), 0 = unconfigured */
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
    uint8_t all_states_payload[1] = {relay_mask};
    uint32_t tpdo_cob_id = COB_ID_PDO_TX + can_node_id;
    can_tx(tpdo_cob_id, all_states_payload, 1);
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
            if (idx == ENTITY_LIST_IDX)
            {
                if (sub == 0) // Number of entries in the entity list
                {
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = NUM_RELAYS; // Assuming NUM_RELAYS is the count of entities
                }
                else if (sub > 0 && sub <= NUM_RELAYS) // Entity Type ID
                {
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE; // Returning 1-byte entity type ID
                    resp[4] = 5; // Assuming '5' is the entity type ID for a light/switch
                }
                else
                {
                    can_send_sdo_abort(idx, sub, 0x06090011); // Sub-index does not exist
                    return;
                }
                can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                return;
            }

            // Handle entity blocks (0x2100 to 0x2100 + NUM_RELAYS * ENTITY_BLOCK_SIZE)
            if (idx >= ENTITY_BLOCK_BASE_IDX && idx < ENTITY_BLOCK_BASE_IDX + NUM_RELAYS * ENTITY_BLOCK_SIZE)
            {
                uint8_t entity_index = (idx - ENTITY_BLOCK_BASE_IDX) / ENTITY_BLOCK_SIZE;
                uint8_t offset_in_block = (idx - ENTITY_BLOCK_BASE_IDX) % ENTITY_BLOCK_SIZE;

                if (entity_index >= NUM_RELAYS)
                {
                    can_send_sdo_abort(idx, sub, 0x06020000); // Object does not exist
                    return;
                }

                switch (offset_in_block)
                {
                    case ENTITY_METADATA_OFFSET: // 0x21XX: Metadata (Name, Device Class, Object ID, Manufacturer)
                        if (sub == 0) // Number of entries
                        {
                            resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                            resp[4] = 4; // Name, Device Class, Object ID, Manufacturer
                        }
                        else if (sub == 1) // Name
                        {
                            char name_buf[10];
                            sprintf(name_buf, "Relay %d", entity_index + 1);
                            size_t name_len = strlen(name_buf);
                            size_t copy_len = (name_len > 4) ? 4 : name_len;
                            resp[0] = SDO_UPLOAD_RESP_4_BYTES - (4 - copy_len); // Adjust command based on actual length
                            memcpy(&resp[4], name_buf, copy_len);
                        }
                        else if (sub == 2) // Device Class (e.g., Light)
                        {
                            resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                            resp[4] = 5; // Light (0x05)
                        }
                        else if (sub == 3) // Object ID
                        {
                            char obj_id_buf[10];
                            sprintf(obj_id_buf, "relay_%d", entity_index + 1);
                            size_t obj_id_len = strlen(obj_id_buf);
                            size_t copy_len = (obj_id_len > 4) ? 4 : obj_id_len;
                            resp[0] = SDO_UPLOAD_RESP_4_BYTES - (4 - copy_len); // Adjust command based on actual length
                            memcpy(&resp[4], obj_id_buf, copy_len);
                        }
                        else if (sub == 4) // Manufacturer
                        {
                            const char* manufacturer_name = "BluePill";
                            size_t man_len = strlen(manufacturer_name);
                            size_t copy_len = (man_len > 4) ? 4 : man_len;
                            resp[0] = SDO_UPLOAD_RESP_4_BYTES - (4 - copy_len); // Adjust command based on actual length
                            memcpy(&resp[4], manufacturer_name, copy_len);
                        }
                        else
                        {
                            can_send_sdo_abort(idx, sub, 0x06090011); // Sub-index does not exist
                            return;
                        }
                        break;
                    case ENTITY_STATE_OFFSET: // 0x21XX+1: Current state
                        if (sub == 0) // Number of entries
                        {
                            resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                            resp[4] = 1;
                        }
                        else if (sub == 1) // State value
                        {
                            resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                            resp[4] = (relay_states >> entity_index) & 0x01;
                        }
                        else
                        {
                            can_send_sdo_abort(idx, sub, 0x06090011); // Sub-index does not exist
                            return;
                        }
                        break;
                    case ENTITY_COMMAND_OFFSET: // 0x21XX+2: Command (write-only)
                        can_send_sdo_abort(idx, sub, 0x06010002); // Attempt to read a write-only object
                        return;
                    default:
                        can_send_sdo_abort(idx, sub, 0x06020000); // Object does not exist
                        return;
                }
                can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                return;
            }

            // TPDO1 Communication Parameters (0x1800)
            if (idx == OD_TPDO_COM_PARAM_BASE) 
            {
                if (sub == 0) { // Highest sub-index supported
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 2; // COB-ID and Transmission Type
                } else if (sub == 1) { // COB-ID
                    resp[0] = SDO_UPLOAD_RESP_4_BYTES;
                    // Valid 11-bit ID, Event-triggered. COB-ID is 0x180 + node_id
                    *((uint32_t *)&resp[4]) = COB_ID_PDO_TX + can_node_id;
                } else if (sub == 2) { // Transmission type
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 254; // Event-driven on change
                } else {
                    can_send_sdo_abort(idx, sub, 0x06090011); // Sub-index does not exist
                    return;
                }
                can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                return;
            }

            // RPDOs are not used for command, SDO is used instead.
            // This section can be removed or left empty if needed for future use.
            if (idx >= OD_RPDO_COM_PARAM_BASE && idx < OD_RPDO_COM_PARAM_BASE + NUM_RELAYS)
            {
                can_send_sdo_abort(idx, sub, 0x06020000); // Object does not exist
                return;
            }

            // TPDO1 Mapping Parameters (0x1A00)
            if (idx == OD_TPDO_MAPPING_PARAM_BASE)
            {
                if (sub == 0) { // Number of mapped objects
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = 1; // We map 1 object: the relay states bitmask
                } else if (sub == 1) { // Mapping entry for the first (and only) object
                    resp[0] = SDO_UPLOAD_RESP_4_BYTES;
                    // Map OD_IDX_RELAY_BITMASK:00, 8 bits length
                    uint32_t mapping = (OD_IDX_RELAY_BITMASK << 16) | (0 << 8) | 8;
                    *((uint32_t *)&resp[4]) = mapping;
                } else {
                    can_send_sdo_abort(idx, sub, 0x06090011); // Sub-index does not exist
                    return;
                }
                can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
                return;
            }

            // TPDO Mapping for individual entities - REMOVED as we use a single bitmask TPDO
            if (idx >= OD_TPDO_MAPPING_PARAM_BASE + 1 && idx < OD_TPDO_MAPPING_PARAM_BASE + NUM_RELAYS) {
                can_send_sdo_abort(idx, sub, 0x06020000); // Object does not exist
                return;
            }

            switch (idx)
            {
            case OD_IDX_RELAY_BITMASK: // Relay states bitmask (Read-Only)
                if (sub == 0) {
                    resp[0] = SDO_UPLOAD_RESP_1_BYTE;
                    resp[4] = relay_states;
                } else {
                    can_send_sdo_abort(idx, sub, 0x06090011); // Sub-index does not exist
                    return;
                }
                break;
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
                size_t len_sw = strlen(sw_version);
                size_t copy_len_sw = (len_sw > 4) ? 4 : len_sw;
                // Adjust command based on actual length of string, up to 4 bytes for expedited transfer
                resp[0] = SDO_UPLOAD_RESP_4_BYTES - (4 - copy_len_sw); 
                memcpy(&resp[4], sw_version, copy_len_sw);
                break;
            case SDO_IDX_HARDWARE_VERSION:
                PRINTLN("SDO HW Ver (idx=0x%04X)", idx);
                const char* hw_version = "1.0";
                size_t len_hw = strlen(hw_version);
                size_t copy_len_hw = (len_hw > 4) ? 4 : len_hw;
                resp[0] = SDO_UPLOAD_RESP_4_BYTES - (4 - copy_len_hw);
                memcpy(&resp[4], hw_version, copy_len_hw);
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
            default:
                PRINTLN("SDO Abort: unknown id=0x%04X", idx);
                can_send_sdo_abort(idx, sub, 0x06020000); // Object does not exist
                return;
            }
            can_tx(COB_ID_SDO_TX + can_node_id, resp, CAN_DATA_LENGTH);
        }
        else if (cmd == SDO_DOWNLOAD_EXP) // --- WRITE ---
        {
            // Handle entity command writes
            if (idx >= ENTITY_BLOCK_BASE_IDX && idx < ENTITY_BLOCK_BASE_IDX + NUM_RELAYS * ENTITY_BLOCK_SIZE)
            {
                uint8_t entity_index = (idx - ENTITY_BLOCK_BASE_IDX) / ENTITY_BLOCK_SIZE;
                uint8_t offset_in_block = (idx - ENTITY_BLOCK_BASE_IDX) % ENTITY_BLOCK_SIZE;

                if (entity_index >= NUM_RELAYS)
                {
                    can_send_sdo_abort(idx, sub, 0x06020000); // Object does not exist
                    return;
                }

                if (offset_in_block == ENTITY_COMMAND_OFFSET) // 0x21XX+2: Command
                {
                    if (sub == 1) // State value
                    {
                        uint8_t new_state = data[4] & 0x01;
                        if (new_state)
                            relay_on(entity_index);
                        else
                            relay_off(entity_index);
                        
                        // Update local relay_states
                        if (new_state)
                            relay_states |= (1 << entity_index);
                        else
                            relay_states &= ~(1 << entity_index);

                        resp[0] = SDO_DOWNLOAD_RESP;
                        can_tx(COB_ID_SDO_TX + can_node_id, resp, 3);
                        return;
                    }
                    else
                    {
                        can_send_sdo_abort(idx, sub, 0x06090011); // Sub-index does not exist
                        return;
                    }
                }
                else
                {
                    can_send_sdo_abort(idx, sub, 0x06010002); // Attempt to write a read-only object
                    return;
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
