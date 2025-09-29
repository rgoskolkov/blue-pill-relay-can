#include "main.h"
#include <stdint.h>

volatile uint32_t g_assert_pc = 0;

void configASSERT_Handler(uint32_t pc) {
    g_assert_pc = pc;
    // Blink a distinct pattern: rapid triple blink, then pause
    // Use HAL functions; this runs with interrupts disabled so be conservative.
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
