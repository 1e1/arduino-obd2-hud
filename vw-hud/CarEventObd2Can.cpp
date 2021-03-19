//#include <mcp_can_dfs.h>
#include "CarEventObd2Can.h"



// ==============================================
// PUBLIC
// ==============================================


void CarEventObd2Can::setBoard(MCP_CAN* board)
{
    this->_board = board;
}


// ==============================================
// PROTECTED
// ==============================================


void CarEventObd2Can::_updateMode(void)
{
    switch (this->_mode) {
        case CarEventObd2Can::MODE_IDLE:
            // TODO
            //this->_board->init_Mask(0,0x03FE0000);
            //this->_board->init_Filt(0,0x03FE0000);
            break;
        case CarEventObd2Can::MODE_DRIVING:
            // TODO
            //this->_board->init_Mask(0,0x03FE0000);
            //this->_board->init_Filt(0,0x03FE0000);
            break;
    }
}


void CarEventObd2Can::_switchOn(void)
{
    this->_sensor = CarEventObd2Can::SENSOR_NONE;
    this->_bufferLength = 0;
    //this->_board->mcp2515.reset();
    //this->_board->setMode(MCP_LISTENONLY);
}


void CarEventObd2Can::_switchOff(void)
{
    //this->_board->setMode(MCP_SLEEP);
}


void CarEventObd2Can::_update(void)
{
    this->_sensor = CarEventObd2Can::SENSOR_NONE; // TODO REMOVE
    this->_bufferLength = 0;
}
