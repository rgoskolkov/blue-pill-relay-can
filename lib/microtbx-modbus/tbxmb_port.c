#include "microtbx.h"
#include "microtbxmodbus.h"
#include "stm32f1xx_hal.h"

// --- Внешние переменные ---
extern TIM_HandleTypeDef htim1;
extern UART_HandleTypeDef huart3;

// --- Локальные переменные ---
static volatile struct
{
  uint8_t  const * data;
  uint16_t         nextIdx;
  uint16_t         totalLen;
} transmitInfo;


/************************************************************************************//**
** \brief     Инициализация порта UART.
****************************************************************************************/
void TbxMbPortUartInit(tTbxMbUartPort port,
                       tTbxMbUartBaudrate baudrate,
                       tTbxMbUartDatabits databits,
                       tTbxMbUartStopbits stopbits,
                       tTbxMbUartParity parity)
{
  // Включаем прерывание по приему (RXNE)
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);
}

/************************************************************************************//**
** \brief     Запуск передачи данных (адаптировано из примера).
****************************************************************************************/
uint8_t TbxMbPortUartTransmit(tTbxMbUartPort port,
                              uint8_t const *data,
                              uint16_t len)
{
  transmitInfo.data = data;
  transmitInfo.totalLen = len;
  transmitInfo.nextIdx = 1U;

  // Запускаем передачу первого байта напрямую в регистр
  huart3.Instance->DR = (transmitInfo.data[0] & (uint8_t)0x01FF);

  if (len > 1) {
    // Включаем прерывание "регистр передатчика пуст" для остальных байтов
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_TXE);
  } else {
    // Если байт один, сразу ждем завершения передачи
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_TC);
  }
  return TBX_OK;
}

/************************************************************************************//**
** \brief     Получение значения счетчика для таймаутов RTU.
****************************************************************************************/
uint16_t TbxMbPortTimerCount(void)
{
  return (uint16_t)__HAL_TIM_GET_COUNTER(&htim1);
}

/************************************************************************************//**
** \brief     Легковесный обработчик прерываний (адаптирован из примера).
****************************************************************************************/
void TbxMbPortUartInterrupt(void)
{
  // --- Обработка приема ---
  if(__HAL_UART_GET_IT_SOURCE(&huart3, UART_IT_RXNE) != RESET && __HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE) != RESET)
  {
    uint8_t rxByte = (uint8_t)(huart3.Instance->DR & (uint8_t)0x00FF);
    TbxMbUartDataReceived(TBX_MB_UART_PORT1, &rxByte, 1);
  }

  // --- Обработка передачи ---
  if(__HAL_UART_GET_IT_SOURCE(&huart3, UART_IT_TXE) != RESET && __HAL_UART_GET_FLAG(&huart3, UART_FLAG_TXE) != RESET)
  {
    if (transmitInfo.nextIdx < transmitInfo.totalLen)
    {
      huart3.Instance->DR = (transmitInfo.data[transmitInfo.nextIdx] & (uint8_t)0x01FF);
      transmitInfo.nextIdx++;
    }
    else
    {
      __HAL_UART_DISABLE_IT(&huart3, UART_IT_TXE);
      __HAL_UART_ENABLE_IT(&huart3, UART_IT_TC);
    }
  }

  // --- Обработка завершения передачи ---
  if(__HAL_UART_GET_IT_SOURCE(&huart3, UART_IT_TC) != RESET && __HAL_UART_GET_FLAG(&huart3, UART_FLAG_TC) != RESET)
  {
    __HAL_UART_CLEAR_FLAG(&huart3, UART_FLAG_TC); // Очищаем флаг
    __HAL_UART_DISABLE_IT(&huart3, UART_IT_TC);
    TbxMbUartTransmitComplete(TBX_MB_UART_PORT1);
  }
}
