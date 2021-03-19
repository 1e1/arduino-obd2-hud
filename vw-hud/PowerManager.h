#ifndef PowerManager_H_
#define PowerManager_H_

#include <Arduino.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "_wiring.h"
#include "macro.h"


#ifndef LOWPOWER_IDLE_TIME
#define LOWPOWER_IDLE_TIME T_16MS
#endif
#ifndef LOWPOWER_WAKEUP_BOD_MS
#define LOWPOWER_WAKEUP_BOD_MS 65
#endif
#ifndef LOWPOWER_IDLE_LOOP
#define LOWPOWER_IDLE_LOOP 255
#endif
#ifndef LOWPOWER_CPU_HIGH
#define LOWPOWER_CPU_HIGH F_16MHz
#endif
#ifndef LOWPOWER_CPU_LOW
#define LOWPOWER_CPU_LOW F_2MHz
#endif
#ifndef LOWPOWER_INTERRUPT_PIN
#define LOWPOWER_INTERRUPT_PIN NOT_AN_INTERRUPT
#endif


#define LOWPOWER_WAKEUP_BOD_MS 65


class PowerManager {

  public:
  typedef enum { INT_TIMER, INT_EXTERNAL } Interrupt;

  typedef enum { 
    F_16MHz=clock_div_1, 
    F_8MHz=clock_div_2, 
    F_4MHz=clock_div_4, 
    F_2MHz=clock_div_8, 
    F_1MHz=clock_div_16, 
    F_500Hz=clock_div_32, 
    F_250Hz=clock_div_64, 
    F_125Hz=clock_div_128,
  } Frequency;

  typedef enum { 
    T_16MS=bit(WDIE), 
    T_32MS=bit(WDIE)|bit(WDP0), 
    T_64MS=bit(WDIE)|bit(WDP1), 
    T_125MS=bit(WDIE)|bit(WDP1)|bit(WDP0), 
    T_250MS=bit(WDIE)|bit(WDP2), 
    T_500MS=bit(WDIE)|bit(WDP2)|bit(WDP0), 
    T_1S=bit(WDIE)|bit(WDP2)|bit(WDP1), 
    T_2S=bit(WDIE)|bit(WDP2)|bit(WDP1)|bit(WDP0), 
    T_4S=bit(WDIE)|bit(WDP3), 
    T_8S=bit(WDIE)|bit(WDP3)|bit(WDP1),
  } Wdt;

  __attribute__((always_inline)) inline static void free(void) { power_all_enable(); };

  static void begin(void);
  static void standby(const Wdt timer);
  static void shutdown(void);
  static const bool isFirstLoop(void);
  static void highCpu(void);
  static void lowCpu(void);
  static const unsigned long realMillis(void);

  protected:
  static void _disableComponents(void);
  static void _enableComponents(void);
  static void _disableTimer(void);
  static void _enableTimer(void);
  static void _addUntimedMillis(const Wdt timer);
  static void _sleepTimer(const byte mode=SLEEP_MODE_IDLE, const Wdt timer=LOWPOWER_IDLE_TIME);
  static void _sleepUntilInterrupt(const byte mode=SLEEP_MODE_PWR_DOWN);

  static unsigned long untimed_ms;
};

extern PowerManager Energy;


#endif
