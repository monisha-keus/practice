
/********************************************************
 * @fn  This function is to declare all GPIOs
 * *****************************************************/

#include "keus_task.h"

bool init_status = 0;
bool read_status = 0;
bool write_status = 0;

bool debounce = true;
bool toggle = false, toggle1 = false, toggle3 = false;
uint8 count = 0, count1 = 0, count3 = 0;//led_state;
uint8 button_Pressed =0 ;
struct CONFIG_INFO_t config_data[4];

void ledTimerCbk(uint8 timerId);
void leddebounceCbk(uint8 timerId);

//Debounce configuration
KeusTimerConfig debounceTimer = {
    &leddebounceCbk,
    200,
    true,
    -1,
    0};

//Timer Configuration
KeusTimerConfig intervalTimer = {
    &ledTimerCbk,
    100,
    true,
    -1,
    0};

void decrypt_data(uint8 *uart_data);
void process_Rx_data(void);
void KEUS_init_fnc(void);
void uart_send_ack(uint8 cmnd_id);

void group_control(void);
void validate_state(uint8 led_no, uint8 led_state);
//void update_led_array(uint8 led_no,uint8 led_state);
void KEUS_loop(void);
void configure_switch(void);
void update_config_struct(uint8 led_no, uint8 led_state);
void update_config_to_memory(void);
void correct_config_state(uint8 led_no, uint8 config_id);
void uart_send_switch_sate_ack(void);
void uart_send_config_sate_ack(void);

void uart_send_button_ack(void);




/*******************************************************
 * @brief  Callback function for timer
           Called every after 100ms

*********************************************************/
void ledTimerCbk(uint8 timerId)
{
  if (toggle)
  { //To control Blinking led rate
    count++;
  }
  if (toggle1)
  { //To control Blink Led rate
    count1++;
  }
}

/**************************************************************
 * @fn      leddebounceCbk
 * @brief   callback function for Debounce,called every after 200ms
 * @param   none
 * @return  None
 * ***********************************************************/

void leddebounceCbk(uint8 timerId)
{
  debounce = true;
}

//********LED and button initilization
extern KeusGPIOPin ledPin1 = {0, 0, GPIO_OUTPUT, false, GPIO_LOW};
extern KeusGPIOPin ledPin2 = {0, 1, GPIO_OUTPUT, false, GPIO_LOW};
extern KeusGPIOPin ledPin3 = {0, 4, GPIO_OUTPUT, false, GPIO_LOW};
extern KeusGPIOPin ledPin4 = {0, 5, GPIO_OUTPUT, false, GPIO_LOW};

extern KeusGPIOPin buttonPin1 = {1, 2, GPIO_INPUT, true, GPIO_LOW};
extern KeusGPIOPin buttonPin2 = {1, 3, GPIO_INPUT, true, GPIO_LOW};
extern KeusGPIOPin buttonPin3 = {1, 4, GPIO_INPUT, true, GPIO_LOW};
extern KeusGPIOPin buttonPin4 = {1, 5, GPIO_INPUT, true, GPIO_LOW};

/*************************************************************
 * @fn      KEUS_init_fnc
 * @brief   All Initilization
 * @return  None
 * @param   None  
 * ***********************************************************/
