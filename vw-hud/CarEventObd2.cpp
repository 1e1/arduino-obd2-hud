#include "CarEventObd2.h"



// ==============================================
// PUBLIC
// ==============================================


const CarEventObd2::Sensor CarEventObd2::getSensor(void)
{
    return this->_sensor;
}


void CarEventObd2::setMode(const Mode mode)
{
    this->_mode = mode;
    this->_updateMode();
}


void CarEventObd2::switchOn(void)
{
    this->_switchOn();
}


void CarEventObd2::switchOff(void)
{
    this->_switchOff();
}


void CarEventObd2::update(void)
{
    // TODO count timeout
    this->_update();
}


const uint8_t CarEventObd2::getTankCapacity(void)
{
    return TANK_CAPACITY_MAX; // TODO
}


const uint8_t CarEventObd2::getTankLoad(void)
{
    return this->_readByte();
}


const unsigned long CarEventObd2::getOdometerValue(void)
{
    return this->_readLong();
}


const unsigned short CarEventObd2::getRpmValue(void)
{
    return this->_readShort();
}


const uint8_t CarEventObd2::getTorqueLoad(void)
{
    return this->_readByte();
}


const uint8_t CarEventObd2::getSpeedValue(void)
{
    return this->_readByte();
}


const unsigned short CarEventObd2::getMafValue(void)
{
    return this->_readShort();
}


// ==============================================
// PROTECTED
// ==============================================


const uint8_t CarEventObd2::_readByte(void)
{
    return this->_buffer[0];
}


const unsigned short CarEventObd2::_readShort(void)
{
    return  (this->_buffer[0] << 8)
        |   (this->_buffer[1] << 0)
        ;
}


const unsigned long CarEventObd2::_readLong(void)
{
    return  (this->_buffer[0] << 24)
        |   (this->_buffer[1] << 16)
        |   (this->_buffer[2] <<  8)
        |   (this->_buffer[3] <<  0)
        ;
}
