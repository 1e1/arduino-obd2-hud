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
shutdown:
  this->_sleep();                     // DEEP SLEEP: bus + screen off, deep sleep until bus INT

  if (!this->_confirmContact()) {     // IGNITION CONFIRMATION: screen stays OFF until Klemme_15
    goto shutdown;
  }

  if (!this->_boot()) {               // BOOT: acquire fuel + odometer (or timeout -> degraded)
    goto shutdown;
  }

  this->_run();                       // DRIVING / IDLE until ignition off

  goto shutdown;
}


// ==============================================
// PROTECTED
// ==============================================


void Workflow::_sleep(void) const
{
  this->_bus->switchOff();            // MCP2515 -> sleep (wakes the MCU on bus activity)
  this->_display->switchOff();
  Energy.shutdown();                  // PWR_DOWN until the INT pin (bus activity) wakes us
}


bool Workflow::_confirmContact(void) const
{
  // Woken by bus activity. Stay economical and keep the SCREEN OFF until the
  // ignition (Klemme_15) is confirmed; otherwise go back to sleep.
  this->_bus->setMode(CarEvent::MODE_IDLE);
  this->_bus->switchOn();

  for (uint16_t ticks = 0; ticks < CONFIRM_TIMEOUT_TICKS; ++ticks) {
    this->_bus->update();

    switch (this->_bus->getSensor()) {
      case CarEvent::SENSOR_IGNITION:
        if (this->_bus->getIgnition() != 0) {
          return true;                // contact on -> proceed to boot
        }
        break;

      case CarEvent::SENSOR_NONE:
        Energy.standby(PowerManager::T_64MS);
        break;

      default:
        break;
    }
  }

  return false;                       // no contact within the window -> re-sleep
}


bool Workflow::_boot(void) const
{
  // Contact confirmed: the cluster is awake. Acquire fuel level + odometer
  // before handing over to _run; give up (degraded) after a timeout.
  //
  // BOOT SCREEN = "starfield cracktro" (see www/boot-demo.html). Ignition is the
  // baseline (screen on, warp at MIN). Each newly-captured signal raises the warp
  // level, capped below max during build-up. The boot frame is rendered EVERY
  // loop iteration, UNTHROTTLED (no requestAnimationFrame gate, no standby): we
  // trade power for fluidity over this brief phase.
  Energy.highCpu();

  this->_display->setTankCapacity(TANK_CAPACITY_MAX);
  this->_display->setPage(Hudisplay::PAGE_BOOT);
  this->_display->setBootOutcome(Hudisplay::BOOT_OUTCOME_NONE);
  this->_display->setBootWarp(0);
  this->_display->switchOn();          // screen on now (ignition already confirmed)

  // captured-signal weights mirror boot-demo's SIG_WEIGHT / TOTAL_PTS:
  //   fuel:3  odo:3  rpm/hb:3  speed:1  conso:1   (sum = 11)
  // warp byte = min(255, points * 255 / 11), capped below max during build-up.
  uint8_t captured = 0;               // bitfield of SensorFlag-like bits (see below)
  uint8_t points = 0;
  unsigned short rpm = 0;             // raw (rpm*4); 0 => engine off
  uint8_t handbrake = 0;

  uint8_t isWaiting = (1 << Workflow::FLAG_HAS_TANK_LOAD)
                    | (1 << Workflow::FLAG_HAS_ODOMETER_VALUE);
  uint16_t ticks = 0;

  do {
    this->_bus->update();

    switch (this->_bus->getSensor()) {
      case CarEvent::SENSOR_TANK_LOAD:
        this->_display->setTankLoad(this->_bus->getTankLoad());
        bitClear(isWaiting, Workflow::FLAG_HAS_TANK_LOAD);
        if (!bitRead(captured, Workflow::FLAG_HAS_TANK_LOAD)) {
          bitSet(captured, Workflow::FLAG_HAS_TANK_LOAD);
          points += 3;                // fuel +large
        }
        break;

      case CarEvent::SENSOR_ODOMETER:
        this->_display->setOdometerValue(this->_bus->getOdometerValue());
        bitClear(isWaiting, Workflow::FLAG_HAS_ODOMETER_VALUE);
        if (!bitRead(captured, Workflow::FLAG_HAS_ODOMETER_VALUE)) {
          bitSet(captured, Workflow::FLAG_HAS_ODOMETER_VALUE);
          points += 3;                // odo +large
        }
        break;

      case CarEvent::SENSOR_RPM:
        rpm = this->_bus->getRpmValue();
        if (!bitRead(captured, Workflow::FLAG_BOOT_RPM_HB)) {
          bitSet(captured, Workflow::FLAG_BOOT_RPM_HB);
          points += 3;                // rpm/hb +large
        }
        break;

      case CarEvent::SENSOR_HANDBRAKE_POSITION:
        handbrake = this->_bus->getHandbrakePosition();
        if (!bitRead(captured, Workflow::FLAG_BOOT_RPM_HB)) {
          bitSet(captured, Workflow::FLAG_BOOT_RPM_HB);
          points += 3;                // rpm/hb +large
        }
        break;

      case CarEvent::SENSOR_VEHICLE_SPEED:
        if (!bitRead(captured, Workflow::FLAG_BOOT_SPEED)) {
          bitSet(captured, Workflow::FLAG_BOOT_SPEED);
          points += 1;                // speed +small
        }
        break;

      case CarEvent::SENSOR_FUEL_CONSUMPTION:
        if (!bitRead(captured, Workflow::FLAG_BOOT_CONSO)) {
          bitSet(captured, Workflow::FLAG_BOOT_CONSO);
          points += 1;                // conso +small
        }
        break;

      case CarEvent::SENSOR_IGNITION:
        if (this->_bus->getIgnition() == 0) {
          return false;               // contact lost during boot -> sleep
        }
        break;

      case CarEvent::SENSOR_NONE:
        if (++ticks >= BOOT_TIMEOUT_TICKS) {
          isWaiting = 0;              // degraded: hand over without all data
        }
        break;

      default:
        break;
    }

    // warp build-up, capped below max (255 * points/11, but never the climax max)
    {
      uint16_t warp = ((uint16_t)points * 255U) / 11U;
      if (warp > 255U) warp = 255U;
      this->_display->setBootWarp((uint8_t)warp);
    }

    // UNTHROTTLED boot render every iteration (fluidity over power, briefly).
    this->_display->bootFrame();
  } while (isWaiting);

  // --- climax: decide outcome from the live criterion, then show it briefly ---
  // driving = (RPM>0 AND handbrake OFF) ; idle = otherwise
  const Hudisplay::BootOutcome outcome = ((rpm > 0) && (handbrake == 0))
                                       ? Hudisplay::BOOT_OUTCOME_DRIVE
                                       : Hudisplay::BOOT_OUTCOME_IDLE;
  this->_display->setBootOutcome(outcome);

  const unsigned long climaxEndMs = Energy.realMillis() + BOOT_CLIMAX_MS;
  do {
    this->_bus->update();
    if (this->_bus->getSensor() == CarEvent::SENSOR_IGNITION
        && this->_bus->getIgnition() == 0) {
      return false;                   // contact lost during the climax -> sleep
    }
    this->_display->bootFrame();
  } while (Energy.realMillis() < climaxEndMs);

  // hand over to _run: the baseline CPU clock is already full speed (highCpu),
  // so _run manages its own standby cadence; just prime the throttled animation
  // clock + reset the trip baselines for the real page.
  this->_display->switchOn();          // reset trip baselines for the real page
  this->_display->requestAnimationFrame(Energy.realMillis());

  return true;
}


