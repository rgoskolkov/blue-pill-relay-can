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

void print_hard_fault_details(void) {
        uint32_t *stack_pointer;
    
    // Получаем указатель стека
    __asm volatile (
        "tst lr, #4\n"          // Проверяем какой стек использовался
        "ite eq\n"               // if-then-else
        "mrseq r0, msp\n"       // main stack pointer
        "mrsne r0, psp\n"       // process stack pointer  
        "mov %0, r0\n"          // сохраняем в stack_pointer
        : "=r" (stack_pointer)
        : 
        : "r0"
    );
    
    // Регистры из стека
    // uint32_t r0 = stack_pointer[0];
    // uint32_t r1 = stack_pointer[1];
    // uint32_t r2 = stack_pointer[2]; 
    // uint32_t r3 = stack_pointer[3];
    // uint32_t r12 = stack_pointer[4];
    uint32_t lr = stack_pointer[5];    // LR в момент сбоя
    uint32_t pc = stack_pointer[6];    // PC в момент сбоя! 
   // uint32_t psr = stack_pointer[7];
    
    // Регистры fault
    uint32_t cfsr = (*((volatile uint32_t *)(0xE000ED28))); // Configurable Fault Status
    uint32_t mmfar = (*((volatile uint32_t *)(0xE000ED34))); // MemManage Fault Address
//    uint32_t bfar = (*((volatile uint32_t *)(0xE000ED38))); // BusFault Address
    
    printf("\r\n=== HARD FAULT ===\r\n");
    
    printf("PC: 0x%08lX\r\n", pc);
    
    printf("LR: 0x%08lX\r\n", lr);
    
    printf("CFSR: 0x%08lX\r\n", cfsr);
    
    // Анализ причин
    if (cfsr & (1 << 0)) {
        printf("Reason: Instruction access violation\r\n");
    }
    if (cfsr & (1 << 1)) {
        printf("Reason: Data access violation\r\n");
    }
    if (cfsr & (1 << 16)) {
        printf("Reason: Invalid state (probably FPU)\r\n");
    }
    if (cfsr & (1 << 17)) {
        printf("Reason: Invalid PC load\r\n");
    }
    
    printf("Fault address: 0x%08lX\r\n", mmfar);
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