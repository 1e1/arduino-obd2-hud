#include "PowerManager.h"




/***********************************************************
 *                       PROPERTIES                        *
 **********************************************************/



unsigned long PowerManager::untimed_ms = 0;



// =============================================================================
// CLASSIC-AVR PATH (real deep-sleep) -- BASELINE, must stay byte-identical.
// =============================================================================
#if VH_DEEP_SLEEP


static const bool _USE_ADC    = false;
static const bool _USE_SPI    = true;
static const bool _USE_MILLIS = true;
static const bool _USE_TWI    = false;
static const bool _USE_USART  = true; // TODO false


#if LOWPOWER_INTERRUPT_PIN != NOT_AN_INTERRUPT
void _ISR_wakeup(void)
{
}

EMPTY_INTERRUPT(WDT_vect);

void attachDefaultInterrupts(const uint8_t mode = FALLING)
{
    attachInterrupt(digitalPinToInterrupt(LOWPOWER_INTERRUPT_PIN), _ISR_wakeup, FALLING);
}

void detachDefaultInterrupts(void)
{
    detachInterrupt(digitalPinToInterrupt(LOWPOWER_INTERRUPT_PIN));
}
#endif




/***********************************************************
 *                         PUBLIC                          *
 **********************************************************/




void PowerManager::begin(void)
{
  for (uint8_t pin = 0; pin < NUM_DIGITAL_PINS; ++pin) {
    pinMode(pin, INPUT_PULLUP);
  }

  PowerManager::_disableComponents();
  PowerManager::_enableComponents();
}


void PowerManager::standby(const Wdt timer)
{
  /*
  SLEEP_MODE_IDLE: 15 mA
  SLEEP_MODE_ADC: 6.5 mA
  SLEEP_MODE_PWR_SAVE: 1.62 mA
  SLEEP_MODE_EXT_STANDBY: 1.62 mA
  SLEEP_MODE_STANDBY : 0.84 mA
  SLEEP_MODE_PWR_DOWN : 0.36 mA
  */
  PowerManager::_sleepTimer(SLEEP_MODE_IDLE, timer);
  //PowerManager::_sleepTimer(SLEEP_MODE_STANDBY, timer);
}


void PowerManager::shutdown(void)
{
  PowerManager::_sleepUntilInterrupt(SLEEP_MODE_PWR_DOWN);

  if (_USE_MILLIS) {
    delay(LOWPOWER_WAKEUP_BOD_MS);
  } else {
    PowerManager::standby(PowerManager::T_64MS);
  }
}


void PowerManager::highCpu(void)
{
  clock_prescale_set((clock_div_t) Frequency::LOWPOWER_CPU_HIGH);
}


void PowerManager::lowCpu(void)
{
  clock_prescale_set((clock_div_t) Frequency::LOWPOWER_CPU_LOW);
}


unsigned long PowerManager::realMillis(void)
{
  return PowerManager::untimed_ms + millis();
}



/***********************************************************
 *                        PROTECTED                        *
 **********************************************************/




void PowerManager::_disableComponents(void)
{
  power_all_disable();
}


void PowerManager::_enableComponents(void)
{
  if (_USE_ADC) power_adc_enable();
  if (_USE_SPI) power_spi_enable();
  if (_USE_TWI) power_twi_enable();

  if (_USE_USART) {
    #if defined(power_usart0_enable)
    power_usart0_enable();
    #elif defined(power_usart1_enable)
    power_usart1_enable();
    #elif defined(power_usart2_enable)
    power_usart2_enable();
    #elif defined(power_usart3_enable)
    power_usart3_enable();
    #endif
  }

  PowerManager::_enableTimer();
}


void PowerManager::_disableTimer(void)
{
  if (_USE_MILLIS) {
    power_timer0_disable(); // delay(), millis(), micro()
    //power_timer1_disable(); // Servo, pwm
    //power_timer2_disable(); // tone(), pwm
  }
}


void PowerManager::_enableTimer(void)
{
  if (_USE_MILLIS) {
    power_timer0_enable(); // delay(), millis(), micro()
    //power_timer1_enable(); // Servo, pwm
    //power_timer2_enable(); // tone(), pwm
  }
}


void PowerManager::_addUntimedMillis(const Wdt timer)
{
  switch (timer) {
    case PowerManager::T_16MS:
      PowerManager::untimed_ms += 1 <<  4;
      break;
    case PowerManager::T_32MS:
      PowerManager::untimed_ms += 1 <<  5;
      break;
    case PowerManager::T_64MS:
      PowerManager::untimed_ms += 1 <<  6;
      break;
    case PowerManager::T_125MS:
      PowerManager::untimed_ms += 1 <<  7;
      break;
    case PowerManager::T_250MS:
      PowerManager::untimed_ms += 1 <<  7;
      break;
    case PowerManager::T_500MS:
      PowerManager::untimed_ms += 1 <<  8;
      break;
    case PowerManager::T_1S:
      PowerManager::untimed_ms += 1 <<  9;
      break;
    case PowerManager::T_2S:
      PowerManager::untimed_ms += 1 << 10;
      break;
    case PowerManager::T_4S:
      PowerManager::untimed_ms += 1 << 11;
      break;
    case PowerManager::T_8S:
      PowerManager::untimed_ms += 1 << 12;
      break;
  }
}