void KEUS_init_fnc(void)
{

  //****Setting GPIOs direction and edge
  KeusGPIOSetDirection(&ledPin1);
  KeusGPIOSetDirection(&buttonPin1);
  KeusGPIOEdgeConfiguration(&buttonPin1, GPIO_RISING_EDGE);
  KeusGPIOReadPinValue(&buttonPin1);
  KeusGPIOSetPinValue(&ledPin1);

  KeusGPIOSetDirection(&ledPin2);
  KeusGPIOSetDirection(&buttonPin2);
  KeusGPIOEdgeConfiguration(&buttonPin2, GPIO_RISING_EDGE);
  KeusGPIOReadPinValue(&buttonPin2);
  KeusGPIOSetPinValue(&ledPin2);

  KeusGPIOSetDirection(&ledPin3);
  KeusGPIOSetDirection(&buttonPin3);
  KeusGPIOEdgeConfiguration(&buttonPin3, GPIO_RISING_EDGE);
  KeusGPIOReadPinValue(&buttonPin3);
  KeusGPIOSetPinValue(&ledPin3);

  KeusGPIOSetDirection(&ledPin4);
  KeusGPIOSetDirection(&buttonPin4);
  KeusGPIOEdgeConfiguration(&buttonPin4, GPIO_RISING_EDGE);
  KeusGPIOReadPinValue(&buttonPin4);
  KeusGPIOSetPinValue(&ledPin4);

  //*****Enabling Interrupts
  KeusGPIOInterruptEnable(&buttonPin1);
  KeusGPIOInterruptEnable(&buttonPin2);
  KeusGPIOInterruptEnable(&buttonPin3);
  KeusGPIOInterruptEnable(&buttonPin4);
  KeusTimerUtilAddTimer(&intervalTimer);
  init_status = KeusThemeSwitchMiniMemoryInit();
  read_status = KeusThemeSwitchMiniReadConfigDataIntoMemory();
  //*****Timer Initialization
  KeusTimerUtilInit();

  //******Timer Start
  KeusTimerUtilStartTimer();

  //*****UART Initialization
  initUart();

  HalUARTWrite(HAL_UART_PORT_0, "KEUS INIT", (byte)osal_strlen("KEUS INIT"));
  KEUS_loop();
}

/**********************************************************************
 * @fn      KEUS_loop
 * @brief   Infinite loop
 * @return  No return after this
 * @param   None
 * *********************************************************************/
void KEUS_loop(void)
{
  while (1)
  {
    HalUARTPoll();
    if (uart_cmd_end)
    {
      uart_cmd_end = 0;
      process_Rx_data();
    }
    if(button_Pressed){
      if (event_t & GET_BUTTON_STATE){
      uart_send_button_ack();
    }
    event_t =0x00;
    }
    if (event_t & GET_BUTTON_STATE)
  {
    //   execute_scene_task();
    uart_send_ack(cmd_id);
  }
  }
}

/**************************************************************************
 * @fn      process_Rx_data
 * @brief   This function is to process Rx data from uart
 * 
 * *************************************************************************/
void process_Rx_data(void)
{

  if (event_t & EXECUTE_SWITCH_STATE)
  {
    group_control();
    uart_send_ack(cmd_id);
  }

  else if (event_t & CONFIGURE_SWITCH)
  {
    configure_switch();
    uart_send_ack(cmd_id);
  }

  else if (event_t & GET_SWITCH_STATE)
  {
    uart_send_switch_sate_ack();
  }

  else if (event_t & GET_CONFIG_STATE)
  {
    //   execute_scene_task();
    uart_send_config_sate_ack();
  }

 // write_status = KeusThemeSwitchMiniWriteConfigDataIntoMemory();
  event_t = 0x00;
}

/*********************************************************************
 * @fn      group_control 
 * @brief   Execute Group of Led at once without saving any scene
 * @param   None
 * @return  None
 * *******************************************************************/
void group_control(void)
{
  uint8 no_of_led, i = 0, data_ptr = 1, led_state, led_no;
  //data_ptr++;
  no_of_led = data_buff[data_ptr];
  data_ptr++;

  for (i = 0; i < no_of_led; i++)
  {
    led_no = data_buff[data_ptr++];
    led_state = data_buff[data_ptr++];
    //update_led_array(led_no,led_state);
    validate_state(led_no, led_state);
  }
  
}

