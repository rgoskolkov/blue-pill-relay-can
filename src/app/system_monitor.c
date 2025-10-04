#include "system_monitor.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Modbus.h"
#include <string.h>
#include <stdio.h>

// Эти переменные должны быть доступны из других модулей
extern modbusHandler_t mHandler;
extern volatile uint32_t usart3_irq_count;
uint32_t last_irq_count = 0;

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

    if (usart3_irq_count != last_irq_count) {
      written = snprintf(p, len, "Diag: IRQ count: %lu, last error: %d, state: %d\r\n", usart3_irq_count, mHandler.i8lastError, mHandler.i8state);
      if (written > 0) {
        p += written;
        len -= written;
      }
      last_irq_count = usart3_irq_count;
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