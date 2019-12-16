#include "keus_timer_util.h"
#include  "hal_mcu.h"
#include  "hal_defs.h"
#include  "hal_types.h"

#define T1STAT_CH0IF BV(0)
#define MAX_TIMERS 15

// in ms
#define TIMER_UNIT 25

static KeusTimerConfig *timersInfo[MAX_TIMERS];
bool initGaurd = false;

// valid timerIds are from 0 - (MAX_TIMERS-1)
// Local Functions
void KeusTimerUtilClearTimer(uint8 timerNo);
void KeusTimerUtilTicker(void);
void KeusTimerUtilInterruptHandler(void);

void KeusTimerUtilClearTimer(uint8 timerNo) {
  if (timersInfo[timerNo] != NULL) {
    timersInfo[timerNo]->timerId = -1;
    timersInfo[timerNo]->currentTimerVal = 0;
    timersInfo[timerNo]->active = false;
  }

  timersInfo[timerNo] = NULL;
}

uint8 KeusTimerUtilGetNumActiveTimers(void) {
  uint8 totalActiveTimers = 0;

  for (uint8 i = 0; i < MAX_TIMERS; i++) {
    if (timersInfo[i] != NULL) {
      if (timersInfo[i]->active) {
        totalActiveTimers += 1;
      }
    }
  }
  
  return totalActiveTimers;
}

void KeusTimerUtilInit() {
  if (!initGaurd) {
    initGaurd = true;
    
    for (uint8 i = 0; i < MAX_TIMERS; i++) {
      KeusTimerUtilClearTimer(i);
    }
    
    T1CCTL0 = 0;    /* Make sure interrupts are disabled */
    T1CCTL1 = 0;    /* Make sure interrupts are disabled */
    T1CCTL2 = 0;    /* Make sure interrupts are disabled */
    T1CCTL3 = 0;    /* Make sure interrupts are disabled */
    T1CCTL4 = 0;    /* Make sure interrupts are disabled */

    // SET TIMER1 TO MODULO MODE
    T1CTL &= ~(BV(0));
    T1CTL &= ~(BV(1));
    
    // PRESCALAR 128 - (1 tick = 4 microseconds)
    T1CTL |= BV(2);
    T1CTL |= BV(3);
    
    // 0b01001100 - SETTING OUTPUT COMPARE/CAPTURE REGISTER TO COMPARE MODE
    T1CCTL0 = 0x4C;
    
    // SETTING COMPARE VALUE TO 25 MICROSECONDS
    uint16 count = 0x186A;
    T1CC0H = (uint8) (count >> 8);
    T1CC0L = (uint8) count;
    
    // GLOBAL CPU INTERRUPT ENABLE
    IEN1 |= BV(1);
  }
}

void KeusTimerUtilStartTimer() {
  // modulo mode of timer 1 - (10)
  T1CTL |= BV(1);
}

void KeusTimerUtilStopTimer() {
  // operation suspended mode of timer1 - (00)
  T1CTL &= ~(BV(1));
}

bool KeusTimerUtilAddTimer(KeusTimerConfig *configObj) {
  if (!initGaurd) {
    KeusTimerUtilInit();
  }
  
  int8 currentAvailableTimer = -1;
  
  for (uint8 i = 0; i < MAX_TIMERS; i++) {
    if (timersInfo[i] == NULL || timersInfo[i]->timerId == -1) {
      currentAvailableTimer = i;
      break;
    }
  }
  
  if (currentAvailableTimer > -1) {
    timersInfo[currentAvailableTimer] = configObj;
    timersInfo[currentAvailableTimer]->active = true;
    timersInfo[currentAvailableTimer]->timerId = currentAvailableTimer;
    timersInfo[currentAvailableTimer]->currentTimerVal = 0;

    return true;
  } else {
    return false;
  }
}

bool KeusTimerUtilRemoveTimer(KeusTimerConfig *configObj) {
  int8 timerId = configObj->timerId;
  
  if (timerId > -1 && timerId < MAX_TIMERS) {
    KeusTimerUtilClearTimer(timerId);
    return true;
  } else {
    return false;
  }
}

void KeusTimerUtilTicker() {
  for (uint8 i = 0; i < MAX_TIMERS; i++) {
    if (timersInfo[i] != NULL && timersInfo[i]->active && timersInfo[i]->timerId != -1) {
      timersInfo[i]->currentTimerVal += TIMER_UNIT;
      
      if (timersInfo[i]->currentTimerVal >= timersInfo[i]->timerInterval) {
        timersInfo[i]->currentTimerVal = 0;
        timersInfo[i]->cbk(timersInfo[i]->timerId);
      }
    }
  }
}

void KeusTimerUtilInterruptHandler() {
  if (T1STAT & T1STAT_CH0IF) {
    T1STAT &= ~(T1STAT_CH0IF);
    KeusTimerUtilTicker();
  }
}

HAL_ISR_FUNCTION( halTimer1Isr, T1_VECTOR ) {
  KeusTimerUtilInterruptHandler();
}