// void update_led_array(uint8 led_no,uint8 led_state){
// switch (led_no)
//   {
//   case 0x01:{
//       // ledPin1.state = led_state;
//       // KeusGPIOSetPinValue(&ledPin1);
//       validate_state(led_no,led_state);
//       break;
//   }
//   case 0x02:{
//     ledPin2.state = led_state;
//     KeusGPIOSetPinValue(&ledPin2);
//     break;
//   }

//   case 0x03:{
//     ledPin3.state = led_state;
//     KeusGPIOSetPinValue(&ledPin3);
//     break;
//   }

//   case 0x04:{
//     ledPin4.state = led_state;
//     KeusGPIOSetPinValue(&ledPin4);
//     break;
//   }

//   default:
//     break;
// }

/***************************************************************************
 * @fn      validate_state
 * @brief   based on config_id of corresponding led validate state
 * @param   led_state : 0-255
 *          led_no    : which led
 * 
 * @return  None
 **************************************************************************/
void validate_state(uint8 led_no, uint8 led_state_t)
{
  uint8 led_state;
  led_state = led_state_t;
  if (led_no == 1)
  {
    if (config_data[0].config_id == ONOFF)
    {
      if (led_state == 0)
      {
        ledPin1.state = LOW;
      }
      else
      {
        ledPin1.state = HIGH;
      }
    }
    else if (config_data[0].config_id == DIMMING)
    {
      ledPin1.state = led_state;
    }
    else if (config_data[0].config_id == FAN_CONTROLLER)
    {
      if (led_state == 0 || led_state == 50 || led_state == 100 || led_state == 150 || led_state == 200 || led_state == 255)
      {
        ledPin1.state = led_state;
      }
      else if (led_state > 0 && led_state < 50)
      {
        ledPin1.state = 50;
      }
      else if (led_state > 50 && led_state < 100)
      {
        ledPin1.state = 100;
      }
      else if (led_state > 100 && led_state < 150)
      {
        ledPin1.state = 150;
      }
      else if (led_state > 150 && led_state < 200)
      {
        ledPin1.state = 200;
      }
      else if (led_state > 200 && led_state < 255)
      {
        ledPin1.state = 255;
      }
    }
    update_config_struct(led_no,ledPin1.state);
  }

  //LED == 2
  else if (led_no == 2)
  {
    if (config_data[1].config_id == ONOFF)
    {
      if (led_state == 0)
      {
        ledPin2.state = LOW;
      }
      else
      {
        ledPin2.state = HIGH;
      }
    }
    else if (config_data[1].config_id == DIMMING)
    {
      ledPin2.state = led_state;
    }
    
    else if (config_data[1].config_id == FAN_CONTROLLER)
    {
      if (ledPin2.state == 0 || ledPin2.state == 50 || ledPin2.state == 100 || ledPin2.state == 150 || ledPin2.state == 200 || ledPin2.state == 255)
      {
        ledPin2.state = led_state;
      }
      else if (led_state > 0 && led_state < 50)
      {
        ledPin2.state = 50;
      }
      else if (led_state > 50 && led_state < 100)
      {
        ledPin2.state = 100;
      }
      else if (led_state > 100 && led_state < 150)
      {
        ledPin2.state = 150;
      }
      else if (led_state > 150 && led_state < 200)
      {
        ledPin2.state = 200;
      }
      else if (led_state > 200 && led_state < 255)
      {
        ledPin2.state = 255;
      }
    }
    update_config_struct(led_no,ledPin2.state);
  }
  //LED == 3
  else if (led_no == 3)
  {
    if (config_data[2].config_id == ONOFF)
    {
      if (led_state == 0)
      {
        ledPin3.state = LOW;
      }
      else
      {
        ledPin3.state = HIGH;
      }
    }
    else if (config_data[2].config_id == DIMMING)
    {
      ledPin3.state = led_state;
      
    }
    else if (config_data[2].config_id == FAN_CONTROLLER)
    {
      if (led_state == 0 || led_state == 50 || led_state == 100 || led_state == 150 || led_state == 200 || led_state == 255)
      {
        ledPin3.state = led_state;
      }
      else if (led_state > 0 && led_state < 50)
      {
        ledPin3.state = 50;
      }
      else if (led_state > 50 && led_state < 100)
      {
        ledPin3.state = 100;
      }
      else if (led_state > 100 && led_state < 150)
      {
        ledPin3.state = 150;
      }
      else if (led_state > 150 && led_state < 200)
      {
        ledPin3.state = 200;
      }
      else if (led_state > 200 && led_state < 255)
      {
        ledPin3.state = 255;
      }
    }
    update_config_struct(led_no,ledPin3.state);
  }
  //LED == 4
  else if (led_no == 4)
  {
    if (config_data[3].config_id == ONOFF)
    {
      if (led_state == 0)
      {
        ledPin4.state = LOW;
      }
      else
      {
        ledPin4.state = HIGH;
      }
    }
    else if (config_data[3].config_id == DIMMING)
    {
      //if(ledPin4.state > 0 && ledPin4.state <=255){
      ledPin4.state = led_state;
      // }
      // else
      // {
      //   led_state = LOW;
      // }
    }
    else if (config_data[3].config_id == FAN_CONTROLLER)
    {
      if (led_state == 0 || led_state == 50 || led_state == 100 || led_state == 150 || led_state == 200 || led_state == 255)
      {
        ledPin4.state = led_state;
      }
      else if (led_state > 0 && led_state < 50)
      {
        ledPin4.state = 50;
      }
      else if (led_state > 50 && led_state < 100)
      {
        ledPin4.state = 100;
      }
      else if (led_state > 100 && led_state < 150)
      {
        ledPin4.state = 150;
      }
      else if (led_state > 150 && led_state < 200)
      {
        ledPin4.state = 200;
      }
      else if (led_state > 200 && led_state < 255)
      {
        ledPin4.state = 255;
      }
    }
    update_config_struct(led_no,ledPin4.state);
  }
}

