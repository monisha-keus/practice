/****************************
 * ****KEUS_TASK.H********
 * **************************/

#ifndef KEUS_TASK_H_INCLUDED
#define KEUS_TASK_H_INCLUDED

#include "keus_gpio_util.h"
#include "keus_timer_util.h"
#include "ZComDef.h"
#include "OSAL_Nv.h"
#include "keus_uart.h"

#define NVIC_MEMORY_POSITION 0x10

bool KeusThemeSwitchMiniMemoryInit(void);
bool KeusThemeSwitchMiniReadConfigDataIntoMemory(void);
bool KeusThemeSwitchMiniWriteConfigDataIntoMemory(void);


#define ON      0x01
#define OFF     0x00
#define HIGH    255
#define LOW     0x00

#define EXECUTE_SWITCH_STATE     0X01
#define CONFIGURE_SWITCH         0X02
#define GET_SWITCH_STATE         0X04
#define GET_CONFIG_STATE         0X08
#define GET_BUTTON_STATE         0X10

#define ONOFF                   1
#define DIMMING                 2
#define FAN_CONTROLLER          3

#define SWITCH_1                1
#define SWITCH_2                2
#define SWITCH_3                3
#define SWITCH_4                4



// #define LED_CONTROL     1
// #define SAVE_SCENE      2
// #define EXECUTE_SCENE   3
// #define DELETE_SCENE    4
// #define GROUP_CONTROL   5

#define LED_OFF         GPIO_HIGH
#define LED_ON          GPIO_LOW


//*****config data 
struct CONFIG_INFO_t{
    uint8   led;
    uint8   valid_state;
    uint8   config_id; 
};


//Array of four config for four led
// struct CONFIG_INFO_t{
//    struct CONFIG   config_t[4];  
// };

extern struct CONFIG_INFO_t config_data[4];

extern void uart_send_ack(uint8 cmnd_id);
extern void initUart(void);

extern uint8 data_buff[20];
extern uint8 uart_cmd_end;
extern uint16 event_t;
extern uint8 cmd_id;
extern uint8 data_length;
extern uint8 button_Pressed;

// extern bool KeusThemeSwitchMiniMemoryInit(void);
// extern bool KeusThemeSwitchMiniReadConfigDataIntoMemory(void);
// extern bool KeusThemeSwitchMiniWriteConfigDataIntoMemory(void);

#endif





