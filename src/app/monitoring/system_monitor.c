#include "system_monitor.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Modbus.h"
#include <string.h>
#include <stdio.h>

// Эти переменные должны быть доступны из других модулей
extern modbusHandler_t mHandler;

const char* taskStateToString(eTaskState state) {
    switch(state) {
        case eRunning:   return "Running";
        case eReady:     return "Ready";
        case eBlocked:   return "Blocked";
        case eSuspended: return "Suspended";
        case eDeleted:   return "Deleted";
        case eInvalid:   return "Invalid";
        default:         return "Unknown";
    }
}

void print_fault_details(void) {
  // Check for and report a previous HardFault
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_BKP_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  if (BKP->DR1 == 0xDEAD) {
      uint32_t high = BKP->DR2;
      uint32_t low = BKP->DR3;
      uint32_t fault_addr = (high << 16) | low;
      //todo uncomment after testing
      //BKP->DR1 = 0; // Clear the magic number

      printf("\r\n--- PREVIOUS SESSION CRASHED ---\r\n");
      printf("HardFault at PC: 0x%08lX\r\n", fault_addr);
      printf("--------------------------------\r\n");
      HAL_Delay(100); // Give time for the message to be sent
  }

  if (BKP->DR7 == 0xBEEF) {
      uint32_t high = BKP->DR8;
      uint32_t low = BKP->DR9;
      uint32_t fault_addr = (high << 16) | low;
      BKP->DR7 = 0; // Clear the magic number

      printf("\r\n--- PREVIOUS SESSION FAILED ---\r\n");
      printf("MemManage fault at: 0x%08lX\r\n", fault_addr);
      printf("--------------------------------\r\n");
      HAL_Delay(100);
  }

  if (BKP->DR4 == 0xAAAA) {
      uint32_t high = BKP->DR5;
      uint32_t low = BKP->DR6;
      uint32_t assert_addr = (high << 16) | low;
      BKP->DR4 = 0; // Clear the magic number

      printf("\r\n--- PREVIOUS SESSION FAILED ---\r\n");
      printf("configASSERT at: 0x%08lX\r\n", assert_addr);
      printf("--------------------------------\r\n");
      HAL_Delay(100);
  }
  HAL_PWR_DisableBkUpAccess();
}

void system_monitor(void) {
    static char buffer[1024]; // Статический буфер для сборки строки
    char *p = buffer;
    int len = sizeof(buffer);
    int written;

    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    
    // Очищаем буфер
    memset(buffer, 0, sizeof(buffer));

    // Заголовок
    written = snprintf(p, len, "--- System Monitor ---\r\n");
    p += written;
    len -= written;

    if (task_count <= 6) {
        unsigned long _total_runtime;
        TaskStatus_t _task_status_array[task_count];
        task_count = uxTaskGetSystemState(_task_status_array, task_count, &_total_runtime);

        for (int i = 0; i < task_count; i++) {
            written = snprintf(p, len, "[TASK] %-20s: %-9s, Prio:%lu, StackHWM:%6u, Runtime: %lu\r\n",
                               _task_status_array[i].pcTaskName,
                               taskStateToString(_task_status_array[i].eCurrentState),
                               _task_status_array[i].uxCurrentPriority,
                               _task_status_array[i].usStackHighWaterMark,
                               _task_status_array[i].ulRunTimeCounter);
            if (written > 0) {
                p += written;
                len -= written;
            }
        }
    }

    // Информация о куче
    written = snprintf(p, len, "[HEAP] Current Free: %u, Minimal Ever Free: %u\r\n",
                       xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    if (written > 0) {
        p += written;
        len -= written;
    }

    printf("%s", buffer);
}

void system_monitor_task(void *argument)
{
    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
        system_monitor();
    }
}