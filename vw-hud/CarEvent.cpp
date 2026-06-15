#include "CarEvent.h"



// ==============================================
// PUBLIC
// ==============================================


CarEvent::Sensor CarEvent::getSensor(void) const
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

    // Timeout is opt-in (HUDISPLAY_TIMEOUT_COUNTER > 0). When disabled (==0) the
    // whole block compiles out — avoids the "comparison always false" on MAX==0.
    #if HUDISPLAY_TIMEOUT_COUNTER > 0
    if (this->_sensor == CarEvent::SENSOR_NONE) {
        if (++this->_timeoutCounter < CarEvent::TIMEOUT_COUNTER_MAX) {
            return;
        }
        this->_sensor = CarEvent::SENSOR_TIMEOUT;
    }
    this->_timeoutCounter = 0;
    #endif
}


uint8_t CarEvent::getTankCapacity(void) const
{
    return TANK_CAPACITY_MAX; // TODO
}


uint8_t CarEvent::getTankLoad(void) const
{
    return this->_readByte();
}


unsigned long CarEvent::getOdometerValue(void) const
{
    return this->_readLong();
}


unsigned short CarEvent::getRpmValue(void) const
{
    return this->_readShort();
}


uint8_t CarEvent::getSpeedValue(void) const
{
    return this->_readByte();
}


uint8_t CarEvent::getGearPosition(void) const
{
    return this->_readByte();
}


uint8_t CarEvent::getHandbrakePosition(void) const
{
    return this->_readByte();
}


uint8_t CarEvent::getIgnition(void) const
{
    return this->_readByte();
}


unsigned short CarEvent::getConsumptionCounter(void) const
{
    return this->_readShort();
}


// ==============================================
// PROTECTED
// ==============================================

