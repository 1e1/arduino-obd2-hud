#ifndef CarEvent_H_
#define CarEvent_H_

#include <Arduino.h>
#include "_constants.h"


#ifndef HUDISPLAY_TIMEOUT_COUNTER
#define HUDISPLAY_TIMEOUT_COUNTER 0
#endif


//using pid_t = byte;
//typedef byte pid_t;
//BOOST_STRONG_TYPEDEF(byte, pid_t)


class CarEvent {
    public:
    static const uint8_t TIMEOUT_COUNTER_MAX = HUDISPLAY_TIMEOUT_COUNTER;

    typedef enum {
        MODE_NONE,
        MODE_IDLE,
        MODE_DRIVING,
        MODE_DRIVING_SPORT,
    } Mode;

    typedef enum {
        SENSOR_NONE,
        SENSOR_TIMEOUT,
        SENSOR_TANK_CAPACITY,
        SENSOR_TANK_LOAD,
        SENSOR_ODOMETER,
        SENSOR_RPM,
        SENSOR_VEHICLE_SPEED,
        SENSOR_FUEL_CONSUMPTION,
        SENSOR_HANDBRAKE_POSITION,
        SENSOR_GEAR_POSITION,
        SENSOR_IGNITION,
    } Sensor;
    const uint8_t SENSOR_COUNT = 11;


    Sensor getSensor(void) const;
    void setMode(const Mode mode);
    void switchOn(void);
    void switchOff(void);

    uint8_t getTankCapacity(void) const; // l: 0-255
    uint8_t getTankLoad(void) const; // native CAN: litres 0-126 (was %255 on OBD)
    unsigned long getOdometerValue(void) const; // native CAN: km integer (was hm on OBD)
    unsigned short getRpmValue(void) const; // raw = rpm*4
    uint8_t getSpeedValue(void) const; // km/h
    //void getKeyPosition(void) const;
    uint8_t getHandbrakePosition(void) const;
    uint8_t getGearPosition(void) const;
    uint8_t getIgnition(void) const; // Klemme 15: 0/1
    unsigned short getConsumptionCounter(void) const; // native: MO5_Verbrauch raw µl (15-bit, rolling)

    void update(void);

    protected:
    virtual void _updateMode(void) =0;
    virtual void _switchOn(void) =0;
    virtual void _switchOff(void) =0;
    virtual void _update(void) =0;

    virtual uint8_t _readByte(void) const =0;
    virtual unsigned short _readShort(void) const =0;
    virtual unsigned long _readLong(void) const =0;

    Mode _mode;
    Sensor _sensor;
    uint8_t _timeoutCounter;
};


#include "CarEventCan.h"
#include "CarEventCanVwPq.h"
#include "CarEventTwaiVwPq.h"   // ESP32 native-TWAI backend (compiled only under ARDUINO_ARCH_ESP32)
#include "CarEventStream.hpp"
#include "CarEventHardware.hpp"

#endif