/**************************************************************************
 * @fn      configure_switch
 * @brief   Based on user input configure switch as on/off,dimming,fancontroller
 * @param   None
 * @return  None  
 * ***********************************************************************/
void configure_switch(void)
{
  uint8 config_id = 0, led_no = 0, data_ptr = 1, no_of_led = 0;
  no_of_led = data_buff[data_ptr++];

  //led_state   = data_buff[data_ptr++];
  for (uint8 i = 0; i < no_of_led; i++)
  {
    led_no = data_buff[data_ptr++];
    config_id = data_buff[data_ptr++];
    correct_config_state(led_no, config_id);
    //update_config_struct(led_no, led_state);
  }
}
/**************************************************************************
 * @fn        correct_state
 * @brief     Based on config,correct state 
 * @return
 * @param     
 * ************************************************************************/
void correct_config_state(uint8 led_no, uint8 config_id)
{
  //led1
  uint8 led_state = 0;
  if (led_no == 1)
  {
    if (config_id == ONOFF)
    {
      if (ledPin1.state == 0)
      {
        led_state = LOW;
      }
      else
      {
        led_state = HIGH;
      }
    }
    else if (config_id == DIMMING)
    {
      //if(ledPin1.state >0 && ledPin1.state <=255){
      led_state = ledPin1.state;
      // }
      // else
      // {
      //   led_state = LOW;
      // }
    }
    else if (config_id == FAN_CONTROLLER)
    {
      if (ledPin1.state == 0 || ledPin1.state == 50 || ledPin1.state == 100 || ledPin1.state == 150 || ledPin1.state == 200 || ledPin1.state == 255)
      {
        led_state = ledPin1.state;
      }
      else if (ledPin1.state > 0 && ledPin1.state < 50)
      {
        led_state = 50;
      }
      else if (ledPin1.state > 50 && ledPin1.state < 100)
      {
        led_state = 100;
      }
      else if (ledPin1.state > 100 && ledPin1.state < 150)
      {
        led_state = 150;
      }
      else if (ledPin1.state > 150 && ledPin1.state < 200)
      {
        led_state = 200;
      }
      else if (ledPin1.state > 200 && ledPin1.state < 255)
      {
        led_state = 255;
      }
    }
    config_data[0].config_id = config_id;
    update_config_struct(led_no,led_state);
  }
  //LED == 2
  else if (led_no == 2)
  {
    if (config_id == ONOFF)
    {
      if (ledPin1.state == 0)
      {
        led_state = LOW;
      }
      else
      {
        led_state = HIGH;
      }
    }
    else if (config_id == DIMMING)
    {
      //if(ledPin1.state >0 && ledPin1.state <=255){
      led_state = ledPin1.state;
    }
    // else
    // {
    //   led_state = LOW;
    // }
    else if (config_id == FAN_CONTROLLER)
    {
      if (ledPin2.state == 0 || ledPin2.state == 50 || ledPin2.state == 100 || ledPin2.state == 150 || ledPin2.state == 200 || ledPin2.state == 255)
      {
        led_state = ledPin1.state;
      }
      else if (ledPin2.state > 0 && ledPin2.state < 50)
      {
        led_state = 50;
      }
      else if (ledPin2.state > 50 && ledPin2.state < 100)
      {
        led_state = 100;
      }
      else if (ledPin2.state > 100 && ledPin2.state < 150)
      {
        led_state = 150;
      }
      else if (ledPin2.state > 150 && ledPin2.state < 200)
      {
        led_state = 200;
      }
      else if (ledPin2.state > 200 && ledPin2.state < 255)
      {
        led_state = 255;
      }
    }
    config_data[1].config_id = config_id;
    update_config_struct( led_no, led_state );
  }
  //LED == 3
  else if (led_no == 3)
  {
    if (config_id == ONOFF)
    {
      if (ledPin3.state == 0)
      {
        led_state = LOW;
      }
      else
      {
        led_state = HIGH;
      }
    }
    else if (config_id == DIMMING)
    {
      led_state = ledPin3.state;
    }
    else if (config_id == FAN_CONTROLLER)
    {
      if (ledPin3.state == 0 || ledPin3.state == 50 || ledPin3.state == 100 || ledPin3.state == 150 || ledPin3.state == 200 || ledPin3.state == 255)
      {
        led_state = ledPin3.state;
      }
      else if (ledPin3.state > 0 && ledPin3.state < 50)
      {
        led_state = 50;
      }
      else if (ledPin3.state > 50 && ledPin3.state < 100)
      {
        led_state = 100;
      }
      else if (ledPin3.state > 100 && ledPin3.state < 150)
      {
        led_state = 150;
      }
      else if (ledPin3.state > 150 && ledPin3.state < 200)
      {
        led_state = 200;
      }
      else if (ledPin3.state > 200 && ledPin3.state < 255)
      {
        led_state = 255;
      }
    }
    config_data[2].config_id = config_id;
    update_config_struct(led_no,led_state);
  }
  //LED == 4
  else if (led_no == 4)
  {
    if (config_id == ONOFF)
    {
      if (ledPin4.state == 0)
      {
        led_state = LOW;
      }
      else
      {
        led_state = HIGH;
      }
    }
    else if (config_id == DIMMING)
    {
      //if(ledPin4.state > 0 && ledPin4.state <=255){
      led_state = ledPin4.state;
      // }
      // else
      // {
      //   led_state = LOW;
      // }
    }
    else if (config_id == FAN_CONTROLLER)
    {
      if (ledPin4.state == 0 || ledPin4.state == 50 || ledPin4.state == 100 || ledPin4.state == 150 || ledPin4.state == 200 || ledPin4.state == 255)
      {
        led_state = ledPin4.state;
      }
      else if (ledPin4.state > 0 && ledPin4.state < 50)
      {
        led_state = 50;
      }
      else if (ledPin4.state > 50 && ledPin4.state < 100)
      {
        led_state = 100;
      }
      else if (ledPin4.state > 100 && ledPin4.state < 150)
      {
        led_state = 150;
      }
      else if (ledPin4.state > 150 && ledPin4.state < 200)
      {
        led_state = 200;
      }
      else if (ledPin4.state > 200 && ledPin4.state < 255)
      {
        led_state = 255;
      }
    }
    config_data[3].config_id = config_id;
    update_config_struct(led_no,led_state);
  }
}

