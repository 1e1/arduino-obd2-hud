#ifndef CarEventObd2Can_H_
#define CarEventObd2Can_H_

#include <Arduino.h>
#include <avr/sleep.h>
#include <mcp_can.h>
#include "CarEventObd2.h"


/*
        111 1110 1000 = 0x7E8
  MASK: 111 1110 1000 00 0000 0000 0000 0000
FILTER: 111 1110 1000 00 0000 0000 0000 0000
*/


class CarEventObd2Can : public CarEventObd2 {
    public:
    void setBoard(MCP_CAN* board);

    protected:
    typedef enum {
        PID_TANK_LOAD       = OBD_PID_TANK_LOAD, 
        PID_ODOMETER_VALUE  = OBD_PID_ODOMETER_VALUE, 
        PID_RPM_VALUE       = OBD_PID_RPM_VALUE,
        PID_TORQUE_LOAD     = OBD_PID_TORQUE_LOAD,
        PID_SPEED_VALUE     = OBD_PID_SPEED_VALUE,
        PID_MAF_VALUE       = OBD_PID_MAF_VALUE,
    } _PID;

    void _updateMode(void);
    void _switchOn(void);
    void _switchOff(void);
    void _update(void);

    MCP_CAN* _board = nullptr;

};


#endif