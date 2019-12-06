/**************************************************************************************************
 Filename:       ZMain.c
 Revised:        $Date: 2010-09-17 16:25:30 -0700 (Fri, 17 Sep 2010) $
 Revision:       $Revision: 23835 $

 Description:    Startup and shutdown code for ZStack
 Notes:          This version targets the Chipcon CC2530


 Copyright 2005-2010 Texas Instruments Incorporated. All rights reserved.

 IMPORTANT: Your use of this Software is limited to those specific rights
 granted under the terms of a software license agreement between the user
 who downloaded the software, his/her employer (which must be your employer)
 and Texas Instruments Incorporated (the "License").  You may not use this
 Software unless you agree to abide by the terms of the License. The License
 limits your use, and you acknowledge, that the Software may not be modified,
 copied or distributed unless embedded on a Texas Instruments microcontroller
 or used solely and exclusively in conjunction with a Texas Instruments radio
 frequency transceiver, which is integrated into your product.  Other than for
 the foregoing purpose, you may not use, reproduce, copy, prepare derivative
 works of, modify, distribute, perform, display or sell this Software and/or
 its documentation for any purpose.

 YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
 PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
 NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
 TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
 NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
 LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
 INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
 OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
 OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
 (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

 Should you have any questions regarding your right to use this Software,
 contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
* INCLUDES
*/

#ifndef NONWK
#include "AF.h"
#endif
#include "hal_adc.h"
#include "hal_flash.h"
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_uart.h"
#include "hal_drivers.h"
#include "OnBoard.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "ZComDef.h"
#include "ZMAC.h"
#include "keus_gpio_util.h"
#include "keus_timer_util.h"



#define ON      0x01
#define OFF     0x00
#define TOGGLE  0x02
#define BLINK   0x03

#define NVIC_MEMORY_POSITION 0x10

#define LED_CONTROL     1
#define SAVE_SCENE      2
#define EXECUTE_SCENE   3
#define DELETE_SCENE    4
#define GROUP_CONTROL   5

#define LED_OFF     GPIO_HIGH
#define LED_ON      GPIO_LOW


/*********************************************************************
* CUSTOM CODE
*/

//uint8 led1_state = 1,led2_state =1,led3_state  = 1 ;
uint8 uart_data[20] = {0};
uint8  rec_buff[10] = {0};
uint8 scene_arr[3][10];
uint8 uart_cmd_started = 0,uart_cmd_end = 0;
uint8 data_length,cmd_id,task_type,scn_no=0,first=0;

uint8 led1_prev_state = 0,led2_prev_state =0,led3_prev_state  = 0,led4_prev_state  = 0 ;
bool toggle = false,toggle1 = false,toggle3 = false;
uint8 count = 0,count1 = 0,count3 = 0, led_blink_t =0,led_no_t = 0,led_no_toggle=0,T_prev=1,timer=1,led_toggle_t=0;

//
KeusGPIOPin ledPin1 = {0, 0, GPIO_OUTPUT, false, LED_ON};
KeusGPIOPin ledPin2 = {0, 1, GPIO_OUTPUT, false, LED_ON};
KeusGPIOPin ledPin3 = {0, 4, GPIO_OUTPUT, false, LED_ON};
KeusGPIOPin ledPin4 = {0, 5, GPIO_OUTPUT, false, LED_ON};

KeusGPIOPin buttonPin1 = {1, 2, GPIO_INPUT, true, LED_ON};
KeusGPIOPin buttonPin2 = {1, 3, GPIO_INPUT, true, LED_ON};
KeusGPIOPin buttonPin3 = {1, 4, GPIO_INPUT, true, LED_ON};
KeusGPIOPin buttonPin4 = {1, 5, GPIO_INPUT, true, LED_ON};


void initUart(void);
void uartRxCb( uint8 port, uint8 event );
void KEUS_delayms(uint16 ms);
void KEUS_init(void);
void KEUS_start(void);
void KEUS_loop(void);

void decrypt_data(uint8 *uart_data);
void process_Rx_data(void);
void led_control(uint8 led_state,uint8 led_no);
void save_scene_task(void);
void execute_scene_task(void);
void uart_send_ack(uint8 Tx_buff);
void execute_scene(uint8 arr_index);
int8 search_scn_id(uint8 scn_id);
void group_control(void);
void led_control_execute(void);
void led_toggle(uint8 led_no);
void led_blink(uint8 led_no);

bool KeusThemeSwitchMiniMemoryInit(void);
bool KeusThemeSwitchMiniReadConfigDataIntoMemory(void);
bool KeusThemeSwitchMiniWriteConfigDataIntoMemory(void);

bool init_status = 0;
bool read_status = 0;
bool write_status = 0;

