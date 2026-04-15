/**
  ******************************************************************************
  * @file           : usb_cdc_monitor.h
  * @brief          : USB CDC monitoring and logging header
  ******************************************************************************
  */

#ifndef USB_CDC_MONITOR_H
#define USB_CDC_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Function prototypes */
void USB_CDC_Log_Init(void);
void USB_CDC_SendLog(const char* format, ...);
void USB_CDC_LogTask(void* argument);
void USB_CDC_TransmitComplete(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_MONITOR_H */
