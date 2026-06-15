#ifndef Hudisplay128x64_H_
#define Hudisplay128x64_H_

#include <Arduino.h>
#include <U8g2lib.h>
#include "Hudisplay.h"
#include "_constants.h"
#include "_wiring.h"


#define HUD_DISPLAY128X64_AREA_BLINK 0

// Boot starfield: integer/fixed-point only (no float in the hot loop).
// Star count tuned for the 328P RAM budget: BOOT_STARS * 5 bytes.
#define HUD_BOOT_STARS 40


class Hudisplay128x64 : public Hudisplay {

  public:
  void setBoard(U8G2* board);

  protected:
  const uint8_t _speedMax_delta = 1;

  void _switchOn(void);
  void _switchOff(void);
  void _setNightMode(const bool isNight);
  void _update(void);
  void _bootFrame(void);

  void _drawRpm(void);
  void _drawStats(void);
  void _drawSpeed(void);
  void _drawMaxSpeed(void);
  void _drawGearPosition(void);
  void _drawTrip(void);
  void _drawTank(void);

  // void _drawBootLogo(void); // if too long // drawXBM(x,y,w,h,arr)

  // ---- boot starfield state (positions) ----
  struct BootStar {
    int8_t   x;     // [-127,127] ~ [-1,1]
    int8_t   y;     // [-127,127] ~ [-1,1]
    uint16_t z;     // Q8 fixed point: 256 == 1.0 (depth)
  };
  BootStar _bootStars[HUD_BOOT_STARS];
  bool     _bootStarsInit = false;
  uint16_t _bootVel = 0;       // Q8 z-decrement per frame (smoothed toward target)
  uint32_t _bootRng = 0x1234ABCDUL;

  uint16_t _bootRand(void);          // tiny xorshift PRNG (no <random>, no float)
  void _bootSpawn(BootStar* s, const bool anyZ);
  void _bootDrawStarfield(void);     // per-band draw inside firstPage/nextPage

  // Frame-constant scalars hoisted OUT of the firstPage/nextPage band loop
  // (VH_DISPLAY_FULLBUFFER==0): the draw code runs once per 16px band (~4x/frame),
  // so these are computed once in _update() and only READ by the per-band draws.
  // They depend solely on per-frame-constant state -> byte-identical output.
  uint16_t _cachedFirstFuelPx = 0;     // hudmath_fuel_px(_firstTankLoad, ...)
  uint16_t _cachedCurrentFuelPx = 0;   // hudmath_fuel_px(_getTankLoad(), ...)
  uint8_t  _cachedConsMin = 0;         // _retrieveMin(_consumptionList)
  uint8_t  _cachedConsMax = 0;         // _retrieveMax(_consumptionList)
  void _cacheFrameScalars(void);       // recompute the cache once per frame

  uint8_t _offsetLabel2(const uint8_t value, const uint8_t charWidth);
  uint8_t _offsetLabel3(const uint8_t value, const uint8_t charWidth);
  uint8_t _offsetLabel4(const unsigned short value, const uint8_t charWidth);
  uint8_t _offsetLabel6(const unsigned long value, const uint8_t charWidth);

  U8G2* _board = nullptr;

};


#endif