void Workflow::_run(void) const
{
  // One filter set (driving superset) covers both driving and idle; only the
  // page and the standby cadence change with the criterion.
  this->_bus->setMode(CarEvent::MODE_DRIVING);
  this->_bus->switchOn();

  unsigned short rpm = 0;             // raw (rpm*4); 0 => engine off
  uint8_t handbrake = 0;
  uint8_t rpmStale = 0;
  Hudisplay::Page page = Hudisplay::PAGE_NONE;

  do {
    this->_bus->update();

    switch (this->_bus->getSensor()) {
      case CarEvent::SENSOR_RPM:
        rpm = this->_bus->getRpmValue();
        rpmStale = 0;
        this->_display->setRpmValue(rpm);
        break;

      case CarEvent::SENSOR_VEHICLE_SPEED:
        this->_display->setSpeedValue(this->_bus->getSpeedValue());
        break;

      case CarEvent::SENSOR_TANK_LOAD:
        this->_display->setTankLoad(this->_bus->getTankLoad());
        break;

      case CarEvent::SENSOR_ODOMETER:
        this->_display->setOdometerValue(this->_bus->getOdometerValue());
        break;

      case CarEvent::SENSOR_FUEL_CONSUMPTION:
        this->_display->setConsumptionCounter(this->_bus->getConsumptionCounter());
        break;

      case CarEvent::SENSOR_GEAR_POSITION:
        this->_display->setGearPosition(this->_bus->getGearPosition());
        break;

      case CarEvent::SENSOR_HANDBRAKE_POSITION:
        handbrake = this->_bus->getHandbrakePosition();
        break;

      case CarEvent::SENSOR_IGNITION:
        if (this->_bus->getIgnition() == 0) {
          return;                     // ignition off -> back to deep sleep
        }
        break;

      case CarEvent::SENSOR_NONE:
        // no RPM frame for a while => engine considered off (RPM "absent in window")
        if (rpmStale < 0xFF && ++rpmStale >= RPM_ABSENT_TICKS) {
          rpm = 0;
        }
        Energy.standby((page == Hudisplay::PAGE_DRIVING) ? PowerManager::T_16MS : PowerManager::T_32MS);
        break;

      default:
        break;
    }

    // idle = (RPM=0 OR handbrake ON) ; driving = (RPM>0 AND handbrake OFF)
    const Hudisplay::Page wanted = ((rpm > 0) && (handbrake == 0))
                                 ? Hudisplay::PAGE_DRIVING
                                 : Hudisplay::PAGE_IDLE;
    if (wanted != page) {
      page = wanted;
      this->_display->setPage(page);
    }

    this->_display->requestAnimationFrame(Energy.realMillis());
  } while (true);
}
