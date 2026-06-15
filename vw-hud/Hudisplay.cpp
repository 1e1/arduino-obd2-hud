#include "Hudisplay.h"
#include "_hudmath.h"


// ==============================================
// PUBLIC
// ==============================================


void Hudisplay::setPage(const Page page)
{
    this->_page = page;
}


void Hudisplay::switchOn()
{
    this->_reset();
    this->_firstMillisIn4s      = this->_millisIn4s();
    this->_firstTankLoad        = this->_currentTankLoad;
    this->_firstOdometerValue   = this->_currentOdometerValue;

    // zero the µl accumulators; _consPrimed=false so the first counter sample
    // only seeds _lastConsCounter (its delta is discarded, not garbage).
    this->_consPrimed       = false;
    this->_lastConsCounter  = 0;
    this->_tripMicroLitres  = 0;
    this->_frameMicroLitres = 0;

    this->_switchOn();
}


void Hudisplay::switchOff()
{
    this->_switchOff();
}


void Hudisplay::setNightMode(const bool isNight)
{
    this->_setNightMode(isNight);
}


#if defined(HUD_VIN)
void Hudisplay::setVin(const char* vin)
{
    // store up to 17 chars, NUL-terminated (out-of-scope: reading the muxed Ident frame)
    strncpy(this->_vin, vin, sizeof(this->_vin) - 1);
    this->_vin[sizeof(this->_vin) - 1] = '\0';
}
#endif


void Hudisplay::setTankCapacity(const uint8_t current)
{
    this->_currentTankCapacity = current;
}


void Hudisplay::setTankLoad(const uint8_t current)
{
    // native CAN: litres 0..capacity (was %255 on OBD)
    this->_currentTankLoad = current;
}


void Hudisplay::setOdometerValue(const unsigned long current)
{
    this->_currentOdometerValue = current;
}


void Hudisplay::setGearPosition(const uint8_t current)
{
    this->_currentGearPosition = current;
}


void Hudisplay::setRpmValue(const unsigned short current)
{
    // rpm = current/4: [0-16,383.75]
    // 255*16 = 16,320
    // rpm = k * (current >> 8) [0-511]
    // k = 32
    // px = rawRpm >> RPM_SHIFT, clamped to the bar width (history list is uint8_t,
    // so the clamp also keeps the stored value in range).
    const uint8_t rpmValueInDiv64 = hudmath_rpm_px(current, this->RPM_SHIFT, 0xFF);

    //this->_currentRpmValueInDiv64 = rpmValueInDiv64;

    //this->_insertValueIntoCurrentMinMax(rpmValueInDiv64, &this->_rpmList[this->_frameIndex]);
    
    this->_rpmList[this->_frameIndex] = rpmValueInDiv64;
}


void Hudisplay::setSpeedValue(const uint8_t current)
{
    //this->_currentSpeedValue = current;

    //this->_insertValueIntoCurrentMinMax(current, &this->_speedList[this->_frameIndex]);
    
    this->_speedList[this->_frameIndex] = current;
}


void Hudisplay::setFuelConsumptionValue(const uint8_t current)
{
    //this->_currentFuelConsumptionValue = current;

    //this->_insertValueIntoCurrentMinMax(current, &this->_consumptionList[this->_frameIndex]);
    
    this->_consumptionList[this->_frameIndex] = current;
}


void Hudisplay::setConsumptionCounter(const unsigned short counter)
{
    // MO5_Verbrauch is a 15-bit rolling µl counter. On the first sample after
    // switchOn()/reset() we only seed _lastConsCounter (delta would be garbage).
    if (!this->_consPrimed) {
        this->_lastConsCounter = counter;
        this->_consPrimed = true;
        return;
    }

    // exact 15-bit unsigned wrap-around difference (sampled << wrap period -> no missed rollover)
    const unsigned short delta = hudmath_cons_delta(counter, this->_lastConsCounter, 0x7FFF);
    this->_lastConsCounter = counter;

    this->_tripMicroLitres  += delta; // trip accumulator (F4 fuel burnt, A2 average)
    this->_frameMicroLitres += delta; // per-frame, reset in animationFrame()
}