/***********************************************************************
 * @fn      update_config_struct
 * @brief   update config structure
 * 
 * ********************************************************************/
void update_config_struct(uint8 led_no, uint8 led_state)
{
  if (led_no == 1)
  {
    config_data[0].valid_state = led_state;
    config_data[0].led = led_no;
  }
  else if (led_no == 2)
  {
    config_data[1].led = led_no;
    config_data[1].valid_state = led_state;
  }
  else if (led_no == 3)
  {
    config_data[2].led = led_no;
    config_data[2].valid_state = led_state;
  }
  else if (led_no == 4)
  {
    config_data[3].led = led_no;
    config_data[3].valid_state = led_state;
  }
  update_config_to_memory();
}

/*****************************************************************************
 * @fn        update_config_to_memory
 * **************************************************************************/
void update_config_to_memory(void)
{
  write_status = KeusThemeSwitchMiniWriteConfigDataIntoMemory();
}


/*****************************************************************************
 * @fn      uart_send_sate_ack
 * @brief   Send back State of all switch to uart
 * @return
 * @param 
 * **************************************************************************/
void uart_send_switch_sate_ack(void){
  uint8 Tx_buff[12] = {0};
  uint8 data_ptr = 3,config_ptr=0;
  Tx_buff[0] = 0xAA;
  Tx_buff[1] = 0x09;
  Tx_buff[2] = cmd_id;
  for(uint8 i = 0;i<4;i++){
  Tx_buff[data_ptr] = config_data[config_ptr].led;
  data_ptr++;
  Tx_buff[data_ptr] = config_data[config_ptr].valid_state;
  data_ptr++;
  config_ptr++;
  }
  Tx_buff[data_ptr] = 0xFF;
  HalUARTWrite(HAL_UART_PORT_0, Tx_buff, 12);

}

