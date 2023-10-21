#include "CarEventCan.h"



// ==============================================
// PUBLIC
// ==============================================


void CarEventCan::setBoard(MCP_CAN* board)
{
    this->_board = board;
    this->_board->setSleepWakeup(1);
}


// ==============================================
// PROTECTED
// ==============================================


void CarEventCan::_updateMode(void)
{
    switch (this->_mode) {
        case CarEventCan::MODE_IDLE:
            /**
              * match:
              * --
              * SENSOR_RPM > 0 => drive             xx 01 0C xx xx xx xx
              * SENSOR_GEAR_POSITION = 0 => sleep   xx 01 ?? xx xx xx xx
              *                              MASK = 07 E8 00 00
              *                              MASK = 07 FF FF 00
              * --
              * SENSOR_TANK_LOAD                    xx 01 2F xx xx xx xx
              * SENSOR_ODOMETER                     xx 01 A6 xx xx xx xx
              * SENSOR_TANK_CAPACITY                xx ?? ?? xx xx xx xx
              * SENSOR_HANDBRAKE_POSITION           xx ?? ?? xx xx xx xx
              *                              MASK = 07 E8 00 00
              *                              MASK = 07 FF FF 00
              * --
              */
            this->_board->init_Mask(0,0x07FFFF00);
            this->_board->init_Filt(0,0x00010C00);
            //this->_board->init_Filt(1,0x0001??00);
            // --
            this->_board->init_Mask(1,0x07FFFF00);
            this->_board->init_Filt(2,0x00012F00);
            this->_board->init_Filt(3,0x0001A600);
            //this->_board->init_Filt(4,0x0001??00);
            //this->_board->init_Filt(5,0x0001??00);
            break;
        case CarEventCan::MODE_DRIVING:
            /**
              * match:
              * --
              * SENSOR_RPM = 0 => idle  xx 01 0C xx xx xx xx
              * SENSOR_VEHICLE_SPEED    xx 01 0D xx xx xx xx
              *                  MASK = 07 E8 00 00
              *                  MASK = 07 FF FF 00
              * --
              * SENSOR_TANK_LOAD        xx 01 2F xx xx xx xx
              * SENSOR_ODOMETER         xx 01 A6 xx xx xx xx
              * SENSOR_MAF              xx 01 10 xx xx xx xx
              * SENSOR_GEAR_POSITION    xx 01 ?? xx xx xx xx
              *                  MASK = 07 E8 00 00
              *                  MASK = 07 FF FF 00
              * --
              */
            this->_board->init_Mask(0,0x07FFFF00);
            this->_board->init_Filt(0,0x00010C00);
            this->_board->init_Filt(1,0x00010D00);
            // --
            this->_board->init_Mask(1,0x07FFFF00);
            this->_board->init_Filt(2,0x00012F00);
            this->_board->init_Filt(3,0x0001A600);
            this->_board->init_Filt(4,0x00011000);
            //this->_board->init_Filt(5,0x0001??00);
            break;
    }
}


void CarEventCan::_switchOn(void)
{
    this->_bufferLength = 0;
    //this->_board->mcp2515.reset();
    this->_board->setMode(MCP_LISTENONLY);
}


void CarEventCan::_switchOff(void)
{
    this->_board->setMode(MCP_SLEEP);
}


void CarEventCan::_update(void)
{
    //if(!digitalRead(VH_INT_CANBUS)) {
        if (CAN_OK == this->_board->readMsgBuf(&this->_rxId, &this->_bufferLength, this->_buffer)) {
            const byte pid = this->_buffer[2];

            switch (pid) {
                case PID_TANK_LOAD:
                    this->_sensor = SENSOR_TANK_LOAD;
                    break;
                case PID_ODOMETER_VALUE:
                    this->_sensor = SENSOR_ODOMETER;
                    break;
                case PID_RPM_VALUE:
                    this->_sensor = SENSOR_RPM;
                    break;
                case PID_TORQUE_LOAD:
                    this->_sensor = SENSOR_TORQUE_LOAD;
                    break;
                case PID_SPEED_VALUE:
                    this->_sensor = SENSOR_VEHICLE_SPEED;
                    break;
                case PID_MAF_VALUE:
                    this->_sensor = SENSOR_MAF;
                    break;
                case PID_ENGINE_FUEL_RATE:
                    this->_sensor = SENSOR_ENGINE_FUEL_RATE;
                    break;
            }
        }
    //}
}


const uint8_t CarEventCan::_readByte(void) const
{
    return this->_buffer[3];
}


const unsigned short CarEventCan::_readShort(void) const
{
    return  (this->_buffer[3] << 8)
        |   (this->_buffer[4] << 0)
        ;
}


const unsigned long CarEventCan::_readLong(void) const
{
    return  (this->_buffer[3] << 24)
        |   (this->_buffer[4] << 16)
        |   (this->_buffer[5] <<  8)
        |   (this->_buffer[6] <<  0)
        ;
}
