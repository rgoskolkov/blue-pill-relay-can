#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#include <stdint.h>

/* --- Flash адреса для хранения конфигурации --- */
/* STM32F103C8 имеет 64KB Flash (0x08000000 - 0x0800FFFF) */
/* Используем последнюю страницу (страница 127, 2KB) */
#define FLASH_CONFIG_ADDR       0x0800FC00
#define FLASH_MAGIC_NUMBER      0xA5A5A5A5

/* --- Смещения для разных параметров --- */
#define FLASH_MAGIC_OFFSET      0x00
#define CAN_NODE_ID_OFFSET      0x04
#define CAN_NODE_ID_ADDR        (FLASH_CONFIG_ADDR + CAN_NODE_ID_OFFSET)

/* --- Функции работы с Flash --- */
int8_t Flash_Write(uint32_t addr, uint32_t value);
uint32_t Flash_Read(uint32_t addr);

#endif /* FLASH_DRIVER_H */
