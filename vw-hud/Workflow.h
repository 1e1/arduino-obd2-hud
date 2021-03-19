#ifndef Workflow_HPP_
#define Workflow_HPP_

#include <Arduino.h>
#include "CarEventObd2.h"
#include "Hudisplay.h"


class Workflow {

  public:
  void setCarEventObd2(CarEventObd2* bus);
  void setHudisplay(Hudisplay* display);

  void update(void);

  protected:
  
  typedef enum { 
      STATE_BOOT, 
      STATE_IDLE, 
      STATE_DRIVE,
      STATE_SHUTDOWN, 
  } State;

  void _wakeup(void);
  void _sleep(void);
  const bool _idle(void);
  const bool _drive(void);


  State _state = STATE_BOOT;
  CarEventObd2* _bus = nullptr;
  Hudisplay* _display = nullptr;

};


#endif
