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
        SENSOR_MAF, 
        SENSOR_ENGINE_FUEL_RATE,
        SENSOR_TORQUE_LOAD, 
        SENSOR_VEHICLE_SPEED,
        SENSOR_FUEL_CONSUMPTION, 
        SENSOR_HANDBRAKE_POSITION, 
        SENSOR_GEAR_POSITION,
    } Sensor;
    const uint8_t SENSOR_COUNT = 13;


    const Sensor getSensor(void) const;
    void setMode(const Mode mode);
    void switchOn(void);
    void switchOff(void);
    
    const uint8_t getTankCapacity(void) const; // l: 0-255
    const uint8_t getTankLoad(void) const; // %255: 0-255
    const unsigned long getOdometerValue(void) const; // hm: 0-2**16
    const unsigned short getRpmValue(void) const; // 
    const uint8_t getTorqueLoad(void) const; // base 255: x-125 [-125-130]
    const uint8_t getSpeedValue(void) const;
    const uint8_t getFuelConsumptionValue(void) const;
    const unsigned short getMafValue(void) const; // gr/s: x/100 = [0-655.35]
    const unsigned short getEngineFuelRateValue(void) const; // l/h: x/20 = [0-3212.75]
    //void getKeyPosition(void) const;
    const uint8_t getHandbrakePosition(void) const;
    const uint8_t getGearPosition(void) const;

    void update(void);

    protected:
    virtual void _updateMode(void) =0;
    virtual void _switchOn(void) =0;
    virtual void _switchOff(void) =0;
    virtual void _update(void) =0;

    virtual const uint8_t _readByte(void) const =0;
    virtual const unsigned short _readShort(void) const =0;
    virtual const unsigned long _readLong(void) const =0;

    Mode _mode;
    Sensor _sensor;
    uint8_t _timeoutCounter;
};


#include "CarEventCan.h"
#include "CarEventStream.hpp"
#include "CarEventHardware.hpp"

#endif