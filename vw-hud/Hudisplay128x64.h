#ifndef Hudisplay128x64_H_
#define Hudisplay128x64_H_

#include <Arduino.h>
#include <U8g2lib.h>
#include "Hudisplay.h"
#include "_wiring.h"


#define HUD_DISPLAY128X64_AREA_BLINK 0


class Hudisplay128x64 : public Hudisplay {

  public:
  void setBoard(U8G2* board);

  protected:
  const uint8_t _speedMax_delta = 1;

  void _switchOn(void);
  void _switchOff(void);
  void _update(void);

  void _drawRpm(void);
  void _drawStats(void);
  void _drawSpeed(void);
  void _drawMaxSpeed(void);
  void _drawGearPosition(void);
  void _drawTrip(void);
  void _drawTank(void);

  // void _drawBootLogo(void); // if too long // drawXBM(x,y,w,h,arr)

  const uint8_t _offsetLabel2(const uint8_t value, const uint8_t charWidth);
  const uint8_t _offsetLabel3(const uint8_t value, const uint8_t charWidth);
  const uint8_t _offsetLabel4(const unsigned short value, const uint8_t charWidth);
  const uint8_t _offsetLabel6(const unsigned long value, const uint8_t charWidth);

  U8G2* _board = nullptr;

};


#endif