extern void ledTimerCbk(uint8 timerId);
void leddebounceCbk(uint8 timerId);
bool debounce = true;

//Debounce configuration
KeusTimerConfig debounceTimer = {
  &leddebounceCbk,
  200,
  true,
  -1,
  0
};

//Timer Configuration
KeusTimerConfig intervalTimer = {
  &ledTimerCbk,
  100,
  true,
  -1,
  0
};

/*******************************************************
 * @brief  Callback function for timer
*          Called every after 100ms
*
*********************************************************/
void ledTimerCbk(uint8 timerId) {
  if(toggle){//To control Blinking led rate
    count++;
  }
  if(toggle1){//To control Blink Led rate
    count1++;
  }
}

/********************************************************
* callback function for Debounce
* called every after 200ms
*********************************************************/
void leddebounceCbk(uint8 timerId) {
  debounce = true;
}

/*
* Uart initialization
*/
void initUart() {
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

/**************************************************************************
* @fn      uart_send_ack
* @brief   send Ack after sucessful receiving cmd.
* @arg     1->uint8-> cmnd_id
***************************************************************************/
void uart_send_ack(uint8 cmnd_id){
  uint8 Tx_buff[5] = {0};
  Tx_buff[0] = 0xAA;
  Tx_buff[1] = 0x01;
  Tx_buff[2] = cmnd_id;
  Tx_buff[3] = 0xFF;
  HalUARTWrite( HAL_UART_PORT_0, Tx_buff, 4);

}
/*****************************************************************************************
** @brief This function is to decrypt the received data and perform the corresponding data
*******************************************************************************************/
void decrypt_data(uint8 *uart_data)
{
  uint8 data_ptr = 0,rec_ptr = 0,length=0;
  data_length = uart_data[ data_ptr + 1 ]; 
  data_ptr = data_ptr + 2;
  cmd_id = uart_data[data_ptr];

  if(cmd_id == 0x01){//led_control
    task_type = 1;
  }
  if(cmd_id == 0x02){//save_scene
    task_type = 2;
  }
  if(cmd_id == 0x03){//execute scene
    task_type = 3;
  }

  if(cmd_id == 0x04){//delete_scene
    task_type = 4;
  }

  if(cmd_id == 0x05){//group_control
    task_type = 5;
  }
  
  data_ptr++;
  length = data_length;
  while(length > 0 ){
      rec_buff[rec_ptr] = uart_data[data_ptr];
      rec_ptr++;
      data_ptr++;
      length--;
    }
    data_ptr = 0;
    rec_ptr = 0; 

  process_Rx_data();

} 

//nvic memory init
bool KeusThemeSwitchMiniMemoryInit(void)
{
//  for (uint8 i = 0; i < CLICK_TYPES; i++)
//  {
//    themeManager.btnThemeMap[i] = 255;
//  }

 uint8 res = osal_nv_item_init(NVIC_MEMORY_POSITION, sizeof(scene_arr), (void *)scene_arr);

 if (res == SUCCESS || res == NV_ITEM_UNINIT)
 {
   return true;
 }
 else
 {
   return false;
 }
}

bool KeusThemeSwitchMiniReadConfigDataIntoMemory(void)
{
 uint8 res = osal_nv_read(NVIC_MEMORY_POSITION, 0, sizeof(scene_arr), (void *)scene_arr);

 if (res == SUCCESS)
 {
   return true;
 }
 else
 {
   return false;
 }
}

bool KeusThemeSwitchMiniWriteConfigDataIntoMemory(void)
{
 uint8 res = osal_nv_write(NVIC_MEMORY_POSITION, 0, sizeof(scene_arr), (void *)scene_arr);

 if (res == SUCCESS)
 {
   return true;
 }
 else
 {
   return false;
 }
}
/********************************************************************
* @fn     process_Rx_data
* @brief  This function is for processed the data received from uart
*********************************************************************/
void process_Rx_data(){
  switch(task_type){
    case LED_CONTROL:
    {
      led_control_execute();
      uart_send_ack(cmd_id);
      break;
    }
    
    case SAVE_SCENE:
    {
      save_scene_task();
      break;
    }
    
    case EXECUTE_SCENE:
    {
      execute_scene_task();
       break;
    }
    case DELETE_SCENE:
    {
      delete_scene();
      break;
    }
    case GROUP_CONTROL:
    {
      group_control();
      uart_send_ack(cmd_id);
      break;

    }
     
    default:
      break;

  }
}

/********************************************************************************
 * @fn      search_scn_id
 * @brief   Search for given scean id exist or not 
 ********************************************************************************/
int8 search_scn_id(uint8 scn_id){
  uint8 i =0;
  int8 arr_index=-1;
  for(i=0;i<3;i++){
    if(scene_arr[i][0] == scn_id){
      arr_index = i;
      break;
    }
  }
  return arr_index; 
}

/***************************************************************************
 * @fn      led_control
 * @brief   control led toggle based on les_state and Led_no
 **************************************************************************/ 
void led_control(uint8 led_state,uint8 led_no){
  
  switch (led_no)
  {
  case 0x01:
    if(led_state == ON){
    ledPin1.state = LED_ON;
    KeusGPIOSetPinValue(&ledPin1);
    led1_prev_state = ON;
  }
  else if(led_state == OFF) {
    ledPin1.state = LED_OFF;
    KeusGPIOSetPinValue(&ledPin1);
    led1_prev_state = OFF;
  }
    break;

    case 0x02:
    if(led_state == ON){
    ledPin2.state = LED_ON;
    KeusGPIOSetPinValue(&ledPin2);
    led2_prev_state = ON;
  }
  else if(led_state == OFF) {
    ledPin2.state = LED_OFF;
    KeusGPIOSetPinValue(&ledPin2);
    led2_prev_state = OFF;
  }
    break;

    case 0x03:
    if(led_state == ON){
    ledPin3.state = LED_ON;
    KeusGPIOSetPinValue(&ledPin3);
    led3_prev_state = ON;
  }
  else if(led_state == OFF) {
    ledPin3.state = LED_OFF;
    KeusGPIOSetPinValue(&ledPin3);
    led3_prev_state = OFF;

  }
    break;

    case 0x04:
    if(led_state == ON){
    ledPin4.state = LED_ON;
    KeusGPIOSetPinValue(&ledPin4);
    led4_prev_state = ON;

  }
  else if(led_state == OFF) {
    ledPin4.state = LED_OFF;
    KeusGPIOSetPinValue(&ledPin4);
    led4_prev_state = OFF;

  }
    break;
  
  default:
    break;
  }
  
}

/*****************************************************************
 * @fn     led_control_execute 
 * @brief   Based on Led state control Led exectuion/
 *          state: 00-> Led off
 *                 01-> Led On
 *                 02-> Blink
 *                 03-> Blinking
 ******************************************************************/ 
void led_control_execute(void){

  uint8 led_state =0,led_no = 0;
  led_no    = rec_buff[0];
  led_state = rec_buff[1];
  
  if(led_state == ON ||led_state == OFF){
    led_control(led_state,led_no);
    led_toggle_t = 0;
  }
  if(led_state == BLINK ){
    led_blink_t = 1;
    led_toggle_t = 0;
    led_no_t = led_no;
    toggle = true;    
    }
  if( led_state == TOGGLE ){
    led_toggle_t = 1;
    first =1;
    led_no_toggle = led_no;
    toggle1 = true;    
  }
  
}
/********************************************************
 * @fn      led_blink
 * @brief   Blink Led for 500ms on and 100ms off 
 *******************************************************/

void led_blink(uint8 led_no){

  if(led_no == 0x01) 
  {
    if(count >=5){ // off for 500ms
      T_prev =1;
      count = 0;
      ledPin1.state = LED_OFF;
      KeusGPIOSetPinValue(&ledPin1);
      led1_prev_state  = OFF;
    }
    else if(count >=1 && T_prev == 1){  //On for 100ms
      T_prev = 0;
      count = 0;
      ledPin1.state = LED_ON;
      KeusGPIOSetPinValue(&ledPin1);
      led1_prev_state  = ON;

    }
  }

    if(led_no == 0x02)
    {
     if(count >5){ // off for 500ms
      T_prev =1;
      count = 0;
      ledPin2.state = LED_OFF;
      KeusGPIOSetPinValue(&ledPin2);
      led2_prev_state  = OFF;
    }
    if(count >1 && T_prev == 1){  //On for 100ms
      T_prev = 0;
      count = 0;
      ledPin2.state = LED_ON;
      KeusGPIOSetPinValue(&ledPin2);
      led2_prev_state  = ON;
    }
  }
if(led_no == 0x03)    {
     if(count >5){ // off for 500ms
      T_prev =1;
      count = 0;
      ledPin3.state = LED_OFF;
      KeusGPIOSetPinValue(&ledPin3);
      led3_prev_state  = OFF;
    }
    if(count >1 && T_prev == 1){  //On for 100ms
      T_prev = 0;
      count = 0;
      ledPin3.state = LED_ON;
      KeusGPIOSetPinValue(&ledPin3);
      led3_prev_state  = ON;
    }
  }
if(led_no == 0x04)    {
     if(count >5){ // off for 500ms
      T_prev =1;
      count = 0;
      ledPin4.state = LED_OFF;
      KeusGPIOSetPinValue(&ledPin4);
      led4_prev_state  = OFF;
    }
    if(count >1 && T_prev == 1){  //On for 100ms
      T_prev = 0;
      count = 0;
      ledPin4.state = LED_ON;
      KeusGPIOSetPinValue(&ledPin4);
      led4_prev_state  = ON;
    }
  }
}
/********************************************************************
 * @brief   Blink led
 *          If on->Off for 100ms->on for 100ms->Off
 *          If Off->On for 100ms->Off for 500ms
 * *****************************************************************/
void led_toggle(uint8 led_no){
  if(led_no == 0x01)
  {
    led_blink_t = 0;
    if(led1_prev_state == ON){
      if(first){
        first=0;
        ledPin1.state = LED_OFF;
        KeusGPIOSetPinValue(&ledPin1);
        //led1_prev_state  = OFF;
        toggle1 = 1;
      }
        else if(count1 > 5 && timer==1 ){
          count1 = 0;
          timer = 2; 
          ledPin1.state = LED_ON;
          KeusGPIOSetPinValue(&ledPin1);
          //led1_prev_state  = ON;
        }
        else if(count1 > 5 && timer==2){
          count1 = 0;
          timer = 1;
          ledPin1.state = LED_OFF;
          KeusGPIOSetPinValue(&ledPin1);
          led1_prev_state  = OFF;
          led_toggle_t =0;
          toggle1 = 0;
        }  
    }
    else if(led1_prev_state == OFF){
      led_blink_t = 0;
      ledPin1.state = LED_ON;
      KeusGPIOSetPinValue(&ledPin1);
      toggle1 = 1;
      //led1_prev_state  = ON;
      if(count1 > 5 ){
      count1 = 0;
      //timer = 2; 
      ledPin1.state = LED_OFF;
      KeusGPIOSetPinValue(&ledPin1);
      led1_prev_state  = OFF;
      led_toggle_t =0;
      toggle1 = 0;
      }
    }
  }
  else if(led_no == 0x02)
    {
     led_blink_t = 0;
     if(led2_prev_state == ON){
        ledPin2.state = LED_OFF;
        KeusGPIOSetPinValue(&ledPin2);
        toggle1 = 1;
        led2_prev_state  = OFF;
        if(count1 > 5 && timer==1 ){
          count1 = 0;
          timer = 2; 
          ledPin2.state = LED_ON;
          KeusGPIOSetPinValue(&ledPin2);
          led2_prev_state  = ON;
        }
        if(count1 > 5 && timer==2){
          ledPin2.state = LED_OFF;
          KeusGPIOSetPinValue(&ledPin2);
          led2_prev_state  = OFF;
          led_toggle_t =0;
          timer = 1;
          count1 = 0;
          toggle1 = 0;

        }
      else if(led2_prev_state == OFF){
        led_blink_t = 0;
        toggle1 = 1;
        ledPin2.state = LED_ON;
        KeusGPIOSetPinValue(&ledPin2);
        led2_prev_state  = ON;
        if(count1 > 1 ){
          count1 = 0;
          //timer = 2; 
          ledPin2.state = LED_OFF;
          KeusGPIOSetPinValue(&ledPin2);
          led2_prev_state  = OFF;
          led_toggle_t =0;
          toggle1 = 0;

        }
      }

    }
  }
  else if(led_no  == 0x03)
    {
     led_blink_t = 0;
     if(led3_prev_state == ON){
        toggle1 = 1;
        ledPin3.state = LED_OFF;
        KeusGPIOSetPinValue(&ledPin1);
        led3_prev_state  = OFF;
        if(count1 > 1 && timer==1 ){
          count1 = 0;
          timer = 2; 
          ledPin3.state = LED_ON;
          KeusGPIOSetPinValue(&ledPin3);
          led3_prev_state  = ON;
        }
        if(count1 > 1 && timer==2){
          count1 = 0;
          timer = 1;
          ledPin3.state = LED_OFF;
          KeusGPIOSetPinValue(&ledPin3);
          led3_prev_state  = OFF;
          led_toggle_t =0;
          toggle1 = 0;

        }

      }
      else if(led3_prev_state == OFF){
        toggle1 = 1;
        ledPin3.state = LED_ON;
        KeusGPIOSetPinValue(&ledPin3);
        led3_prev_state  = ON;
        if(count1 > 1 ){
          count1 = 0;
          //timer = 2; 
          ledPin3.state = LED_OFF;
          KeusGPIOSetPinValue(&ledPin3);
          led3_prev_state  = OFF;
          led_toggle_t =0;
          toggle1 = 0;

        }
      }
    }
  else if(led_no == 0x04)
    {
      led_blink_t = 0;
     if(led4_prev_state == ON){
        ledPin4.state = LED_OFF;
        KeusGPIOSetPinValue(&ledPin4);
        toggle1 = 1;
        led4_prev_state  = OFF;
        if(count1 > 1 && timer==1 ){
          count1 = 0;
          timer = 2; 
          ledPin4.state = LED_ON;
          KeusGPIOSetPinValue(&ledPin4);
          led4_prev_state  = ON;
        }
        if(count1 > 1 && timer==2){
          count1 = 0;
          timer = 1;
          ledPin4.state = LED_OFF;
          KeusGPIOSetPinValue(&ledPin4);
          led4_prev_state  = OFF;
          led_toggle_t =0;
          toggle1 = 0;

        }
      }
      else if(led4_prev_state == OFF){
        toggle1 = 1;
        ledPin4.state = LED_ON;
        KeusGPIOSetPinValue(&ledPin4);
        led4_prev_state  = ON;
        if(count1 > 4 ){
          count1 = 0;
          //timer = 2; 
          ledPin4.state = LED_OFF;
          KeusGPIOSetPinValue(&ledPin4);
          led4_prev_state  = OFF;
          led_toggle_t =0;
        }
      }
    }
}


/*****************************************************************************
*@brief Save given Scene in Two dimensional Array\
*       /if scene doesnot exist else replace
******************************************************************************/
void save_scene_task(void){

  led_toggle_t = 0;
  led_blink_t = 0;
  uint8 i=0,j=0,scn_id;
  scn_id = rec_buff[j];
  int8 arr_index = 0;

  // **search if scn_id already exist
  arr_index = search_scn_id(scn_id);

  // **If scene exist replace the scene
  if(arr_index>=0){
    for(j=0;j<data_length;j++){
      scene_arr[arr_index][j] = rec_buff[j];
    }
  }
  else {

    if(scn_no>2){
      HalUARTWrite( HAL_UART_PORT_0, "memory full", (byte)osal_strlen("memory full"));
    }
    else
    {
      for(i=scn_no;i<scn_no+1;i++){
      for(j=0;j<data_length;j++){
        scene_arr[i][j] = rec_buff[j];
      }
    }
    uart_send_ack(cmd_id);
    }
  }
  scn_no++; 
  write_status = KeusThemeSwitchMiniWriteConfigDataIntoMemory();
}

/*********************************************************************************
 * @brief   Execute given Scene from saved scene if exist
 * 
***********************************************************************************/
void execute_scene_task(void){

  led_toggle_t = 0;
  led_blink_t = 0;

  uint8 scn_id    = rec_buff[0];
  int8 arr_index = 0;
  arr_index = search_scn_id(scn_id);
  
    if(arr_index>=0){
      execute_scene(arr_index);
      uart_send_ack(cmd_id);
    }
    else
    {
       HalUARTWrite( HAL_UART_PORT_0, "Not exist", (byte)osal_strlen("Not exist"));

    }
    
  }

void execute_scene(uint8 arr_index){
  uint8 no_of_led,i=0,data_ptr = 0,led_state,led_no;
  no_of_led = scene_arr[arr_index][1];
  data_ptr = 2;

  for(i=0;i<no_of_led;i++){
    led_no    = scene_arr[arr_index][data_ptr];
    led_state = scene_arr[arr_index][data_ptr + 1];
    data_ptr  = data_ptr+2;
    led_control(led_state,led_no);
  }

}

/**********************************************************************
 * @brief Execute Group of Led at once without saving any scene
 * 
 * *******************************************************************/
void group_control(void){
  uint8 no_of_led,i=0,data_ptr = 0,led_state,led_no;
  no_of_led = rec_buff[data_ptr];
  data_ptr++;

  for(i=0;i<no_of_led;i++){
    led_no    = rec_buff[data_ptr];
    data_ptr++;
    led_state = rec_buff[data_ptr];
    data_ptr++;
    led_control(led_state,led_no);
  }
}

/*************************************************************************
 * @brief   Callback function for Uart
 * ***********************************************************************/
void uartRxCb( uint8 port, uint8 event ) {
 uint8  ch,data_ptr=0;

   while (Hal_UART_RxBufLen(port))
 {
   // Read one byte from UART to ch
   HalUARTRead (port, &ch, 1);

   //check for the start byte
   if (ch == 0xAA) {
     uart_cmd_started = 1;
     data_ptr = 0;
   }

   if(uart_cmd_started){
    if(ch == 0xFF){
      uart_cmd_end = 1;
      uart_cmd_started = 0;
    }
   }
   uart_data[ data_ptr++ ] = ch;
   if(data_ptr >20){
     data_ptr = 0; 
   }
 }
}

void KEUS_delayms(uint16 ms) {
 for (uint16 i = 0; i < ms; i++) {
   Onboard_wait(1000);
 }
}


void KEUS_init() {
  KeusGPIOSetDirection(&ledPin1);
  KeusGPIOSetDirection(&buttonPin1);
  KeusGPIOEdgeConfiguration (&buttonPin1,GPIO_FALLING_EDGE);
  KeusGPIOReadPinValue(&buttonPin1);
  KeusGPIOSetPinValue(&ledPin1);
  

  KeusGPIOSetDirection(&ledPin2);
  KeusGPIOSetDirection(&buttonPin2);
  KeusGPIOEdgeConfiguration (&buttonPin2,GPIO_FALLING_EDGE);
  KeusGPIOReadPinValue(&buttonPin2);
  KeusGPIOSetPinValue(&ledPin2);

  KeusGPIOSetDirection(&ledPin3);
  KeusGPIOSetDirection(&buttonPin3);
  KeusGPIOEdgeConfiguration (&buttonPin3,GPIO_FALLING_EDGE);
  KeusGPIOReadPinValue(&buttonPin3);
  KeusGPIOSetPinValue(&ledPin3);

  KeusGPIOSetDirection(&ledPin4);
  KeusGPIOSetDirection(&buttonPin4);
  KeusGPIOEdgeConfiguration (&buttonPin4,GPIO_FALLING_EDGE);
  KeusGPIOReadPinValue(&buttonPin4);
  KeusGPIOSetPinValue(&ledPin4);

  KeusGPIOInterruptEnable(&buttonPin1);
  KeusGPIOInterruptEnable(&buttonPin2);
  KeusGPIOInterruptEnable(&buttonPin3);
  KeusGPIOInterruptEnable(&buttonPin4);
  KeusTimerUtilAddTimer(&intervalTimer);

  init_status = KeusThemeSwitchMiniMemoryInit();
  read_status = KeusThemeSwitchMiniReadConfigDataIntoMemory();


 initUart();

 HalUARTWrite( HAL_UART_PORT_0, "KEUS INIT", (byte)osal_strlen("KEUS INIT"));
KEUS_loop();
}

void KEUS_loop() {
 while (1) {

   HalUARTPoll();
   if( uart_cmd_end == 1 ){
     uart_cmd_end = 0;
   decrypt_data(uart_data);
 }
 if(led_blink_t == 1)
 {
   led_blink(led_no_t);
 }

if(led_toggle_t == 1){
  led_toggle(led_no_toggle); 
   //KEUS_delayms(1000);
 }
  }
}


//*******************************************************************************

HAL_ISR_FUNCTION( halKeusPort1Isr, P1INT_VECTOR )
{
  HAL_ENTER_ISR();
  if(debounce){
      debounce = false;
      if (P1IFG & BV(buttonPin1.bit)) {
          ledPin1.state = KeusGPIOToggledState(ledPin1.state);
          KeusGPIOSetPinValue(&ledPin1);
          led1_prev_state  = !ledPin1.state;
      }

      if (P1IFG & BV(buttonPin2.bit)) {
          ledPin2.state = KeusGPIOToggledState(ledPin2.state); 
          KeusGPIOSetPinValue(&ledPin2);
          led2_prev_state  = !ledPin2.state;
      }
        
      if (P1IFG & BV(buttonPin3.bit)) {
        ledPin3.state = KeusGPIOToggledState(ledPin3.state); 
        KeusGPIOSetPinValue(&ledPin3);
        led3_prev_state  = !ledPin3.state;
      }
      if (P1IFG & BV(buttonPin4.bit)) {
        ledPin4.state = KeusGPIOToggledState(ledPin4.state); 
        KeusGPIOSetPinValue(&ledPin4);
        led4_prev_state  = !ledPin4.state;
      }
    }
    KeusTimerUtilAddTimer(&debounceTimer);

  /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
  */
  P1IFG = 0;
  P1IF = 0;

  CLEAR_SLEEP_MODE();
  HAL_EXIT_ISR();
}
/*********************************************************************
* LOCAL FUNCTIONS
*/

static void zmain_ext_addr( void );
#if defined ZCL_KEY_ESTABLISH
static void zmain_cert_init( void );
#endif
static void zmain_dev_info( void );
static void zmain_vdd_check( void );

#ifdef LCD_SUPPORTED
static void zmain_lcd_init( void );
#endif

/*********************************************************************
* @fn      main
* @brief   First function called after startup.
* @return  don't care
*/
int main( void )
{
 // Turn off interrupts
 osal_int_disable( INTS_ALL );

 // Initialization for board related stuff such as LEDs
 HAL_BOARD_INIT();

//Turn on timer
 KeusTimerUtilInit();
 KeusTimerUtilStartTimer();

 // Make sure supply voltage is high enough to run
 zmain_vdd_check();

 // Initialize board I/O
 InitBoard( OB_COLD );

 // Initialze HAL drivers
 HalDriverInit();

 // Initialize NV System
 osal_nv_init( NULL );

 // Initialize the MAC
 ZMacInit();

 // Determine the extended address
 zmain_ext_addr();

#if defined ZCL_KEY_ESTABLISH
 // Initialize the Certicom certificate information.
 zmain_cert_init();
#endif

 // Initialize basic NV items
 zgInit();

#ifndef NONWK
 // Since the AF isn't a task, call it's initialization routine
 afInit();
#endif

 // Initialize the operating system
 osal_init_system();

 // Allow interrupts
 osal_int_enable( INTS_ALL );

 // Final board initialization
 InitBoard( OB_READY );
 
 KEUS_init();

 // Display information about this device
 zmain_dev_info();

#ifdef WDT_IN_PM1
 /* If WDT is used, this is a good place to enable it. */
 WatchDogEnable( WDTIMX );
#endif
 
 osal_start_system(); // No Return from here

 return 0;  // Shouldn't get here.
} // main()

/*********************************************************************
* @fn      zmain_vdd_check
* @brief   Check if the Vdd is OK to run the processor.
* @return  Return if Vdd is ok; otherwise, flash LED, then reset
*********************************************************************/
static void zmain_vdd_check( void )
{
 uint8 cnt = 16;
 
 do {
   while (!HalAdcCheckVdd(VDD_MIN_RUN));
 } while (--cnt);
}

/**************************************************************************************************
* @fn          zmain_ext_addr
*
* @brief       Execute a prioritized search for a valid extended address and write the results
*              into the OSAL NV system for use by the system. Temporary address not saved to NV.
*
* input parameters
*
* None.
*
* output parameters
*
* None.
*
* @return      None.
**************************************************************************************************
*/
static void zmain_ext_addr(void)
{
 uint8 nullAddr[Z_EXTADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
 uint8 writeNV = TRUE;

 // First check whether a non-erased extended address exists in the OSAL NV.
 if ((SUCCESS != osal_nv_item_init(ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL))  ||
     (SUCCESS != osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress)) ||
     (osal_memcmp(aExtendedAddress, nullAddr, Z_EXTADDR_LEN)))
 {
   // Attempt to read the extended address from the location on the lock bits page
   // where the programming tools know to reserve it.
   HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET, aExtendedAddress, Z_EXTADDR_LEN);

   if (osal_memcmp(aExtendedAddress, nullAddr, Z_EXTADDR_LEN))
   {
     // Attempt to read the extended address from the designated location in the Info Page.
     if (!osal_memcmp((uint8 *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET), nullAddr, Z_EXTADDR_LEN))
     {
       osal_memcpy(aExtendedAddress, (uint8 *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET), Z_EXTADDR_LEN);
     }
     else  // No valid extended address was found.
     {
       uint8 idx;
       
#if !defined ( NV_RESTORE )
       writeNV = FALSE;  // Make this a temporary IEEE address
#endif

       /* Attempt to create a sufficiently random extended address for expediency.
        * Note: this is only valid/legal in a test environment and
        *       must never be used for a commercial product.
        */
       for (idx = 0; idx < (Z_EXTADDR_LEN - 2);)
       {
         uint16 randy = osal_rand();
         aExtendedAddress[idx++] = LO_UINT16(randy);
         aExtendedAddress[idx++] = HI_UINT16(randy);
       }
       // Next-to-MSB identifies ZigBee devicetype.
#if ZG_BUILD_COORDINATOR_TYPE && !ZG_BUILD_JOINING_TYPE
       aExtendedAddress[idx++] = 0x10;
#elif ZG_BUILD_RTRONLY_TYPE
       aExtendedAddress[idx++] = 0x20;
#else
       aExtendedAddress[idx++] = 0x30;
#endif
       // MSB has historical signficance.
       aExtendedAddress[idx] = 0xF8;
     }
   }

   if (writeNV)
   {
     (void)osal_nv_write(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress);
   }
 }

 // Set the MAC PIB extended address according to results from above.
 (void)ZMacSetReq(MAC_EXTENDED_ADDRESS, aExtendedAddress);
}

