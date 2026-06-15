#ifndef Hudisplay_H_
#define Hudisplay_H_

#include <Arduino.h>
#include "_wiring.h"   // HUD_VIN (board-config) must be visible before the class body


#ifndef HUDISPLAY_HISTORY_FRAME_SIZE
//#define HUDISPLAY_HISTORY_FRAME_SIZE 50
//#define HUDISPLAY_HISTORY_FRAME_SIZE 32
#define HUDISPLAY_HISTORY_FRAME_SIZE 64
#endif
#ifndef HUDISPLAY_FRAME_DURATION_MS
//#define HUDISPLAY_FRAME_DURATION_MS 40
#define HUDISPLAY_FRAME_DURATION_MS 64
#endif


class Hudisplay {
    public:
    static const uint8_t HISTORY_FRAME_SIZE = HUDISPLAY_HISTORY_FRAME_SIZE;
    static const uint8_t FRAME_DURATION_MS = HUDISPLAY_FRAME_DURATION_MS;
    static const uint8_t RPM_SHIFT = 8;

    typedef enum {
        PAGE_NONE,
        PAGE_IDLE,
        PAGE_DRIVING,
        PAGE_BOOT,
    } Page;

    // Boot "starfield cracktro" outcome (climax of the boot animation).
    typedef enum {
        BOOT_OUTCOME_NONE,      // still building up
        BOOT_OUTCOME_DRIVE,     // warp burst + "READY > DRIVE"
        BOOT_OUTCOME_IDLE,      // decelerate/settle + "PARKED"
    } BootOutcome;

    struct MinMax
    {
        uint8_t min = 0;
        uint8_t max = 0;
    };

    void setPage(const Page page);
    void switchOn(void);
    void switchOff(void);
    void setNightMode(const bool isNight);

    #if defined(HUD_VIN)
    void setVin(const char* vin /* up to 17 chars, listen-only from muxed Ident 0x5D2 */);
    #endif
    void setTankCapacity(const uint8_t current /* l: 0-255 */);
    void setTankLoad(const uint8_t current /* native CAN: litres 0-capacity (was %255 on OBD) */);
    void setOdometerValue(const unsigned long current /* native CAN: km integer (was hm on OBD) */);
    void setGearPosition(const uint8_t current);
    void setRpmValue(const unsigned short current); /*  */
    //void setTorqueLoad(const uint8_t current); /* base 255: x-125 [-125-130] */
    void setSpeedValue(const uint8_t current);
    void setFuelConsumptionValue(const uint8_t current); // legacy/OBD; kept for compatibility, now unused
    void setConsumptionCounter(const unsigned short counter); // native: MO5_Verbrauch raw µl (15-bit, rolling)
    //void setMafValue(const unsigned short current); /* gr/s: x/100 = [0-655.35] */

    bool requestAnimationFrame(const unsigned long now);
    void animationFrame(void);

    // ---- boot starfield (UNTHROTTLED: bypasses the FRAME_DURATION_MS gate) ----
    void setBootWarp(const uint8_t level0to255);     // build-up warp level (capped below max by Workflow)
    void setBootOutcome(const BootOutcome outcome);  // none / drive / idle
    void bootFrame(void);                            // update + render one boot frame immediately

    protected:
    void _reset(void);
    uint8_t _getTankCapacity(void) const;
    uint8_t _getTankLoad(void) const;
    unsigned long _getOdometerValue(void) const;
    uint8_t _getGearPosition(void) const;
    uint8_t _getRpmValueInDiv64(void) const;
    uint8_t _getSpeedValue(void) const;
    uint8_t _getFuelConsumptionValue(void) const;
    uint8_t _retrieveMin(uint8_t* valueList) const;
    uint8_t _retrieveMax(uint8_t* valueList) const;
    MinMax _retrieveMinMax(uint8_t* valueList) const;
    unsigned long _getDistance(void) const;
    unsigned short _millisIn4s(void) const;
    unsigned short _getDuration() const;
    uint8_t _getTankValue(void) const;
    uint8_t _getConsumptionLoad(void) const;
    uint8_t _getConsumptionInL(void) const;
    uint8_t _fuelLoadToL(const uint8_t load) const;

    float _getAverageConsumptionInLp10km(void) const;
    float _getFuelBurntInL(void) const; // from µl trip accumulator
    uint8_t _getAverageSpeedInKmh(void) const;

    virtual void _switchOff(void) =0;
    virtual void _switchOn(void) =0;
    virtual void _setNightMode(const bool isNight) =0;
    virtual void _update(void) =0;
    virtual void _bootFrame(void) =0;   // 128x64 renders the starfield

    Page _page;
    #if defined(HUD_VIN)
    char _vin[18] = {0};                // 17-char VIN + NUL; rendered on the idle page (V1)
    #endif
    uint8_t _bootWarp = 0;              // 0..255 build-up level (Workflow caps below max)
    BootOutcome _bootOutcome = BOOT_OUTCOME_NONE;
    uint8_t _frameIndex = 0;
    unsigned short _firstMillisIn4s = 0;
    unsigned long _nextFrameTimeMs = 0;
    uint8_t _firstTankLoad;                 // CAN: litres at trip start
    unsigned long _firstOdometerValue;      // CAN: km at trip start
    uint8_t _currentGearPosition;           // CAN: --
    uint8_t _currentTankCapacity;           // CAN: litres
    uint8_t _currentTankLoad;               // CAN: litres 0-capacity
    unsigned long _currentOdometerValue;    // CAN: km integer

    // MO5_Verbrauch µl accumulators (see can-pivot-decision: zero-drift integer fuel)
    bool _consPrimed;                       // false until first counter sample seeds _lastConsCounter
    unsigned short _lastConsCounter;        // last raw 15-bit µl counter
    unsigned long _tripMicroLitres;         // µl burnt since trip start (F4/A2)
    unsigned long _frameMicroLitres;        // µl burnt within the current frame (instantaneous bar)
    //unsigned short _currentRpmValueInDiv64; // ODB: 2-bytes
    //uint8_t _currentSpeedValue;             // ODB: 1-byte
    //uint8_t _currentFuelConsumptionValue;   // ODB: --
    //MinMax _rpmList[HISTORY_FRAME_SIZE];
    //MinMax _speedList[HISTORY_FRAME_SIZE];    // TODO single list (max)
    //MinMax _consumptionList[HISTORY_FRAME_SIZE];
    MinMax _rpmMinMax;
    uint8_t _rpmList[HISTORY_FRAME_SIZE];
    MinMax _speedMinMax;
    uint8_t _speedList[HISTORY_FRAME_SIZE];    // TODO single list (max)
    //MinMax _consumptionMinMax;
    uint8_t _consumptionList[HISTORY_FRAME_SIZE];
};


#include "Hudisplay128x64.h"
#include "HudisplayStream.hpp"

#endif