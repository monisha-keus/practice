#ifndef KEUS_TIMER_UTIL_H
#define KEUS_TIMER_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "ZComDef.h"
#include "hal_mcu.h"
  
typedef void (*KeusTimerCbk)( uint8 timerId );

typedef struct {
  KeusTimerCbk cbk;
  uint16 timerInterval;
  bool active;
  int8 timerId;
  uint16 currentTimerVal;
} KeusTimerConfig;

extern void KeusTimerUtilInit(void);
extern void KeusTimerUtilStartTimer(void);
extern void KeusTimerUtilStopTimer(void);
extern bool KeusTimerUtilAddTimer(KeusTimerConfig *configObj);
extern bool KeusTimerUtilRemoveTimer(KeusTimerConfig *configObj);

extern uint8 KeusTimerUtilGetNumActiveTimers(void);

#ifdef __cplusplus
}
#endif

#endif