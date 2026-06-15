#ifndef Workflow_HPP_
#define Workflow_HPP_

#include <Arduino.h>
#include "CarEvent.h"
#include "Hudisplay.h"
#include "PowerManager.h"


/**
 * State machine (see doc/DESIGN.md):
 *   off       -> (power) -> setup()
 *   deep sleep            : bus + screen off, deep sleep until bus-activity INT
 *   ignition confirmation : woken; screen stays OFF until Klemme_15 confirmed
 *   boot                  : acquire fuel + odometer (timeout -> degraded)
 *   driving / idle        : driven by  driving = ignition AND (RPM>0 AND handbrake OFF)
 *                                       idle    = ignition AND (RPM=0 OR  handbrake ON)
 * Any state returns to deep sleep on Klemme_15 = 0.
 */
class Workflow {

  public:
  typedef enum {
      FLAG_HAS_TANK_LOAD,
      FLAG_HAS_ODOMETER_VALUE,
      // boot-screen "captured-signal" bits (warp build-up; see _boot)
      FLAG_BOOT_RPM_HB,
      FLAG_BOOT_SPEED,
      FLAG_BOOT_CONSO,
  } SensorFlag;

  void setCarEvent(CarEvent* bus);
  void setHudisplay(Hudisplay* display);

  void update(void);

  protected:
  // loop budgets (in "no sensor" ticks); tune on bench
  static const uint16_t CONFIRM_TIMEOUT_TICKS = 64;  // ignition not seen -> re-sleep
  static const uint16_t BOOT_TIMEOUT_TICKS    = 64;  // fuel/odo not seen -> degraded
  static const uint8_t  RPM_ABSENT_TICKS      = 32;  // no RPM frame -> engine considered off
  static const uint16_t BOOT_CLIMAX_MS        = 800; // hold the boot climax (~0.8 s) before _run

  void _sleep(void) const;
  bool _confirmContact(void) const;
  bool _boot(void) const;
  void _run(void) const;

  CarEvent* _bus = nullptr;
  Hudisplay* _display = nullptr;

};


#endif
