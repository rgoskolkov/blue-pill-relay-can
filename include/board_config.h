/*
  board_config.h
   Можно переопределить через build_flags (-D...) в platformio.ini
*/

#ifndef MODBUS_UART_STOPBITS
#define MODBUS_UART_STOPBITS 1
#endif

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "main.h"
#include <stdint.h>

/* Параметры по умолчанию (можно переопределить через -D в platformio.ini) */
#ifndef NUM_SWITCHES
#define NUM_SWITCHES 8U
#endif

#ifndef MODBUS_BAUDRATE
#define MODBUS_BAUDRATE 19200U
#endif

#ifndef MODBUS_SLAVE_ID
#define MODBUS_SLAVE_ID 1U
#endif

/* MODBUS_UART_PARITY: 0 = NONE, 1 = ODD, 2 = EVEN
   По умолчанию — NONE*/
#ifndef MODBUS_UART_PARITY
#define MODBUS_UART_PARITY 0
#endif

#ifndef UART1_DEBUG
#define UART1_DEBUG 0
#endif

#ifndef MONITOR_TASK
#define MONITOR_TASK 0
#endif

/* DEBOUNCE_MS — интервал дебаунса для механических переключателей в миллисекундах.
   Назначение: фильтрация механического дребезга контактов перед генерацией события.
   Как задать:
     - глобально в проекте: добавить в platformio.ini в секцию env:
         build_flags = -DDEBOUNCE_MS=80
     - или прямо в этом файле заменить значение ниже.

   По умолчанию установлен 50 ms — разумное значение для большинства кнопок.
*/
#ifndef DEBOUNCE_MS
#define DEBOUNCE_MS 300U
#endif

/* Relay pins (берутся напрямую из main.h) */
#define RELAY1_PIN RELAY1_Pin
#define RELAY1_PORT RELAY1_GPIO_Port
#define RELAY2_PIN RELAY2_Pin
#define RELAY2_PORT RELAY2_GPIO_Port
#define RELAY3_PIN RELAY3_Pin
#define RELAY3_PORT RELAY3_GPIO_Port
#define RELAY4_PIN RELAY4_Pin
#define RELAY4_PORT RELAY4_GPIO_Port
#define RELAY5_PIN RELAY5_Pin
#define RELAY5_PORT RELAY5_GPIO_Port
#define RELAY6_PIN RELAY6_Pin
#define RELAY6_PORT RELAY6_GPIO_Port
#define RELAY7_PIN RELAY7_Pin
#define RELAY7_PORT RELAY7_GPIO_Port
#define RELAY8_PIN RELAY8_Pin
#define RELAY8_PORT RELAY8_GPIO_Port

/* Switch pins (берутся напрямую из main.h) */
#define SWITCH1_PIN SWITCH1_Pin
#define SWITCH1_PORT SWITCH1_GPIO_Port
#define SWITCH2_PIN SWITCH2_Pin
#define SWITCH2_PORT SWITCH2_GPIO_Port
#define SWITCH3_PIN SWITCH3_Pin
#define SWITCH3_PORT SWITCH3_GPIO_Port
#define SWITCH4_PIN SWITCH4_Pin
#define SWITCH4_PORT SWITCH4_GPIO_Port
#define SWITCH5_PIN SWITCH5_Pin
#define SWITCH5_PORT SWITCH5_GPIO_Port
#define SWITCH6_PIN SWITCH6_Pin
#define SWITCH6_PORT SWITCH6_GPIO_Port
#define SWITCH7_PIN SWITCH7_Pin
#define SWITCH7_PORT SWITCH7_GPIO_Port
#define SWITCH8_PIN SWITCH8_Pin
#define SWITCH8_PORT SWITCH8_GPIO_Port

// Hardware Porting Layer: Pin Mappings
extern const GPIO_TypeDef* const relay_ports[NUM_SWITCHES];
extern const uint16_t relay_pins[NUM_SWITCHES];
extern const GPIO_TypeDef* const switch_ports[NUM_SWITCHES];
extern const uint16_t switch_pins[NUM_SWITCHES];

/* =================================================================================
   Hardware Porting Layer: Function Prototypes
==================================================================================*/

// --- System Tick ---
uint32_t Board_GetTick(void);
void Board_Delay(uint32_t Delay);

// --- GPIO ---
void Board_GPIO_Write(void *gpio_port, uint16_t gpio_pin, uint8_t state);
uint8_t Board_GPIO_Read(void *gpio_port, uint16_t gpio_pin);

// --- Wrappers for specific hardware ---
void Board_LED_On(void);
void Board_LED_Off(void);
void Board_Relay_On(uint8_t relay_number);
void Board_Relay_Off(uint8_t relay_number);
uint8_t Board_Switch_Read(uint8_t switch_number);


#endif /* BOARD_CONFIG_H */