#if defined ZCL_KEY_ESTABLISH
/**************************************************************************************************
* @fn          zmain_cert_init
*
* @brief       Initialize the Certicom certificate information.
*
* input parameters
*
* None.
*
* output parameters
*
* None.
*
* @return      None.
**************************************************************************************************
*/
static void zmain_cert_init(void)
{
 uint8 certData[ZCL_KE_IMPLICIT_CERTIFICATE_LEN];
 uint8 nullData[ZCL_KE_IMPLICIT_CERTIFICATE_LEN] = {
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
 };

 (void)osal_nv_item_init(ZCD_NV_IMPLICIT_CERTIFICATE, ZCL_KE_IMPLICIT_CERTIFICATE_LEN, NULL);
 (void)osal_nv_item_init(ZCD_NV_DEVICE_PRIVATE_KEY, ZCL_KE_DEVICE_PRIVATE_KEY_LEN, NULL);

 // First check whether non-null certificate data exists in the OSAL NV. To save on code space,
 // just use the ZCD_NV_CA_PUBLIC_KEY as the bellwether for all three.
 if ((SUCCESS != osal_nv_item_init(ZCD_NV_CA_PUBLIC_KEY, ZCL_KE_CA_PUBLIC_KEY_LEN, NULL))    ||
     (SUCCESS != osal_nv_read(ZCD_NV_CA_PUBLIC_KEY, 0, ZCL_KE_CA_PUBLIC_KEY_LEN, certData))  ||
     (osal_memcmp(certData, nullData, ZCL_KE_CA_PUBLIC_KEY_LEN)))
 {
   // Attempt to read the certificate data from its corresponding location on the lock bits page.
   HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_CA_PUBLIC_KEY_OSET, certData,
                                        ZCL_KE_CA_PUBLIC_KEY_LEN);
   // If the certificate data is not NULL, use it to update the corresponding NV items.
   if (!osal_memcmp(certData, nullData, ZCL_KE_CA_PUBLIC_KEY_LEN))
   {
     (void)osal_nv_write(ZCD_NV_CA_PUBLIC_KEY, 0, ZCL_KE_CA_PUBLIC_KEY_LEN, certData);
     HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IMPLICIT_CERT_OSET, certData,
                                          ZCL_KE_IMPLICIT_CERTIFICATE_LEN);
     (void)osal_nv_write(ZCD_NV_IMPLICIT_CERTIFICATE, 0,
                         ZCL_KE_IMPLICIT_CERTIFICATE_LEN, certData);
     HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_DEV_PRIVATE_KEY_OSET, certData,
                                       ZCL_KE_DEVICE_PRIVATE_KEY_LEN);
     (void)osal_nv_write(ZCD_NV_DEVICE_PRIVATE_KEY, 0, ZCL_KE_DEVICE_PRIVATE_KEY_LEN, certData);
   }
 }
}
#endif