void PowerManager::_sleepTimer(const byte mode, const Wdt timer)
{
  PowerManager::_addUntimedMillis(timer);

  noInterrupts();         // timed sequence follows
  /*if (_USE_ADC)*/ ADCSRA &= ~(1 << ADEN); // disable ADC
  MCUSR = 0;              // clear various "reset" flags

  PowerManager::_disableTimer();

  WDTCSR = bit(WDCE) | bit(WDE);  // allow changes, disable reset
  WDTCSR = timer;                 // set interrupt mode and an interval
  //wdt_reset();                    // pat the dog

  set_sleep_mode(mode);
  sleep_enable();

  // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);

  interrupts();     // guarantees next instruction executed
  sleep_cpu();
  sleep_disable();  // cancel sleep as a precaution

  PowerManager::_enableTimer();

  wdt_disable();
  if (_USE_ADC) ADCSRA |= (1 << ADEN);
}


void PowerManager::_sleepUntilInterrupt(const byte mode)
{
  #if LOWPOWER_INTERRUPT_PIN != NOT_AN_INTERRUPT
  noInterrupts();         // timed sequence follows
  /*if (_USE_ADC)*/ ADCSRA &= ~(1 << ADEN); // disable ADC
  MCUSR = 0;              // clear various "reset" flags
  attachDefaultInterrupts();

  set_sleep_mode(mode);
  sleep_enable();

  // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);

  interrupts();     // guarantees next instruction executed
  sleep_cpu();
  sleep_disable();  // cancel sleep as a precaution

  detachDefaultInterrupts();
  if (_USE_ADC) ADCSRA |= (1 << ADEN);
  #endif
}


// =============================================================================
// PORTABLE FALLBACK PATH (megaavr / SAMD / RP2040 / ESP32).
// Compiles everywhere; deep-sleep DEPTH is a follow-up (TODOs below). No
// <avr/*.h>, no HW registers. standby == a plain timed delay (the Wdt enum
// carries its duration in ms), shutdown == no-op, realMillis == millis().
// =============================================================================
#else


void PowerManager::begin(void)
{
  // Nothing to gate off on the portable path. Real per-arch low-power init
  // (clock gating / peripheral power domains) is a follow-up.
  // TODO(megaavr): configure RTC/PIT as the periodic wake source.
  // TODO(SAMD):    configure GCLK/standby + RTC wake.
  // TODO(RP2040):  no init needed for sleep; dormant uses XOSC + GPIO wake.
  // TODO(ESP32):   no init needed; deep-sleep wake configured at sleep time.
}


void PowerManager::standby(const Wdt timer)
{
  // Fallback "standby" = a plain busy/idle wait for the requested interval.
  // Keeps the frame/window wall-clock cadence correct (Workflow gates on
  // realMillis()) at the cost of NOT actually lowering power yet.
  // TODO(megaavr): sleep in STANDBY/PWR_DOWN with the RTC/PIT as wake source.
  // TODO(SAMD):    __WFI() in STANDBY with the RTC as the periodic wake.
  // TODO(RP2040):  sleep_goto_sleep_for() / low-power sleep for `timer` ms.
  // TODO(ESP32):   light-sleep (esp_sleep_enable_timer_wakeup + light_sleep).
  delay((unsigned long)timer);
}


void PowerManager::shutdown(void)
{
  // Fallback "deep sleep until bus activity" = no-op (the caller loop will just
  // keep polling the CAN board). The board does NOT actually power down yet.
  // TODO(megaavr): PWR_DOWN, wake on the INT pin (PORT interrupt).
  // TODO(SAMD):    deep STANDBY, wake on EXTINT from the MCP2515 INT pin.
  // TODO(RP2040):  XOSC dormant, wake on the GPIO tied to MCP2515 INT.
  // TODO(ESP32):   esp_deep_sleep_start(); wake on GPIO (or TWAI once ported).
}


void PowerManager::highCpu(void)
{
  // TODO(per-arch): raise the core clock if a low/high split is used. The
  // boards on this path already run fast enough that the boot animation is
  // fluid at the default clock, so this is a no-op for now.
}


void PowerManager::lowCpu(void)
{
  // TODO(per-arch): drop the core clock for the idle/confirmation phases.
}


unsigned long PowerManager::realMillis(void)
{
  // No untimed sleep accounting on the fallback path: standby uses delay(), so
  // millis() already advances across the wait. untimed_ms stays 0.
  return PowerManager::untimed_ms + millis();
}


#endif // VH_DEEP_SLEEP
