#include "Workflow.h"



// ==============================================
// PUBLIC
// ==============================================


void Workflow::setCarEventObd2(CarEventObd2* bus)
{
  this->_bus = bus;
}


void Workflow::setHudisplay(Hudisplay* display)
{
  this->_display = display;
}


void Workflow::update(void)
{
  switch (this->_state) {
    case Workflow::STATE_SHUTDOWN:
      _sleep();

    case Workflow::STATE_BOOT:
      _wakeup();

    case Workflow::STATE_IDLE: 
      this->_state = _idle() ? Workflow::STATE_DRIVE : Workflow::STATE_SHUTDOWN;
      break;

    case Workflow::STATE_DRIVE:
      if (!_drive()) {
        this->_state = Workflow::STATE_IDLE;
      }
      break;
  }
}


// ==============================================
// PROTECTED
// ==============================================


void Workflow::_wakeup(void)
{
  const uint8_t hasTankCapacityBit  = 0;
  const uint8_t hasTankLoad         = 1;
  const uint8_t hasOdometerValueBit = 2;
  uint8_t isWaiting = (1 << hasTankCapacityBit)
                    | (1 << hasTankLoad)
                    | (1 << hasOdometerValueBit)
                    ;
                    
  this->_bus->setMode(CarEventObd2::MODE_IDLE);
  this->_bus->switchOn();

  do {
    this->_bus->update();
    
    switch (this->_bus->getSensor()) {
      // TODO REMOVE hack
      case CarEventObd2::SENSOR_TANK_CAPACITY: 
        this->_display->setTankCapacity(this->_bus->getTankCapacity());
        bitClear(isWaiting, hasTankCapacityBit);
        break;

      case CarEventObd2::SENSOR_TANK_LOAD: 
        this->_display->setTankLoad(this->_bus->getTankLoad());
        bitClear(isWaiting, hasTankLoad);
        break;

      case CarEventObd2::SENSOR_ODOMETER: 
        this->_display->setOdometerValue(this->_bus->getOdometerValue());
        bitClear(isWaiting, hasOdometerValueBit);
        break;
    }
  } while (isWaiting);
  
  this->_display->switchOn();
}


void Workflow::_sleep(void)
{
  this->_bus->switchOff();
  this->_display->switchOff();

  // registerInterrupt()
  // deepSleep()
}


const bool Workflow::_idle(void)
{
  this->_bus->setMode(CarEventObd2::MODE_IDLE);
  this->_display->setPage(Hudisplay::PAGE_IDLE);

  // drawStats()
  this->_display->requestAnimationFrame(true);
  do {
    this->_bus->update();
    
    switch (this->_bus->getSensor()) {
      case CarEventObd2::SENSOR_RPM: 
        return true;

      /*
      case CarEventObd2::MSG_KEY_POSITION: 
        return STATE_SHUTDOWN;
      */
    }

    // waitUntilEvent
    // | rpm > 0 )) _start()
    // | key(off))) _sleep()
    // | deepSleep(1s)
  } while (false);
}

const bool Workflow::_drive(void)
{
  this->_bus->setMode(CarEventObd2::MODE_DRIVING);
  this->_display->setPage(Hudisplay::PAGE_DRIVING);

  do {
    this->_bus->update();

    switch (this->_bus->getSensor()) {
      case CarEventObd2::SENSOR_TANK_LOAD: 
        this->_display->setTankLoad(this->_bus->getTankLoad());
        break;

      case CarEventObd2::SENSOR_ODOMETER: 
        this->_display->setOdometerValue(this->_bus->getOdometerValue());
        break;

      case CarEventObd2::SENSOR_RPM: 
        this->_display->setRpmValue(this->_bus->getRpmValue());

        // TODO REMOVE hack
        if (0 == this->_bus->getRpmValue()) {
          return false;
        }
        break;

      case CarEventObd2::SENSOR_VEHICLE_SPEED: 
        this->_display->setSpeedValue(this->_bus->getSpeedValue());
        break;
      /*
      case CarEventObd2::SENSOR_FUEL_CONSUMPTION: 
        this->_display->setMafValue(this->_bus->getMafValue());
        break;
      */
    }

    this->_display->requestAnimationFrame();
  } while (true);

  return true;

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
