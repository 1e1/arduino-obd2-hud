#ifndef CarEventCan_H_
#define CarEventCan_H_

#include <Arduino.h>
#include <avr/sleep.h>
#include <mcp_can.h>
#include "CarEvent.h"


/*
        111 1110 1000 = 0x7E8
  MASK: 111 1110 1000 00 0000 0000 0000 0000
FILTER: 111 1110 1000 00 0000 0000 0000 0000
*/


class CarEventCan : public CarEvent {
    public:
    void setBoard(MCP_CAN* board);

    protected:
    typedef enum {
        PID_TANK_LOAD           = OBD_PID_TANK_LOAD, 
        PID_ODOMETER_VALUE      = OBD_PID_ODOMETER_VALUE, 
        PID_RPM_VALUE           = OBD_PID_RPM_VALUE,
        PID_TORQUE_LOAD         = OBD_PID_TORQUE_LOAD,
        PID_SPEED_VALUE         = OBD_PID_SPEED_VALUE,
        PID_MAF_VALUE           = OBD_PID_MAF_VALUE,
        PID_ENGINE_FUEL_RATE    = OBD_PID_ENGINE_FUEL_RATE,
        PID_GEAR_POSITION       = OBD_PID_GEAR_POSITION,
    } _PID;

    void _updateMode(void);
    void _switchOn(void);
    void _switchOff(void);
    void _update(void);

    const uint8_t _readByte(void) const;
    const unsigned short _readShort(void) const;
    const unsigned long _readLong(void) const;

    MCP_CAN* _board = nullptr;
    unsigned long _rxId;
    unsigned char _buffer[8];
    uint8_t _bufferLength;

};


#endif