#include "input_driver.h"
#include "can_adapter.h"
#include "relay_driver.h"
extern UART_HandleTypeDef huart1;

void application_init(void)
{
    can_adapter_init();
    Input_Init();
    relay_init();
}

// Кольцевой буфер для логирования в прерываниях
#define LOG_BUFFER_SIZE 2048
static char log_buffer[LOG_BUFFER_SIZE];
static volatile uint16_t log_head = 0;
static volatile uint16_t log_tail = 0;
static volatile uint8_t tx_in_progress = 0;
static volatile uint8_t overflow_pending = 0;

// Атомарная запись в буфер
static void buffer_put_char(char c) {
    uint16_t next = (log_head + 1) % LOG_BUFFER_SIZE;
    if (next == log_tail) {
        overflow_pending = 1; // отметить что было переполнение
        return;
    }
    log_buffer[log_head] = c;
    log_head = next;
}

// Запись строки
static void buffer_puts(const char *s, int len) {
    for (int i = 0; i < len; i++) {
        buffer_put_char(s[i]);
    }
}

// Атомарное чтение из буфера
static int buffer_get_char(char *c) {
    if (log_tail != log_head) {
        *c = log_buffer[log_tail];
        log_tail = (log_tail + 1) % LOG_BUFFER_SIZE;
        return 1;
    }
    return 0;
}

// Запуск передачи из буфера через DMA
static void flush_log_buffer(void) {
    if (tx_in_progress || log_tail == log_head) {
        return;
    }
    
    static char tx_buf[LOG_BUFFER_SIZE + 5]; // +5 для ...\r\n
    uint16_t count = 0;
    char c;
    
    while (buffer_get_char(&c) && count < LOG_BUFFER_SIZE) {
        tx_buf[count++] = c;
    }
    
    // Если было переполнение - добавим ... в конец
    if (overflow_pending) {
        overflow_pending = 0;
        tx_buf[count++] = '.';
        tx_buf[count++] = '.';
        tx_buf[count++] = '.';
        tx_buf[count++] = '\r';
        tx_buf[count++] = '\n'; 
    }
    
    if (count > 0) {
        tx_in_progress = 1;
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)tx_buf, count);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        tx_in_progress = 0;
        flush_log_buffer();
    }
}

int _write(int file, char *ptr, int len) {
    buffer_puts(ptr, len);
    flush_log_buffer();
    return len;
}