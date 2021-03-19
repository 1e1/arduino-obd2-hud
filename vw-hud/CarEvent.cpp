#include "CarEvent.h"



// ==============================================
// PUBLIC
// ==============================================


const CarEvent::Sensor CarEvent::getSensor(void) const
{
    return this->_sensor;
}


void CarEvent::setMode(const Mode mode)
{
    this->_mode = mode;
    this->_updateMode();
}


void CarEvent::switchOn(void)
{
    this->_switchOn();
}


void CarEvent::switchOff(void)
{
    this->_switchOff();
}


void CarEvent::update(void)
{
    this->_sensor = CarEvent::SENSOR_NONE;

    this->_update();

    if (this->_sensor == CarEvent::SENSOR_NONE) {
        ++this->_timeoutCounter;

        if(this->_timeoutCounter < CarEvent::TIMEOUT_COUNTER_MAX) {
            return;
        }
        
        #if HUDISPLAY_TIMEOUT_COUNTER > 0
        this->_sensor = CarEvent::SENSOR_TIMEOUT;
        #endif
    }

    this->_timeoutCounter = 0;
}


const uint8_t CarEvent::getTankCapacity(void) const
{
    return TANK_CAPACITY_MAX; // TODO
}


const uint8_t CarEvent::getTankLoad(void) const
{
    return this->_readByte();
}


const unsigned long CarEvent::getOdometerValue(void) const
{
    return this->_readLong();
}


const unsigned short CarEvent::getRpmValue(void) const
{
    return this->_readShort();
}


const uint8_t CarEvent::getTorqueLoad(void) const
{
    return this->_readByte();
}


const uint8_t CarEvent::getSpeedValue(void) const
{
    return this->_readByte();
}


const uint8_t CarEvent::getFuelConsumptionValue(void) const
{
    return this->_readByte();
}


const unsigned short CarEvent::getMafValue(void) const
{
    return this->_readShort();
}


const unsigned short CarEvent::getEngineFuelRateValue(void) const
{
    return this->_readShort();
}


const uint8_t CarEvent::getGearPosition(void) const
{
    return this->_readByte();
}


const uint8_t CarEvent::getHandbrakePosition(void) const
{
    return this->_readByte();
}


// ==============================================
// PROTECTED
// ==============================================

