#include "heartbeat.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "can_adapter.h"
#include "board_config.h"

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

void system_monitor(void) {
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    // Заголовок
    printf("- SysState -\r\n Stack\r\n Name\t\tState\tPrio\tLeft\tTotal\tUse%%\r\n");
    if (task_count <= 10) {
        unsigned long _total_runtime;
        TaskStatus_t _task_status_array[task_count];
        task_count = uxTaskGetSystemState(_task_status_array, task_count, &_total_runtime);

        for (int i = 0; i < task_count; i++) {
            uint32_t total_stack = get_task_stack_size(_task_status_array[i].pcTaskName);
            uint32_t hwm_bytes = _task_status_array[i].usStackHighWaterMark * sizeof(StackType_t);
            uint32_t used_percent = 0;
            if (total_stack > 0) {
                used_percent = ((total_stack - hwm_bytes) * 100) / total_stack;
            }

            printf("%-10s\t%u\t%lu\t%lu\t%lu\t%lu%%\r\n",
                               _task_status_array[i].pcTaskName,
                               _task_status_array[i].eCurrentState,
                               _task_status_array[i].uxCurrentPriority,
                               hwm_bytes,
                               total_stack,
                               used_percent);
        }
    }

    // Информация о куче
    printf("[HEAP] Total: %u, CurrFree: %u, MinFree: %u\r\n",
                       (unsigned int)configTOTAL_HEAP_SIZE,
                       (unsigned int)xPortGetFreeHeapSize(),
                       (unsigned int)xPortGetMinimumEverFreeHeapSize());

    // Общая информация о RAM
    uint32_t total_ram = (uint32_t)&_estack - 0x20000000;
    uint32_t static_plus_bss = (uint32_t)&_ebss - (uint32_t)&_sdata;
    uint32_t heap_size = (unsigned int)configTOTAL_HEAP_SIZE;
    uint32_t static_only = static_plus_bss - heap_size;
    uint32_t free_for_stack = total_ram - static_plus_bss;

    printf("[RAM] Tot: %lu | Stat: %lu | Free: %lu\r\n",
                       total_ram,
                       static_only,
                       free_for_stack);
}

void heartbeat_task(void *argument)
{
    printf("heartbeat_task started\r\n");
    can_send_heartbeat();
    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_TIME_MS));
        #if MONITOR_TASK == 1
            system_monitor();
        #endif
        can_send_heartbeat();
    }
}