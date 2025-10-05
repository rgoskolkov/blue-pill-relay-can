#include "main.h"
#include <stdint.h>
#include <stdio.h>

void configASSERT_Handler(uint32_t pc) {
    printf("configASSERT_Handler at PC: %08lX\n", pc);
    for (;;) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");

        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");

        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

        for (volatile int i = 0; i < 800000; ++i) __asm volatile("nop");
    }
}
