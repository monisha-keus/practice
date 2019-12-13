

#include "keus_uart.h"

uint8 data_length = 0,cmd_id = 0,uart_cmd_end = 0,uart_data_ptr = 0,rcv_ptr = 0,task_type =0,command_byte=0;
uint8 recv_buff[20] = {0};  //buffer to hold complete data comming through uart
uint8 data_buff[20] = {0};  //buffer to hold only data comming through uart
uint16 event_t = 0x00;

/*************************************************************************
 * @brief   Callback function for Uart
 * @param
 * @return
 * ***********************************************************************/
void uartRxCb( uint8 port, uint8 event ) {
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
    

    // else if( uart_cmd_started == 2){


    //   uart_cmd_started=3;
    //   uart_data_ptr =0;
    //   cmd_id = ch;
    //   if(cmd_id == 0x01){//Execute switch state
    //     command_byte = 0x01;
    //   }
    //   if(cmd_id == 0x02){//configure switch
    //     command_byte = 0x02;
    //   }
    //   if(cmd_id == 0x03){//get switch state
    //     command_byte = 0x04;
    //   }
    // }
    // else if((uart_data_ptr+2) >= data_length && ch == UARE_TERNINATOR){
    //   event_t = (event_t | command_byte);
    //   command_byte = 0; 
    //   uart_cmd_end = 1;
    //   uart_cmd_started = 0;
    //   uart_data_ptr =0;
    //   rcv_ptr = 0;
    // }
    // else if((uart_data_ptr < data_length) && (command_byte != 0) && (ch != UARE_TERNINATOR)){ 

    //   data_buff[uart_data_ptr] = ch;
    //   uart_data_ptr++;
    // }
   
    // // else if(uart_cmd_end == 1){
    // //   event_t = (event_t | command_byte);
    // //   command_byte = 0; 
    // // }
    // // else if(uart_data_ptr > data_length){
    // //   uart_data_ptr = 0; 
    // //   rcv_ptr = 0;
    // // }
    // else{
    //   uart_data_ptr = 0; 
    //   rcv_ptr = 0;
    // }
  }
}

/*
* Uart initialization
*/
void initUart(void) {
 halUARTCfg_t uartConfig;
 uartConfig.configured           = TRUE;
 uartConfig.baudRate             = HAL_UART_BR_115200;
 uartConfig.flowControl          = FALSE;
 uartConfig.flowControlThreshold = 48;
 uartConfig.rx.maxBufSize        = 128;
 uartConfig.tx.maxBufSize        = 128;
 uartConfig.idleTimeout          = 6;  
 uartConfig.intEnable            = TRUE;
 uartConfig.callBackFunc         = uartRxCb;
 HalUARTOpen (HAL_UART_PORT_0, &uartConfig);
}
 