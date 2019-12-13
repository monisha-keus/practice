#ifndef KEUS_UART_H_INCLUDED
#define KEUS_UART_H_INCLUDED

#include "keus_gpio_util.h"
#include "hal_uart.h"
#include "OSAL.h"

#define MAX_UART_BUF_LEN    20

#define UART_INITIATOR  0xAA
#define UART_TERNINATOR 0xFF

void uartRxCb( uint8 port, uint8 event );
void initUart(void);

#endif  //KEUS_UART_H_INCLUDED
