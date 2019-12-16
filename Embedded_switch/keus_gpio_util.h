#ifndef KEUS_GPIO_UTIL_H
#define KEUS_GPIO_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif  

#include "ZComDef.h"
#include "hal_mcu.h"

#ifndef KEUS_GPIO_INTERRUPT_HANDLER_DISABLE
typedef void (*KeusInterruptHandler)(void);
#endif
  
typedef struct {
  const uint8 port;
  const uint8 bit;
  uint8 dir;
  const bool istristate;
  uint8 state;
  #ifndef KEUS_GPIO_INTERRUPT_HANDLER_DISABLE
  KeusInterruptHandler handler;
  #endif
} KeusGPIOPin;

#define GPIO_HIGH 0x01
#define GPIO_LOW 0x00
  
#define GPIO_OUTPUT 0x01
#define GPIO_INPUT 0x00

#define GPIO_RISING_EDGE 0x01
#define GPIO_FALLING_EDGE 0x02
    
extern void KeusGPIOSetDirection(KeusGPIOPin *pin);
extern void KeusGPIOSetPinValue(KeusGPIOPin *pin);
extern void KeusGPIOReadPinValue(KeusGPIOPin *pin);
extern uint8 KeusGPIOToggledState(uint8 state);
#ifndef KEUS_GPIO_INTERRUPT_HANDLER_DISABLE
extern void KeusGPIOSetInterruptHandler(KeusGPIOPin *pin, uint8 num);
#endif
extern void KeusGPIOInterruptEnable(KeusGPIOPin *pin);
extern void KeusGPIOEdgeConfiguration(KeusGPIOPin *pin, uint8 edgeConfig);

#ifdef __cplusplus
}
#endif

#endif