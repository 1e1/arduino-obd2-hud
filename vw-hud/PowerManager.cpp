#include "PowerManager.h"




/***********************************************************
 *                       PROPERTIES                        *
 **********************************************************/



static const bool _USE_ADC    = false;
static const bool _USE_SPI    = true;
static const bool _USE_MILLIS = true;
static const bool _USE_TWI    = false;
static const bool _USE_USART  = true; // TODO false


unsigned long PowerManager::untimed_ms = 0;


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
  for (uint8_t pin; pin<NUM_DIGITAL_PINS; ++pin) {
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


const unsigned long PowerManager::realMillis(void)
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
