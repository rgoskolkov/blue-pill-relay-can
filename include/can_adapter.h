#ifndef CAN_ADAPTER_H
#define CAN_ADAPTER_H

#include "can.h"
#include <stdint.h>

/* --- CANopen-like COB-IDs (11-bit standard) --- */
#define COB_ID_PDO_TX         0x180  /* TPDO: событие изменения реле (device → шина) */
#define COB_ID_PDO_RX         0x200  /* RPDO: команда на реле (шина → device) */
#define COB_ID_SDO_TX         0x580  /* SDO Response: ответ на конфиг запрос */
#define COB_ID_SDO_RX         0x600  /* SDO Request: запрос конфигурации */
#define COB_ID_NMT            0x700  /* NMT команды (broadcast) */
#define COB_ID_HEARTBEAT      0x700  /* Heartbeat сообщения (Node ID в младших битах) */

#define CAN_DATA_LENGTH       8

/* --- SDO команды --- */
#define SDO_UPLOAD_REQ            0x40
#define SDO_UPLOAD_RESP_4_BYTES   0x43
#define SDO_UPLOAD_RESP_3_BYTES   0x47
#define SDO_UPLOAD_RESP_2_BYTES   0x4B
#define SDO_UPLOAD_RESP_1_BYTE    0x4F
#define SDO_DOWNLOAD_EXP          0x2F
#define SDO_DOWNLOAD_RESP         0x60
#define SDO_ABORT                 0x80

/* === Стандартные объекты CANopen === */
#define SDO_IDX_DEVICE_TYPE             0x1000
#define SDO_IDX_ERROR_REGISTER          0x1001
#define SDO_IDX_DEVICE_NAME             0x1008
#define SDO_IDX_SW_VERSION              0x100A
#define SDO_IDX_HARDWARE_VERSION        0x1009
#define SDO_IDX_PRODUCER_HEARTBEAT_TIME 0x1017
#define SDO_IDX_IDENTITY_OBJECT         0x1018

#define OD_RPDO_COM_PARAM_BASE          0x1400
#define OD_TPDO_COM_PARAM_BASE          0x1800
#define OD_RPDO_MAPPING_PARAM_BASE      0x1600
#define OD_TPDO_MAPPING_PARAM_BASE      0x1A00

/* === Специфичные объекты для can2mqtt === */
#define SDO_IDX_NODE_ID                 0x2002  /* Node ID устройства (чтение/запись) */
#define OD_IDX_RELAY_BITMASK            0x2200  /* Relay states bitmask (read-only) */

// === Новые объекты для "умного" обнаружения сущностей ===
#define ENTITY_LIST_IDX         0x2F00 // Индекс для массива типов сущностей
#define ENTITY_BLOCK_BASE_IDX   0x2100 // Базовый индекс для первого блока сущностей
#define ENTITY_BLOCK_SIZE       16     // Размер блока OD для одной сущности (0x2100 - 0x210F)

// Смещения внутри блока сущности
#define ENTITY_METADATA_OFFSET  0  // 0x2100 (для первой сущности)
#define ENTITY_STATE_OFFSET     1  // 0x2101 (для первой сущности)
#define ENTITY_COMMAND_OFFSET   2  // 0x2102 (для первой сущности)

/* Identity Object константы */
#define CANOPEN_VENDOR_ID       0x00000BEE
#define CANOPEN_PRODUCT_CODE    0x0000F103
#define CANOPEN_REVISION_NUMBER 1
#define CANOPEN_SERIAL_NUMBER   0x00000001

/* ====== PUBLIC API ====== */
void can_adapter_init(void);
void can_adapter_set_node_id(uint8_t node_id);
uint8_t can_adapter_get_node_id(void);
void can_publish_relay_changed(uint8_t relay_mask, uint8_t relay_index, uint8_t state);
void can_send_heartbeat(void);

#endif // CAN_ADAPTER_H