/**************************************************************************************************
* @fn          zmain_dev_info
*
* @brief       This displays the IEEE (MSB to LSB) on the LCD.
*
* input parameters
*
* None.
*
* output parameters
*
* None.
*
* @return      None.
**************************************************************************************************
*/
static void zmain_dev_info(void)
{
 
#if defined ( SERIAL_DEBUG_SUPPORTED ) || (defined ( LEGACY_LCD_DEBUG ) && defined (LCD_SUPPORTED))
 uint8 i;
 uint8 *xad;
 uint8 lcd_buf[Z_EXTADDR_LEN*2+1];

 // Display the extended address.
 xad = aExtendedAddress + Z_EXTADDR_LEN - 1;

 for (i = 0; i < Z_EXTADDR_LEN*2; xad--)
 {
   uint8 ch;
   ch = (*xad >> 4) & 0x0F;
   lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
   ch = *xad & 0x0F;
   lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
 }
 lcd_buf[Z_EXTADDR_LEN*2] = '\0';
 HalLcdWriteString( "IEEE: ", HAL_LCD_DEBUG_LINE_1 );
 HalLcdWriteString( (char*)lcd_buf, HAL_LCD_DEBUG_LINE_2 );
#endif
}

#ifdef LCD_SUPPORTED
/*********************************************************************
* @fn      zmain_lcd_init
* @brief   Initialize LCD at start up.
* @return  none
*********************************************************************/
static void zmain_lcd_init ( void )
{
#ifdef SERIAL_DEBUG_SUPPORTED
 {
   HalLcdWriteString( "TexasInstruments", HAL_LCD_DEBUG_LINE_1 );

#if defined( MT_MAC_FUNC )
#if defined( ZDO_COORDINATOR )
     HalLcdWriteString( "MAC-MT Coord", HAL_LCD_DEBUG_LINE_2 );
#else
     HalLcdWriteString( "MAC-MT Device", HAL_LCD_DEBUG_LINE_2 );
#endif // ZDO
#elif defined( MT_NWK_FUNC )
#if defined( ZDO_COORDINATOR )
     HalLcdWriteString( "NWK Coordinator", HAL_LCD_DEBUG_LINE_2 );
#else
     HalLcdWriteString( "NWK Device", HAL_LCD_DEBUG_LINE_2 );
#endif // ZDO
#endif // MT_FUNC
 }
#endif // SERIAL_DEBUG_SUPPORTED
}
#endif

/*********************************************************************
*********************************************************************/