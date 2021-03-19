#ifndef Hudisplay_H_
#define Hudisplay_H_

#include <Arduino.h>


#ifndef HUDISPLAY_HISTORY_FRAME_SIZE
//#define HUDISPLAY_HISTORY_FRAME_SIZE 50
#define HUDISPLAY_HISTORY_FRAME_SIZE 32
#endif
#ifndef HUDISPLAY_FRAME_DURATION_MS
//#define HUDISPLAY_FRAME_DURATION_MS 40
#define HUDISPLAY_FRAME_DURATION_MS 64
#endif


class Hudisplay {
    public:

    typedef enum { 
        PAGE_NONE, 
        PAGE_IDLE, 
        PAGE_DRIVING, 
    } Page;

    struct MinMax
    {
        uint8_t min = 0;
        uint8_t max = 0;
    };

    void setPage(const Page page);
    void switchOn(void);
    void switchOff(void);

    void setTankCapacity(const uint8_t current /* l: 0-255 */);
    void setTankLoad(const uint8_t current /* % base 255: 0-255 */);
    void setOdometerValue(const unsigned long current /* hm: 0-2**16 */);
    void setRpmValue(const unsigned short current); /*  */
    //void setTorqueLoad(const uint8_t current); /* base 255: x-125 [-125-130] */
    void setSpeedValue(const uint8_t current);
    void setFuelConsumptionValue(const uint8_t current);
    //void setMafValue(const unsigned short current); /* gr/s: x/100 = [0-655.35] */

    const bool requestAnimationFrame(const bool force=false);

    protected:
    void _reset(void);
    const uint8_t _getTankCapacity(void);
    const uint8_t _getTankLoad(void);
    const unsigned long _getOdometerValue(void);
    const uint8_t _getRpmValueInDiv64(void);
    const uint8_t _getSpeedValue(void);
    const uint8_t _getFuelConsumptionValue(void);
    void _insertValueIntoCurrentMinMax(unsigned long value, MinMax* minMax);
    const uint8_t _retrieveMin(MinMax* minMax);
    const uint8_t _retrieveMin(uint8_t* valueList);
    const uint8_t _retrieveMax(MinMax* minMax);
    const uint8_t _retrieveMax(uint8_t* valueList);
    const unsigned long _getDistance(void);
    const unsigned short _millisIn4s(void);
    const unsigned short _getDuration();
    const uint8_t _getTankValue(void);
    const uint8_t _getConsumptionLoad(void);
    const uint8_t _getConsumptionInL(void);
    const uint8_t _fuelLoadToL(const uint8_t load);

    const float _getAverageConsumptionInDlp100km(void);
    const uint8_t _getAverageSpeedInKmph(void);

    virtual void _switchOff(void) =0;
    virtual void _switchOn(void) =0;
    virtual void _update(void) =0;

    Page _page;
    uint8_t _frameIndex;
    unsigned short _firstMillisIn4s;
    unsigned long _nextFrameTimeMs;
    uint8_t _firstTankLoad;
    unsigned long _firstOdometerValue;      // ODB: 4-bytes
    uint8_t _currentTankCapacity;           // ODB: --
    uint8_t _currentTankLoad;               // ODB: 1-byte
    unsigned long _currentOdometerValue;    // ODB: 4-bytes
    //unsigned short _currentRpmValueInDiv64; // ODB: 2-bytes
    //uint8_t _currentSpeedValue;             // ODB: 1-byte
    //uint8_t _currentFuelConsumptionValue;   // ODB: --
    //MinMax _rpmList[HUDISPLAY_HISTORY_FRAME_SIZE];
    //MinMax _speedList[HUDISPLAY_HISTORY_FRAME_SIZE];    // TODO single list (max)
    //MinMax _consumptionList[HUDISPLAY_HISTORY_FRAME_SIZE];
    uint8_t _rpmList[HUDISPLAY_HISTORY_FRAME_SIZE];
    uint8_t _speedList[HUDISPLAY_HISTORY_FRAME_SIZE];    // TODO single list (max)
    uint8_t _consumptionList[HUDISPLAY_HISTORY_FRAME_SIZE];
};


#include "Hudisplay128x64.h"
#include "HudisplayStream.hpp"

#endif