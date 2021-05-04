#include "Hudisplay.h"


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
    
    this->_switchOn();
}


void Hudisplay::switchOff()
{
    this->_switchOff();
}


void Hudisplay::setTankCapacity(const uint8_t current)
{
    this->_currentTankCapacity = current;
}


void Hudisplay::setTankLoad(const uint8_t current)
{
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
    const uint8_t rpmValueInDiv64 = current >> 8;

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


const bool Hudisplay::requestAnimationFrame(const unsigned long now)
{
    if (this->_nextFrameTimeMs - now < this->FRAME_DURATION_MS) {
        return false;
    }

    this->_nextFrameTimeMs = now + this->FRAME_DURATION_MS;

    this->animationFrame();

    return true;
}


void Hudisplay::animationFrame()
{
    this->_update();

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
    this->_consumptionList[this->_frameIndex] = this->_consumptionList[previousFrameIndex];
}


// ==============================================
// PROTECTED
// ==============================================


void Hudisplay::_reset(void)
{
    memset(this->_rpmList        , 0, sizeof(this->_rpmList        ));
    memset(this->_speedList      , 0, sizeof(this->_speedList      ));
    memset(this->_consumptionList, 0, sizeof(this->_consumptionList));
}


const uint8_t Hudisplay::_getTankCapacity(void) const
{
    return this->_currentTankCapacity;
}


const uint8_t Hudisplay::_getTankLoad(void) const
{
    return this->_currentTankLoad;
}


const unsigned long Hudisplay::_getOdometerValue(void) const
{
    return this->_currentOdometerValue;
}


const uint8_t Hudisplay::_getGearPosition(void) const
{
    return this->_currentGearPosition;
}


const uint8_t Hudisplay::_getRpmValueInDiv64(void) const
{
    return this->_rpmList[this->_frameIndex];
}


const uint8_t Hudisplay::_getSpeedValue(void) const
{
    return this->_speedList[this->_frameIndex];
}


const uint8_t Hudisplay::_getFuelConsumptionValue(void) const
{
    return this->_consumptionList[this->_frameIndex];
}


void Hudisplay::_insertValueIntoCurrentMinMax(unsigned long value, MinMax* minMax)
{
    if (value < minMax->min) {
        minMax->min = value;
    } else if (value > minMax->max) {
        minMax->max = value;
    }
}


const uint8_t Hudisplay::_retrieveMin(MinMax* minMax) const
{
    uint8_t min = minMax[0].min;

    for (uint8_t i=1; i<this->HISTORY_FRAME_SIZE; ++i) {
        if (min > minMax[i].min) {
            min = minMax[i].min;
        }
    }

    return min;
}


const uint8_t Hudisplay::_retrieveMin(uint8_t* valueList) const
{
    uint8_t min = valueList[0];

    for (uint8_t i=1; i<this->HISTORY_FRAME_SIZE; ++i) {
        if (min > valueList[i]) {
            min = valueList[i];
        }
    }

    return min;
}


const uint8_t Hudisplay::_retrieveMax(MinMax* minMax) const
{
    uint8_t max = minMax[0].max;

    for (uint8_t i=1; i<this->HISTORY_FRAME_SIZE; ++i) {
        if (max < minMax[i].max) {
            max = minMax[i].max;
        }
    }

    return max;
}


const uint8_t Hudisplay::_retrieveMax(uint8_t* valueList) const
{
    uint8_t max = valueList[0];

    for (uint8_t i=1; i<this->HISTORY_FRAME_SIZE; ++i) {
        if (max < valueList[i]) {
            max = valueList[i];
        }
    }

    return max;
}


const unsigned long Hudisplay::_getDistance(void) const
{
    return this->_currentOdometerValue - this->_firstOdometerValue;
}


const unsigned short Hudisplay::_millisIn4s(void) const
{
    return int(this->_nextFrameTimeMs >> 12);
}


const unsigned short Hudisplay::_getDuration(void) const
{
    return this->_millisIn4s() - this->_firstMillisIn4s;
}


const uint8_t Hudisplay::_getTankValue(void) const
{
    return this->_fuelLoadToL(this->_getTankLoad());
}


const uint8_t Hudisplay::_getConsumptionLoad(void) const
{
    return this->_firstTankLoad - this->_currentTankLoad;
}


const uint8_t Hudisplay::_getConsumptionInL(void) const
{
    return this->_fuelLoadToL(this->_getConsumptionLoad());
}


const uint8_t Hudisplay::_fuelLoadToL(const uint8_t load) const
{
    return (load * this->_getTankCapacity()) >> 8;
}


const float Hudisplay::_getAverageConsumptionInLp10km(void) const
{
    const unsigned short lengthInHm = this->_getDistance();
    const uint8_t fuelBurnt = this->_getConsumptionInL();
    // consumption = 100* (fuelBurnt) / (lengthInHm / 10)

    return 10000 * fuelBurnt/lengthInHm;
}


const uint8_t Hudisplay::_getAverageSpeedInKmph(void) const
{
    const unsigned short lengthInHm = this->_getDistance();
    const unsigned short durationInMinutes = this->_getDuration() / 15;
    // speed = (lengthInHm / 10) / (durationInMinutes / 60)
    
    return 6 * lengthInHm / durationInMinutes;
}