/*****************************************************************************
 * @fn      uart_send_config_sate_ack
 * @brief   Send back State of all config_id to uart
 * @return
 * @param 
 * **************************************************************************/
void uart_send_config_sate_ack(void){
  uint8 Tx_buff[12] = {0};
  uint8 data_ptr = 3,config_ptr=0;
  Tx_buff[0] = 0xAA;
  Tx_buff[1] = 0x09;
  Tx_buff[2] = cmd_id;
  for(uint8 i = 0;i<4;i++){
  Tx_buff[data_ptr] = config_data[config_ptr].led;
  data_ptr++;
  Tx_buff[data_ptr] = config_data[config_ptr].config_id;
  data_ptr++;
  config_ptr++;
  }
  Tx_buff[data_ptr] = 0xFF;
  HalUARTWrite(HAL_UART_PORT_0, Tx_buff, 12);

}

/*************************************************************************
 * @fn      uart_send_button_ack
 * @brief   send to uart which button is pressed 
 * 
 * ***********************************************************************/
void uart_send_button_ack(void){
  uint8 Tx_buff[6] = {0};
  Tx_buff[0] = 0xAA;
  Tx_buff[1] = 0x02;
  Tx_buff[2] = cmd_id;
  Tx_buff[3] = button_Pressed;
  Tx_buff[4] = 0xFF;
  HalUARTWrite(HAL_UART_PORT_0, Tx_buff, 6);
}

