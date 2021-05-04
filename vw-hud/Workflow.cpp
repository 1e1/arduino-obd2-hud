#include "Workflow.h"



// ==============================================
// PUBLIC
// ==============================================


void Workflow::setCarEvent(CarEvent* bus)
{
  this->_bus = bus;
}


void Workflow::setHudisplay(Hudisplay* display)
{
  this->_display = display;
}


void Workflow::update(void)
{
  goto boot;

shutdown:
  _sleep();

boot:
  if (!this->_wakeup()) {
    goto shutdown;
  }

idle:
  if (!this->_idle()) {
    goto shutdown;
  }

drive:
  if (!this->_drive()) {
    goto idle;
  }
}


// ==============================================
// PROTECTED
// ==============================================


void Workflow::_sleep(void) const
{
  this->_bus->switchOff();
  this->_display->switchOff();

  // TODO Energy.shutdown();
}


const bool Workflow::_wakeup(void) const
{
  uint8_t isWaiting = (1 << Workflow::FLAG_HAS_TANK_CAPACITY)
                    | (1 << Workflow::FLAG_HAS_TANK_LOAD)
                    | (1 << Workflow::FLAG_HAS_ODOMETER_VALUE)
                    ;
                    
  this->_bus->setMode(CarEvent::MODE_IDLE);
  this->_bus->switchOn();

  do {
    this->_bus->update();
    
    switch (this->_bus->getSensor()) {
      // TODO REMOVE hack
      case CarEvent::SENSOR_TANK_CAPACITY: 
        this->_display->setTankCapacity(this->_bus->getTankCapacity());
        bitClear(isWaiting, Workflow::FLAG_HAS_TANK_CAPACITY);
        break;

      case CarEvent::SENSOR_TANK_LOAD: 
        this->_display->setTankLoad(this->_bus->getTankLoad());
        bitClear(isWaiting, Workflow::FLAG_HAS_TANK_LOAD);
        break;

      case CarEvent::SENSOR_ODOMETER: 
        this->_display->setOdometerValue(this->_bus->getOdometerValue());
        bitClear(isWaiting, Workflow::FLAG_HAS_ODOMETER_VALUE);
        break;

      case CarEvent::SENSOR_NONE:
        Energy.standby(PowerManager::T_64MS);
        break;

      case CarEvent::SENSOR_TIMEOUT: 
        return false;
    }
  } while (isWaiting);
  
  this->_display->setPage(Hudisplay::PAGE_NONE);
  this->_display->requestAnimationFrame(Energy.realMillis());
  this->_display->switchOn();

  return true;
}


const bool Workflow::_idle(void) const
{
  this->_bus->setMode(CarEvent::MODE_IDLE);
  this->_display->setPage(Hudisplay::PAGE_IDLE);

  this->_display->animationFrame();
  do {
    this->_bus->update();
    
    switch (this->_bus->getSensor()) {
      case CarEvent::SENSOR_RPM: 
        this->_display->setRpmValue(this->_bus->getRpmValue());
        return true;
      /*
      case CarEvent::SENSOR_HANDBRAKE_POSITION: 
        if (this->_bus->getHandbrakePosition() == 0) {
          return true;
        }
        break;
      */
      /*
      case CarEvent::MSG_KEY_POSITION: 
        return STATE_SHUTDOWN;
      */

      case CarEvent::SENSOR_NONE: 
        Energy.standby(PowerManager::T_32MS);
        break;

      case CarEvent::SENSOR_TIMEOUT: 
        return false;
    }

    // waitUntilEvent
    // | rpm > 0 )) _start()
    // | key(off))) _sleep()
    // | deepSleep(1s)
  } while (true);
}


const bool Workflow::_drive(void) const
{
  this->_bus->setMode(CarEvent::MODE_DRIVING);
  this->_display->setPage(Hudisplay::PAGE_DRIVING);

  do {
    this->_bus->update();

    switch (this->_bus->getSensor()) {
      case CarEvent::SENSOR_TANK_LOAD: 
        this->_display->setTankLoad(this->_bus->getTankLoad());
        break;

      case CarEvent::SENSOR_ODOMETER: 
        this->_display->setOdometerValue(this->_bus->getOdometerValue());
        break;

      case CarEvent::SENSOR_RPM: 
        this->_display->setRpmValue(this->_bus->getRpmValue());

        // TODO REMOVE hack if Handbrake
        if (0 == this->_bus->getRpmValue()) {
          return false;
        }
        break;

      case CarEvent::SENSOR_VEHICLE_SPEED: 
        this->_display->setSpeedValue(this->_bus->getSpeedValue());
        break;

      case CarEvent::SENSOR_FUEL_CONSUMPTION: 
        this->_display->setFuelConsumptionValue(this->_bus->getFuelConsumptionValue());
        break;
      /*
      case CarEvent::SENSOR_MAF: 
        this->_display->setMafValue(this->_bus->getMafValue());
        break;
      */ /*
      case CarEvent::SENSOR_ENGINE_FUEL_RATE: 
        this->_display->setFuelConsumptionValue(5*this->_bus->getEngineFuelRateValue());
        break;
      */
      case CarEvent::SENSOR_GEAR_POSITION: 
        this->_display->setGearPosition(this->_bus->getGearPosition());
        break;
      case CarEvent::SENSOR_HANDBRAKE_POSITION: 
        if (this->_bus->getHandbrakePosition() != 0) {
          return true;
        }
        break;
      case CarEvent::SENSOR_NONE: 
        Energy.standby(PowerManager::T_16MS);
        break;

      case CarEvent::SENSOR_TIMEOUT: 
        return false;
    }

    this->_display->requestAnimationFrame(Energy.realMillis());
  } while (true);

  // fuel consumption
  /*
    l/100km = k * ratio * MAF(gr/s) / (100* VSS(km/h))
            = k * ratio * 3600.MAF(gr/h) / (100* VSS(km/h))
            = k(l/gr) * ratio(fuel/air) * 3600.MAF(gr/h) / (100* VSS(km/h))

    EngineFuelRate: L/h
    VSS: km/h
    => 100*EFR/VSS
  */


  // waitUntilEvent
  // | swith(CAN.code)
  // | | rpm=0 )) _terminate()
  // | | *)) ...
}