bool Hudisplay::requestAnimationFrame(const unsigned long now)
{
    if (this->_nextFrameTimeMs - now < this->FRAME_DURATION_MS) {
        return false;
    }

    this->_nextFrameTimeMs = now + this->FRAME_DURATION_MS;

    this->_rpmMinMax = this->_retrieveMinMax(this->_rpmList);
    this->_speedMinMax = this->_retrieveMinMax(this->_speedList);

    // The page (PAGE_DRIVING / PAGE_IDLE) is chosen by Workflow from the
    // RPM + handbrake criterion, not by the old _isStationary() heuristic.
    this->animationFrame();

    return true;
}


void Hudisplay::animationFrame()
{
    this->_update();

    // Instantaneous consumption for this frame, from the µl burnt during the frame.
    //   l/100km = (µl/1e6 L) / (km driven this frame) * 100
    //   km this frame = speed[km/h] * (FRAME_DURATION_MS / 3600000)
    // Display-near ring byte SCALE: byte = clamp(round(l_per_100 * 2), 0, 255)
    //   => 0.5 l/100km per pixel, full-scale 127.5 l/100km. Consistent with _drawTank,
    //      which reads _consumptionList entries directly as pixel offsets on the fuel bar.
    uint8_t instConsByte = 0;
    const uint8_t speedKmh = this->_getSpeedValue();
    if (speedKmh != 0 && this->_frameMicroLitres != 0) {
        // Integer/fixed-point equivalent of the old soft-float path (drops the FPU
        // dependency from the per-frame hot path). Derivation:
        //   l/100km = µl * 360 / (speed * FRAME_DURATION_MS)
        //   byte    = clamp(round(l/100km * 2), 0, 255)         (0.5 l/100 per unit)
        //           = clamp(round(720*µl / (speed*FRAME_DURATION_MS)), 0, 255)
        // Round-half-up via integer division: (num + den/2) / den. FRAME_DURATION_MS
        // is even (64), so den = speed*64 is always even -> den/2 is exact and the
        // result is byte-identical to floor(720*µl/den + 0.5). uint32 intermediates:
        // 720 * max-frame-µl stays well under 2^32.
        const uint32_t den = (uint32_t)speedKmh * (uint32_t)this->FRAME_DURATION_MS;
        const uint32_t scaled = ((uint32_t)this->_frameMicroLitres * 720UL + (den >> 1)) / den;
        instConsByte = (scaled > 255UL) ? (uint8_t)255 : (uint8_t)scaled;
    }
    this->_frameMicroLitres = 0;

    const uint8_t previousFrameIndex = this->_frameIndex;
    ++this->_frameIndex;

    if (this->_frameIndex == this->HISTORY_FRAME_SIZE) {
        this->_frameIndex = 0;
    }

    /*
    this->_rpmList[this->_frameIndex].min = this->_currentRpmValueInDiv64;
    this->_rpmList[this->_frameIndex].max = this->_currentRpmValueInDiv64;
    this->_speedList[this->_frameIndex].min = this->_currentSpeedValue;
    this->_speedList[this->_frameIndex].max = this->_currentSpeedValue;
    this->_consumptionList[this->_frameIndex].min = this->_currentFuelConsumptionValue;
    this->_consumptionList[this->_frameIndex].max = this->_currentFuelConsumptionValue;
    */
    this->_rpmList[this->_frameIndex] = this->_rpmList[previousFrameIndex];
    this->_speedList[this->_frameIndex] = this->_speedList[previousFrameIndex];
    // consumption ring is fed by the µl counter (above), not carried forward
    this->_consumptionList[this->_frameIndex] = instConsByte;
}


void Hudisplay::setBootWarp(const uint8_t level0to255)
{
    this->_bootWarp = level0to255;
}


void Hudisplay::setBootOutcome(const BootOutcome outcome)
{
    this->_bootOutcome = outcome;
}


void Hudisplay::bootFrame(void)
{
    // UNTHROTTLED on purpose: no FRAME_DURATION_MS gate, no history rotation.
    // Run the starfield as fast as the loop allows during _boot.
    this->_bootFrame();
}


// ==============================================
// PROTECTED
// ==============================================


void Hudisplay::_reset(void)
{
    // Per-frame state was indeterminate after `new` (default-init), which let the
    // ring index run out of range at startup (only normalised on ==HISTORY_FRAME_SIZE).
    this->_frameIndex      = 0;
    this->_nextFrameTimeMs = 0;
    this->_firstMillisIn4s = 0;
    this->_rpmMinMax.min   = 0;
    this->_rpmMinMax.max   = 0;
    this->_speedMinMax.min = 0;
    this->_speedMinMax.max = 0;

    memset(this->_rpmList        , 0, sizeof(this->_rpmList        ));
    memset(this->_speedList      , 0, sizeof(this->_speedList      ));
    memset(this->_consumptionList, 0, sizeof(this->_consumptionList));

    // µl fuel accumulators (re-seeded on first setConsumptionCounter sample)
    this->_consPrimed       = false;
    this->_lastConsCounter  = 0;
    this->_tripMicroLitres  = 0;
    this->_frameMicroLitres = 0;
}


