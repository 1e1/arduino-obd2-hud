#ifndef Workflow_HPP_
#define Workflow_HPP_

#include <Arduino.h>
#include "CarEvent.h"
#include "Hudisplay.h"
#include "PowerManager.h"


class Workflow {

  public:
  typedef enum { 
      FLAG_HAS_TANK_CAPACITY, 
      FLAG_HAS_TANK_LOAD, 
      FLAG_HAS_ODOMETER_VALUE,
  } SensorFlag;

  void setCarEvent(CarEvent* bus);
  void setHudisplay(Hudisplay* display);

  void update(void);

  protected:

  typedef enum { 
      STATE_BOOT, 
      STATE_IDLE, 
      STATE_DRIVE,
      STATE_SHUTDOWN, 
  } State;

  void _sleep(void) const;
  const bool _wakeup(void) const;
  const bool _idle(void) const;
  const bool _drive(void) const;


  CarEvent* _bus = nullptr;
  Hudisplay* _display = nullptr;

};


#endif
