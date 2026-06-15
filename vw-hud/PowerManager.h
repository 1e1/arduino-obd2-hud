#ifndef PowerManager_H_
#define PowerManager_H_

#include <Arduino.h>
#include "_wiring.h"
#include "macro.h"

// -----------------------------------------------------------------------------
// PowerManager is dual-pathed by architecture:
//
//   ARDUINO_ARCH_AVR (classic ATmega328) -- REAL deep-sleep: avr/wdt.h +
//     avr/sleep.h, WDT interrupt timing, INT0 wake. This path is the BASELINE
//     and must stay byte-identical (Uno: flash 23460 B, RAM 1420 B).
//
//   every other arch (megaavr / SAMD / RP2040 / ESP32) -- a PORTABLE FALLBACK
//     that only has to COMPILE: standby() = a plain timed delay, shutdown() =
//     no-op, realMillis() = millis(). Deep-sleep DEPTH is a follow-up; each
//     fallback carries a TODO naming the real per-arch mechanism. Selected by
//     VH_DEEP_SLEEP==0 in _wiring.h.
//
// The public API and the Wdt/Frequency/Interrupt enums are IDENTICAL on every
// arch so Workflow/Hudisplay (which call Energy.standby(PowerManager::T_*),
// highCpu(), realMillis(), ...) compile unchanged everywhere.
// -----------------------------------------------------------------------------

#if VH_DEEP_SLEEP
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#endif


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


class PowerManager {

  public:
  typedef enum { INT_TIMER, INT_EXTERNAL } Interrupt;

#if VH_DEEP_SLEEP
  // Classic-AVR: the Frequency values ARE the clock_prescale divider codes and
  // the Wdt values ARE the WDTCSR register bit-patterns (used as-is in HW regs).
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
#else
  // Portable fallback: the enums are plain tokens (no HW register meaning).
  // Wdt values are the corresponding durations in MILLISECONDS so the fallback
  // standby() can use them directly as a delay/time accounting unit.
  typedef enum {
    F_16MHz, F_8MHz, F_4MHz, F_2MHz, F_1MHz, F_500Hz, F_250Hz, F_125Hz,
  } Frequency;

  typedef enum {
    T_16MS=16,
    T_32MS=32,
    T_64MS=64,
    T_125MS=125,
    T_250MS=250,
    T_500MS=500,
    T_1S=1000,
    T_2S=2000,
    T_4S=4000,
    T_8S=8000,
  } Wdt;
#endif

  __attribute__((always_inline)) inline static void free(void) {
#if VH_DEEP_SLEEP
    power_all_enable();
#endif
  };

  static void begin(void);
  static void standby(const Wdt timer);
  static void shutdown(void);
  static bool isFirstLoop(void);
  static void highCpu(void);
  static void lowCpu(void);
  static unsigned long realMillis(void);

  protected:
#if VH_DEEP_SLEEP
  static void _disableComponents(void);
  static void _enableComponents(void);
  static void _disableTimer(void);
  static void _enableTimer(void);
  static void _addUntimedMillis(const Wdt timer);
  static void _sleepTimer(const byte mode=SLEEP_MODE_IDLE, const Wdt timer=LOWPOWER_IDLE_TIME);
  static void _sleepUntilInterrupt(const byte mode=SLEEP_MODE_PWR_DOWN);
#endif

  static unsigned long untimed_ms;
};

extern PowerManager Energy;


#endif