uint8_t Hudisplay::_getTankCapacity(void) const
{
    return this->_currentTankCapacity;
}


uint8_t Hudisplay::_getTankLoad(void) const
{
    return this->_currentTankLoad;
}


unsigned long Hudisplay::_getOdometerValue(void) const
{
    return this->_currentOdometerValue;
}


uint8_t Hudisplay::_getGearPosition(void) const
{
    return this->_currentGearPosition;
}


uint8_t Hudisplay::_getRpmValueInDiv64(void) const
{
    return this->_rpmList[this->_frameIndex];
}


uint8_t Hudisplay::_getSpeedValue(void) const
{
    return this->_speedList[this->_frameIndex];
}


uint8_t Hudisplay::_getFuelConsumptionValue(void) const
{
    return this->_consumptionList[this->_frameIndex];
}


uint8_t Hudisplay::_retrieveMin(uint8_t* valueList) const
{
    uint8_t min = valueList[0];

    for (uint8_t i=1; i<this->HISTORY_FRAME_SIZE; ++i) {
        if (min > valueList[i]) {
            min = valueList[i];
        }
    }

    return min;
}


uint8_t Hudisplay::_retrieveMax(uint8_t* valueList) const
{
    uint8_t max = valueList[0];

    for (uint8_t i=1; i<this->HISTORY_FRAME_SIZE; ++i) {
        if (max < valueList[i]) {
            max = valueList[i];
        }
    }

    return max;
}

Hudisplay::MinMax Hudisplay::_retrieveMinMax(uint8_t* valueList) const
{
    MinMax out;
    out.min = valueList[0];
    out.max = valueList[0];

    for (uint8_t i=1; i<this->HISTORY_FRAME_SIZE; ++i) {
        if (out.max < valueList[i]) {
            out.max = valueList[i];
        } else if (out.min > valueList[i]) {
            out.min = valueList[i];
        }
    }

    return out;
}


unsigned long Hudisplay::_getDistance(void) const
{
    return this->_currentOdometerValue - this->_firstOdometerValue;
}


unsigned short Hudisplay::_millisIn4s(void) const
{
    return (unsigned short) (this->_nextFrameTimeMs >> 12);
}


unsigned short Hudisplay::_getDuration(void) const
{
    return this->_millisIn4s() - this->_firstMillisIn4s;
}


uint8_t Hudisplay::_getTankValue(void) const
{
    // litres directly (native CAN Tankinhalt)
    return this->_getTankLoad();
}


uint8_t Hudisplay::_getConsumptionLoad(void) const
{
    // litres burnt by tank-level delta (start - current); clamp to 0 on refuel/noise
    // (current > first) so the uint8 subtraction can't wrap to a huge value.
    if (this->_currentTankLoad > this->_firstTankLoad) {
        return 0;
    }
    return this->_firstTankLoad - this->_currentTankLoad;
}


uint8_t Hudisplay::_getConsumptionInL(void) const
{
    // litres directly (native units); identity passthrough
    return this->_getConsumptionLoad();
}


uint8_t Hudisplay::_fuelLoadToL(const uint8_t load) const
{
    // native CAN already stores litres -> identity (kept for ABI compatibility)
    return load;
}


float Hudisplay::_getFuelBurntInL(void) const
{
    // exact litres burnt from the µl trip accumulator
    return (float)this->_tripMicroLitres / 1000000.0f;
}


float Hudisplay::_getAverageConsumptionInLp10km(void) const
{
    // average l/100km = 100 * litresBurnt / distanceKm (distanceKm==0 guarded)
    return hudmath_avg_cons_lp100(this->_getFuelBurntInL(), this->_getDistance());
}


uint8_t Hudisplay::_getAverageSpeedInKmh(void) const
{
    // distance is now integer km; average km/h = 60 * km / durationMinutes (guarded)
    const unsigned long distanceKm = this->_getDistance();
    const unsigned short durationInMinutes = this->_getDuration() / 15;

    return hudmath_avg_speed_kmh(distanceKm, durationInMinutes);
}