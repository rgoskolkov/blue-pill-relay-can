#include "system_monitor.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"

// Символы из linker-скрипта для анализа памяти
extern uint32_t _sdata; // Начало .data
extern uint32_t _edata; // Конец .data
extern uint32_t _sbss;  // Начало .bss
extern uint32_t _ebss;  // Конец .bss
extern uint32_t _estack; // Конец RAM

// Доступ к атрибутам задач для получения размеров стека
extern const osThreadAttr_t ledTask_attributes;
extern const osThreadAttr_t monitorTask_attributes;
extern const osThreadAttr_t inputTask_attributes;

// Вспомогательная функция для получения общего размера стека задачи по ее имени
uint32_t get_task_stack_size(const char* task_name) {
    if (strcmp(task_name, ledTask_attributes.name) == 0) {
        return ledTask_attributes.stack_size;
    }
    if (strcmp(task_name, monitorTask_attributes.name) == 0) {
        return monitorTask_attributes.stack_size;
    }
    if (strcmp(task_name, inputTask_attributes.name) == 0) {
        return inputTask_attributes.stack_size;
    }
    return 0; // Задача не найдена
}

const char* taskStateToString(eTaskState state) {
    switch(state) {
        case eRunning:   return "Run";
        case eReady:     return "Ready";
        case eBlocked:   return "Blc";
        case eSuspended: return "Spd";
        case eDeleted:   return "Del";
        case eInvalid:   return "Inv";
        default:         return "Uwn";
    }
}

void print_fault_details(void) {
  // Check for and report a previous HardFault
  printf("\r\n--- APPLICATION STARTUP ---\r\n");
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_BKP_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  if (BKP->DR1 == 0xDEAD) {
      uint32_t high = BKP->DR2;
      uint32_t low = BKP->DR3;
      uint32_t fault_addr = (high << 16) | low;
      //todo uncomment after testing
      BKP->DR1 = 0; // Clear the magic number

      printf("\r\n--- PREVIOUS SESSION CRASHED ---\r\n");
      printf("HardFault at PC: 0x%08lX\r\n", fault_addr);
      printf("--------------------------------\r\n");
  }

  if (BKP->DR7 == 0xBEEF) {
      uint32_t high = BKP->DR8;
      uint32_t low = BKP->DR9;
      uint32_t fault_addr = (high << 16) | low;
      //todo uncomment after testing
      //BKP->DR7 = 0; // Clear the magic number

      printf("\r\n--- PREVIOUS SESSION FAILED ---\r\n");
      printf("MemManage fault at: 0x%08lX\r\n", fault_addr);
      printf("--------------------------------\r\n");
  }

  if (BKP->DR4 == 0xAAAA) {
      uint32_t high = BKP->DR5;
      uint32_t low = BKP->DR6;
      uint32_t assert_addr = (high << 16) | low;
      //todo uncomment after testing
      BKP->DR4 = 0; // Clear the magic number

      printf("\r\n--- PREVIOUS SESSION FAILED ---\r\n");
      printf("configASSERT at: 0x%08lX\r\n", assert_addr);
      printf("--------------------------------\r\n");
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

    if (task_count <= 10) {
        unsigned long _total_runtime;
        TaskStatus_t _task_status_array[task_count];
        task_count = uxTaskGetSystemState(_task_status_array, task_count, &_total_runtime);

        written = snprintf(p, len, "Name\t\tState\tPrio\tStackLeft\tTotal\tUsed%%\r\n");
        if (written > 0) { p += written; len -= written; }
        written = snprintf(p, len, "--------------------------------------------------------------------------\r\n");
        if (written > 0) { p += written; len -= written; }


        for (int i = 0; i < task_count; i++) {
            uint32_t total_stack = get_task_stack_size(_task_status_array[i].pcTaskName);
            uint32_t hwm_bytes = _task_status_array[i].usStackHighWaterMark * sizeof(StackType_t);
            uint32_t used_percent = 0;
            if (total_stack > 0) {
                used_percent = ((total_stack - hwm_bytes) * 100) / total_stack;
            }

            written = snprintf(p, len, "%-10s\t%-3s\t%lu\t%lu\t\t%lu\t%lu%%\r\n",
                               _task_status_array[i].pcTaskName,
                               taskStateToString(_task_status_array[i].eCurrentState),
                               _task_status_array[i].uxCurrentPriority,
                               hwm_bytes,
                               total_stack,
                               used_percent);
            if (written > 0) {
                p += written;
                len -= written;
            }
        }
    }

    // Информация о куче
    written = snprintf(p, len, "[HEAP] Total: %u, Current Free: %u, Minimal Ever Free: %u\r\n",
                       (unsigned int)configTOTAL_HEAP_SIZE,
                       (unsigned int)xPortGetFreeHeapSize(),
                       (unsigned int)xPortGetMinimumEverFreeHeapSize());
    if (written > 0) {
        p += written;
        len -= written;
    }

    // Общая информация о RAM
    uint32_t total_ram = (uint32_t)&_estack - 0x20000000;
    uint32_t static_plus_bss = (uint32_t)&_ebss - (uint32_t)&_sdata;
    uint32_t heap_size = (unsigned int)configTOTAL_HEAP_SIZE;
    uint32_t static_only = static_plus_bss - heap_size;
    uint32_t free_for_stack = total_ram - static_plus_bss;

    written = snprintf(p, len, "[RAM] Total: %lu | Static: %lu | Heap: %lu | Free for Stacks: %lu\r\n",
                       total_ram,
                       static_only,
                       heap_size,
                       free_for_stack);
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