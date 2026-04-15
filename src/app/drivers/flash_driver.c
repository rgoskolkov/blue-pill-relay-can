#include "flash_driver.h"
#include "stm32f1xx_hal.h"

/**
 * @brief Записать 32-битное значение во Flash
 */
int8_t Flash_Write(uint32_t addr, uint32_t value)
{
    HAL_StatusTypeDef status;

    // Проверить что адрес в допустимом диапазоне
    if (addr < FLASH_CONFIG_ADDR || addr >= (FLASH_CONFIG_ADDR + 0x400)) {
        return -1;
    }

    HAL_FLASH_Unlock();

    // Очистить страницу (STM32F1 очищает по страницам)
    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_CONFIG_ADDR;
    EraseInitStruct.NbPages = 1;

    uint32_t PageError = 0;
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return -1;
    }

    // Записать magic number
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_CONFIG_ADDR, FLASH_MAGIC_NUMBER);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return -1;
    }

    // Записать значение по указанному адресу
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, value);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return -1;
    }

    HAL_FLASH_Lock();
    return 0;
}

/**
 * @brief Прочитать 32-битное значение из Flash
 */
uint32_t Flash_Read(uint32_t addr)
{
    // Проверить magic number
    uint32_t magic = *(__IO uint32_t*)FLASH_CONFIG_ADDR;
    if (magic != FLASH_MAGIC_NUMBER) {
        return 0xFFFFFFFF; // Flash пустой или повреждён
    }

    return *(__IO uint32_t*)addr;
}
