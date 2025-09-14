# Как перенести файлы из STM32CubeMX в этот PlatformIO-проект

Кратко: сгенерировать проект в CubeMX, затем скопировать указанные файлы в корень этого проекта. Проект предполагает, что вы используете MCU STM32F103Cx (BluePill) и настраиваете USART1 и GPIO через CubeMX.

## Обязательные настройки в CubeMX
- MCU: STM32F103C8Tx (BluePill) или аналог
- Clock: либо HSE 8 MHz → PLL x9 → SYSCLK 72 MHz (если есть внешний кварц), либо HSI/PLL (например 48 MHz) с корректными prescaler для APB1
- Peripherals:
  - USART1 Asynchronous, PA9=TX, PA10=RX; baud (по умолчанию 19200)
  - GPIO: конфигурируйте пины реле и переключателей (PBx/PAx) как Outputs / Inputs с Pull-up
- Включите NVIC interrupt для USART1 (если используете прерывания)

## Файлы, которые нужно скопировать из сгенерированного проекта
Сгенерированный проект содержит Src/ и Inc/ — копируйте в наш проект следующие файлы:

- From CubeMX Src/ -> project/src/
  - usart.c               (MX_USART2_UART_Init, huart2)
  - gpio.c                (если сгенерирован и вы хотите использовать MX_GPIO_Init())
  - system_stm32f1xx.c    (SystemClock_Config / тактирование)
  - stm32f1xx_it.c        (обработчики прерываний)
  - sysmem.c / syscalls.c  (если есть — полезно для корректной линковки)
  - stm32f1xx_hal_msp.c   (или любое сгенерированное HAL MSP)
  - startup_*.s           (если CubeMX сгенерировал — обычно PlatformIO уже содержит нужный)
  - любые другие Src/*.c, которые CubeMX сгенерировал и которые вам нужны

- From CubeMX Inc/ -> project/include/
  - usart.h
  - main.h                (главное — тут должны быть определения PIN/PORT: RELAYx_Pin/RELAYx_GPIO_Port, SWITCHx_Pin/..)
  - stm32f1xx_it.h
  - stm32f1xx_hal_conf.h  (обычно уже есть)
  - любые Inc/*.h, сгенерированные CubeMX, которые используются в Src/*.c

- Middleware (опционально)
  - если CubeMX добавил Middlewares/FreeModbus — скопируйте папку в project/lib/ или project/src/middleware/ и сообщите мне, я помогу подключить.
  - если нет, можно интегрировать стороннюю библиотеку (например microtbx-modbus) позже.

## Команды (пример) — выполните в каталоге сгенерированного проекта
Замените пути под себя.

```bash
# из каталога сгенерированного CubeMX проекта
cd /path/to/CubeMXProject

# скопировать src -> проект
cp Src/usart.c   /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus/src/
cp Src/system_stm32f1xx.c /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus/src/
cp Src/stm32f1xx_it.c     /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus/src/
cp Src/sysmem.c           /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus/src/ || true
cp Src/syscalls.c         /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus/src/ || true
# если есть:
cp Src/gpio.c /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus/src/ || true

# и include
cp Inc/usart.h /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus/include/
cp Inc/main.h  /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus/include/
cp Inc/stm32f1xx_it.h /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus/include/ || true
```

## Удалить/переименовать конфликтующие заглушки (если они есть)
В проекте могут быть временные stub-файлы — удалите их или переместите в backup:

```bash
cd /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus
mv src/usart_stub.c src/usart_stub.c.bak 2>/dev/null || true
mv src/gpio.c       src/gpio.c.bak 2>/dev/null || true
mv src/gpio\ 2.c    src/gpio.c.bak 2>/dev/null || true
```

## Настройка параметров сборки (опционально)
Если хотите переопределять параметры (NUM_SWITCHES, MODBUS_BAUDRATE, MODBUS_SLAVE_ID) через PlatformIO, добавьте в `platformio.ini`:

```ini
build_flags = 
  -DNUM_SWITCHES=8
  -DMODBUS_BAUDRATE=19200
  -DMODBUS_SLAVE_ID=1
```

## После копирования
1. Запуск сборки:
   - cd /Users/romanoskolkov/Desktop/cube/plue-pill-relay-modbus
   - pio run
2. Если сборка падает с undefined reference — пришлите вывод `pio run -v` и я подгоню недостающие символы (обычно huart2, MX_USART2_UART_Init, SystemClock_Config, MX_GPIO_Init и т.п.)

## Примечание про Modbus-стек
- Если CubeMX сгенерировал FreeModbus — хорошо, я помогу подключить его.
- Если FreeModbus нет и вы хотите использовать внешнюю библиотеку (например microtbx-modbus), мы интегрируем её после того, как HAL/USART/GPIO готовы.