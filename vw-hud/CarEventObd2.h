#ifndef CarEventObd2_H_
#define CarEventObd2_H_

#include <Arduino.h>
#include "_constants.h"


//using pid_t = byte;
//typedef byte pid_t;
//BOOST_STRONG_TYPEDEF(byte, pid_t)


class CarEventObd2 {
    public:

    typedef enum { 
        MODE_NONE, 
        MODE_IDLE, 
        MODE_DRIVING,
        MODE_DRIVING_SPORT, 
    } Mode;

    typedef enum { 
        SENSOR_NONE,
        SENSOR_TANK_CAPACITY, 
        SENSOR_TANK_LOAD, 
        SENSOR_ODOMETER,
        SENSOR_RPM, 
        SENSOR_TORQUE_LOAD, 
        SENSOR_VEHICLE_SPEED,
        SENSOR_FUEL_CONSUMPTION, 
        SENSOR_HANDBRAKE_POSITION, 
        SENSOR_GEAR_POSITION,
    } Sensor;
    const uint8_t SENSOR_COUNT = 10;


    const Sensor getSensor(void);
    void setMode(const Mode mode);
    void switchOn(void);
    void switchOff(void);
    
    const uint8_t getTankCapacity(void); // l: 0-255
    const uint8_t getTankLoad(void); // %255: 0-255
    const unsigned long getOdometerValue(void); // hm: 0-2**16
    const unsigned short getRpmValue(void); // 
    const uint8_t getTorqueLoad(void); // base 255: x-125 [-125-130]
    const uint8_t getSpeedValue(void);
    //const uint8_t getFuelConsumptionValue(void);
    const unsigned short getMafValue(void); // gr/s: x/100 = [0-655.35]
    //void getKeyPosition(void)
    //void getHandbrakePosition(void)
    //void getGearPosition(void)

    void update(void);

    protected:
    const uint8_t _readByte(void);
    const unsigned short _readShort(void);
    const unsigned long _readLong(void);

    virtual void _updateMode(void) =0;
    virtual void _switchOn(void) =0;
    virtual void _switchOff(void) =0;
    virtual void _update(void) =0;

    Mode _mode;
    Sensor _sensor;
    unsigned char _buffer[8];
    uint8_t _bufferLength;
};


#include "CarEventObd2Can.h"
#include "CarEventObd2Stream.hpp"
#include "CarEventObd2Hardware.hpp"

#endif