#ifndef CAN_ADAPTER_H
#define CAN_ADAPTER_H

#include "can.h"
#include <stdint.h>

/* --- CANopen-like COB-IDs (11-bit standard) --- */
#define COB_ID_PDO_TX         0x180  /* TPDO: событие изменения реле (device → шина) */
#define COB_ID_PDO_RX         0x200  /* RPDO: команда на реле (шина → device) */
#define COB_ID_SDO_TX         0x580  /* SDO Response: ответ на конфиг запрос */
#define COB_ID_SDO_RX         0x600  /* SDO Request: запрос конфигурации */

#define CAN_DATA_LENGTH       8

/* --- SDO Object Dictionary --- */
#define SDO_IDX_NODE_ID       0x0001  /* Чтение/запись Node ID */
#define SDO_IDX_RELAY_STATE   0x0010  /* Чтение текущего состояния реле */

/* --- SDO команды --- */
#define SDO_UPLOAD_REQ        0x40
#define SDO_UPLOAD_RESP       0x43
#define SDO_DOWNLOAD_EXP      0x2F
#define SDO_DOWNLOAD_RESP     0x60
#define SDO_ABORT             0x80

/* ====== PUBLIC API ====== */

/** Инициализация CAN, фильтрация, отправка Boot-up */
void can_adapter_init(void);

/** Установить Node ID (сохраняет в Flash) */
void can_adapter_set_node_id(uint8_t node_id);

/** Получить текущий Node ID */
uint8_t can_adapter_get_node_id(void);

/** ОПУБЛИКОВАТЬ событие: реле изменило состояние.
 *  Вызывается из relay_driver при каждом изменении.
 *  data[0] = битовая маска всех 8 реле
 *  data[1] = индекс реле, которое изменилось (0-7)
 *  data[2] = новое состояние (0 или 1)
 */
void can_publish_relay_changed(uint8_t relay_mask, uint8_t relay_index, uint8_t state);

/** Обработать входящий CAN-фрейм (вызывается из IRQ callback).
 *  Если пришла команда на реле — переключает реле и публикует событие.
 *  Если SDO запрос — отвечает.
 */
void can_process_rx(uint32_t cob_id, const uint8_t *data, uint8_t len);

#endif // CAN_ADAPTER_H