/**************************************************************************
  * @fn      uart_send_ack
  * @brief   send Ack after sucessful receiving cmd.
  * @arg     1->uint8-> cmnd_id
  * ************************************************************************/
void uart_send_ack(uint8 cmnd_id)
{
  uint8 Tx_buff[5] = {0};
  Tx_buff[0] = 0xAA;
  Tx_buff[1] = 0x01;
  Tx_buff[2] = cmnd_id;
  Tx_buff[3] = 0xFF;
  HalUARTWrite(HAL_UART_PORT_0, Tx_buff, 4);
}
/*******************************************************************************
 * @fn      HAL_ISR_FUNCTION
 * @brief   ISR for button pressed
 * ****************************************************************************/

HAL_ISR_FUNCTION(halKeusPort1Isr, P1INT_VECTOR)
{
  HAL_ENTER_ISR();
  if (debounce)
  {
    debounce = false;
    if (P1IFG & BV(buttonPin1.bit))
    {
      ledPin1.state = KeusGPIOToggledState(ledPin1.state); //toggle led at button pressed
      KeusGPIOSetPinValue(&ledPin1);
      button_Pressed = SWITCH_1;
      event_t = 0x10;
    }

    if (P1IFG & BV(buttonPin2.bit))
    {
      ledPin2.state = KeusGPIOToggledState(ledPin2.state);
      KeusGPIOSetPinValue(&ledPin2);
      button_Pressed = SWITCH_2;
      event_t = 0x10;
    }

    if (P1IFG & BV(buttonPin3.bit))
    {
      ledPin3.state = KeusGPIOToggledState(ledPin3.state);
      KeusGPIOSetPinValue(&ledPin3);
      button_Pressed = SWITCH_3;
      event_t = 0x10;
    }
    if (P1IFG & BV(buttonPin4.bit))
    {
      ledPin4.state = KeusGPIOToggledState(ledPin4.state);
      KeusGPIOSetPinValue(&ledPin4);
      button_Pressed = SWITCH_4;
      event_t = 0x10;
    }
    KeusTimerUtilAddTimer(&debounceTimer);
  }

  /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
  */
  P1IFG = 0;
  P1IF = 0;

  CLEAR_SLEEP_MODE();
  HAL_EXIT_ISR();
}



/*******************************************************
 * @fn          KeusThemeSwitchMiniMemoryInit
 * @brief       nvic memory init
 * @return
 * @param
 * *****************************************************/
bool KeusThemeSwitchMiniMemoryInit(void)
{
  //  for (uint8 i = 0; i < CLICK_TYPES; i++)
  //  {
  //    themeManager.btnThemeMap[i] = 255;
  //  }

  uint8 res = osal_nv_item_init(NVIC_MEMORY_POSITION, sizeof(config_data), (void *)config_data);

  if (res == SUCCESS || res == NV_ITEM_UNINIT)
  {
    return true;
  }
  else
  {
    return false;
  }
}
/*******************************************************
 * @fn      KeusThemeSwitchMiniReadConfigDataIntoMemory
 * @brief    
 * @return
 * @param  
 * ****************************************************/
bool KeusThemeSwitchMiniReadConfigDataIntoMemory(void)
{
  uint8 res = osal_nv_read(NVIC_MEMORY_POSITION, 0, sizeof(config_data), (void *)config_data);

  if (res == SUCCESS)
  {
    return true;
  }
  else
  {
    return false;
  }
}
/********************************************************
 * @fn  KeusThemeSwitchMiniWriteConfigDataIntoMemory
 * @brief   
 * @return
 * @param
 * *****************************************************/
bool KeusThemeSwitchMiniWriteConfigDataIntoMemory(void)
{
  uint8 res = osal_nv_write(NVIC_MEMORY_POSITION, 0, sizeof(config_data), (void *)config_data);

  if (res == SUCCESS)
  {
    return true;
  }
  else
  {
    return false;
  }
}