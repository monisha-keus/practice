

#include "keus_uart.h"

uint8 data_length = 0,cmd_id = 0,uart_cmd_end = 0,uart_data_ptr = 0,rcv_ptr = 0,task_type =0,command_byte=0;
uint8 recv_buff[20] = {0};  //buffer to hold complete data comming through uart
uint8 data_buff[20] = {0};  //buffer to hold only data comming through uart
uint16 event_t = 0x00;

/*************************************************************************
 * @brief   Callback function for Uart0
 * @param
 * @return
 * ***********************************************************************/
void uart0RxCb( uint8 port, uint8 event ) {
  uint8  ch,uart_cmd_started=0;
  while (Hal_UART_RxBufLen(port))
  {
    // Read one byte from UART to ch
    HalUARTRead (port, &ch, 1);
    //check for the start byte
    if (ch == UART_INITIATOR && uart_cmd_started == 0) {
      uart_cmd_started = 1;
      uart_data_ptr=0;
    }
    else if(/*ch != UART_INITIATOR && */uart_cmd_started == 1){
      data_length = ch;
      uart_cmd_started = 2;
      uart_data_ptr = 0; 
    }
    else if(uart_cmd_started == 2  && (uart_data_ptr == data_length) && ch == UART_TERNINATOR){ 
      uart_cmd_end = 1;
      uart_cmd_started = 0;
      uart_data_ptr =0;

    }
    else if(uart_cmd_started == 2 && (uart_data_ptr+1) > data_length ){
      uart_cmd_started = 0;
      uart_data_ptr =0;
    }
    else if(uart_cmd_started == 2)
    {
      data_buff[uart_data_ptr++] = ch;
    }
    else
    {
      uart_cmd_started = 0;
      uart_data_ptr =0;
    }
    if(uart_cmd_end){
      cmd_id = data_buff[0];
      switch (cmd_id){
        case 0x01:
          event_t = 0x01;
          break;
        case 0x02:
          event_t = 0x02;
          break;
        case 0x03:
          event_t = 0x04;
          break;
        case 0x04:
          event_t = 0x08;
          break;
        default:
        break;
      

      }
    }
    
  }
}

/*
* Uart0 initialization
*/
void initUart0(void) {
 halUARTCfg_t uart0Config;
 uart0Config.configured           = TRUE;
 uart0Config.baudRate             = HAL_UART_BR_115200;
 uart0Config.flowControl          = FALSE;
 uart0Config.flowControlThreshold = 48;
 uart0Config.rx.maxBufSize        = 128;
 uart0Config.tx.maxBufSize        = 128;
 uart0Config.idleTimeout          = 6;  
 uart0Config.intEnable            = TRUE;
 uart0Config.callBackFunc         = uart0RxCb;
 HalUARTOpen (HAL_UART_PORT_0, &uart0Config);
}
 