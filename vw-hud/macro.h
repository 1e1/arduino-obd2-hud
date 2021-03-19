#ifndef macro_H_
#define macro_H_



/** DEBUGGING TOOLS **/
#if HUD_LOG_LEVEL
  #define LOG_START() Serial.begin(9600)
  #define LOG(...)    Serial.print  (__VA_ARGS__)
  #define LOGLN(...)  Serial.println(__VA_ARGS__); Serial.flush()
  #define WAIT(ms)    delay(ms)
#else
  #define LOG_START()
  #define LOG(...)
  #define LOGLN(...)
  #define WAIT(ms)
#endif
/** === **/



#if HUD_USE_LED
  #if defined(LED_BUILTIN) && !defined(HUD_LOG_LED)
    #define HUD_LOG_LED LED_BUILTIN
  #endif
#endif

#ifdef HUD_LOG_LED
  #define BUSYLED_INIT pinMode(HUD_LOG_LED, OUTPUT)
  #define BUSYLED_ON   digitalWrite(HUD_LOG_LED, LOW)
  #define BUSYLED_OFF  digitalWrite(HUD_LOG_LED, HIGH)
#else
  #define BUSYLED_INIT
  #define BUSYLED_ON
  #define BUSYLED_OFF
#endif



/** FAST TRIGO **/
#define FACTOR_PI_UINT12(x) byte((50*x) >> 4)
#define FACTOR_PI_UINT8(x)  byte((804*x) >> 8)
/** === **/


#define SOFTWARE_RESET    asm volatile ("jmp 0")


#endif // macro_H_
