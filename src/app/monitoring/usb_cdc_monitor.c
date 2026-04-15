/**
  ******************************************************************************
  * @file           : usb_cdc_monitor.c
  * @brief          : USB CDC monitoring and logging task for FreeRTOS
  ******************************************************************************
  */

#include "main.h"
#include "cmsis_os2.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Private variables ---------------------------------------------------------*/
extern USBD_HandleTypeDef hUsbDeviceFS;

/* USB CDC logging queue */
static osMessageQueueId_t usb_log_queue_handle = NULL;
#define USB_LOG_QUEUE_SIZE    16
#define USB_LOG_MSG_SIZE      128

typedef struct {
    char data[USB_LOG_MSG_SIZE];
    uint16_t len;
} usb_log_msg_t;

/* Buffer for USB transmission */
static uint8_t tx_buffer[USB_LOG_MSG_SIZE];
static volatile uint8_t usb_tx_busy = 0;

/*
 * @brief USB CDC transmission complete callback
 */
void USB_CDC_TransmitComplete(void)
{
    usb_tx_busy = 0;
}

/*
 * @brief Initialize USB CDC logging
 */
void USB_CDC_Log_Init(void)
{
    usb_log_queue_handle = osMessageQueueNew(USB_LOG_QUEUE_SIZE, sizeof(usb_log_msg_t), NULL);
    usb_tx_busy = 0;
}

/*
 * @brief Send log message via USB CDC
 */
void USB_CDC_SendLog(const char* format, ...)
{
    if (usb_log_queue_handle == NULL) return;
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return;
    
    usb_log_msg_t msg;
    va_list args;
    va_start(args, format);
    msg.len = vsnprintf(msg.data, USB_LOG_MSG_SIZE, format, args);
    va_end(args);
    
    if (msg.len > 0) {
        /* Non-blocking send to queue - don't wait if queue is full */
        osStatus_t status = osMessageQueuePut(usb_log_queue_handle, &msg, 0, 0);
        if (status != osOK) {
            /* Queue full, message dropped */
        }
    }
}

/*
 * @brief USB CDC logging task (run in FreeRTOS)
 */
void USB_CDC_LogTask(void* argument)
{
    usb_log_msg_t msg;
    
    for (;;) {
        /* Wait for message from queue */
        osStatus_t status = osMessageQueueGet(usb_log_queue_handle, &msg, NULL, osWaitForever);
        
        if (status == osOK) {
            /* Wait until USB is not busy */
            while (usb_tx_busy) {
                osDelay(1);
            }
            
            /* Check USB state */
            if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
                usb_tx_busy = 1;
                memcpy(tx_buffer, msg.data, msg.len);
                
                uint8_t result = CDC_Transmit_FS(tx_buffer, msg.len);
                if (result != USBD_OK) {
                    usb_tx_busy = 0;
                }
            }
        }
    }
}
