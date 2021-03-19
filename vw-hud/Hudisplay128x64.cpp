#include "Hudisplay128x64.h"



static const uint8_t _GAUGE_HEIGHT = 10;
static const uint8_t _SCREEN_WIDTH = 128;
static const uint8_t _SCREEN_HEIGHT = 64;



// ==============================================
// PUBLIC
// ==============================================


void Hudisplay128x64::setBoard(U8G2* board)
{
    this->_board = board;

    //this->_board->setBusClock(1000000000UL);
    //this->_board->setBusClock(1000000);
    //this->_board->setBusClock(8000000);
    this->_board->begin();
    this->_board->setFontMode(1);
    this->_board->setFontPosBaseline();

    #if DISPLAY_FULLBUFFER != 0
    this->_board->clearBuffer();
    this->_board->sendBuffer();
    #endif
}


// ==============================================
// PROTECTED
// ==============================================


void Hudisplay128x64::_switchOn(void)
{
    this->_board->setPowerSave(0);
}


void Hudisplay128x64::_switchOff(void)
{
    this->_board->setPowerSave(1);
}


void Hudisplay128x64::_update(void)
{
    #if DISPLAY_FULLBUFFER == 0
    this->_board->firstPage();
    do {

    switch (this->_page) {
        case Hudisplay::PAGE_DRIVING:
            this->_drawRpm();               // high fps
            this->_drawSpeed();             // avg fps
            this->_drawMaxSpeed();          // low fps
            this->_drawGearPosition();      // low fps
            break;

        case Hudisplay::PAGE_IDLE:
            this->_drawStats();     // avg fps
            break;
    }

    this->_drawTrip(); // low fps
    this->_drawTank(); // high fps if Fuel Consumption

    } while (this->_board->nextPage());

    #else
    this->_board->clearBuffer();

    switch (this->_page) {
        case Hudisplay::PAGE_DRIVING:
            this->_drawRpm();    // high fps
            if (this->_frameIndex & 0b1 == 0) {
                this->_drawSpeed();  // avg fps
            }
            if (this->_frameIndex == 0) {
                this->_drawMaxSpeed();   // low fps
            }
            if (this->_frameIndex == (this->HISTORY_FRAME_SIZE >> 1)) {
                this->_drawTrip(); // low fps
            }
            if (this->_frameIndex == (this->HISTORY_FRAME_SIZE >> 2)) {
                this->_drawGearPosition(); // low fps
            }
            break;

        case Hudisplay::PAGE_IDLE:
            this->_drawStats(); // avg fps
            this->_drawTrip();  // low fps
            break;
    }

    this->_drawTank(); // high fps if Fuel Consumption
    #endif
}


void Hudisplay128x64::_drawRpm(void)
{   
    const u8g2_uint_t y0 = 0;
    const u8g2_uint_t x1 = this->_getRpmValueInDiv64();
    const u8g2_uint_t xmin = this->_retrieveMin(this->_rpmList);
    const u8g2_uint_t xmax = this->_retrieveMax(this->_rpmList);

    this->_board->setDrawColor(1); // 0, 1, 2=XOR

    this->_board->drawVLine(0, y0+1, _GAUGE_HEIGHT-2);
    this->_board->drawVLine(1, y0+2, _GAUGE_HEIGHT-4);
    if (x1>2) {
        this->_board->drawPixel(2, y0+2);
        this->_board->drawPixel(2, y0+_GAUGE_HEIGHT-3);
    }
    this->_board->drawHLine(1, y0, x1);
    this->_board->drawHLine(1, y0+1, x1);
    this->_board->drawHLine(1, _GAUGE_HEIGHT-2, x1);
    this->_board->drawHLine(1, _GAUGE_HEIGHT-1, x1);

    this->_board->drawVLine(x1-1, y0, _GAUGE_HEIGHT+2);
    this->_board->drawVLine(x1, _GAUGE_HEIGHT+1, 2);
    this->_board->drawVLine(x1+1, y0, _GAUGE_HEIGHT+2);

    if (xmax-xmin > 3) {
        this->_board->drawBox(xmin+1, y0+3, xmax-xmin-1, 4);
    }
    this->_board->drawVLine(xmin, y0+4, 2);
    this->_board->drawVLine(xmax, y0+4, 2);

    this->_board->setDrawColor(0); // 0, 1, 2=XOR
    this->_board->drawVLine(x1, 0, _GAUGE_HEIGHT+1);

    #if HUD_DISPLAY128X64_AREA_BLINK
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    if (this->_frameIndex%2) this->_board->drawFrame(0, 0, _SCREEN_WIDTH, _GAUGE_HEIGHT+3);
    #endif
    #if DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(0, 0, _SCREEN_WIDTH, _GAUGE_HEIGHT+2);
    #endif
}


void Hudisplay128x64::_drawStats(void)
{
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    this->_board->setFont(u8g2_font_profont12_tr);

    const uint8_t fontWidth = 6;

    {
        const unsigned long kilometers = (this->_getOdometerValue() / 10);
        const uint8_t hectometers = (this->_getOdometerValue() - (10 * kilometers));
        const uint8_t odometerLabelWidth = (fontWidth * strlen("######.# km")) - this->_offsetLabel6(kilometers, fontWidth);
        
        this->_board->setCursor((_SCREEN_WIDTH - odometerLabelWidth)>>1, 10);
        this->_board->print(kilometers);
        this->_board->print('.');
        this->_board->print(hectometers);
        this->_board->print(F(" km"));
    }

    const unsigned short lengthInHm = this->_getDistance();

    if (lengthInHm == 0) {
        return;
    }

    {
        const uint8_t speedValue = this->_getAverageSpeedInKmph();
        const uint8_t speedLabelWidth = (fontWidth * strlen("### km/h")) - this->_offsetLabel3(speedValue, fontWidth);

        this->_board->setCursor((_SCREEN_WIDTH - speedLabelWidth)>>1, 22);
        this->_board->print(speedValue);
        this->_board->print(F(" km/h"));
    }

    {
        const float fuelInDlp100km = this->_getAverageConsumptionInDlp100km();
        const uint8_t fuelInLp100km = (fuelInDlp100km / 10);
        const uint8_t fuelInLp100kmFloat = (fuelInDlp100km - (10 * fuelInLp100km));
        
        char fuelLabel[3];
        itoa(fuelInLp100km, fuelLabel, 10);
        const uint8_t fuelLabelWidth = (fontWidth * strlen("##.# l/100")) - this->_offsetLabel3(fuelInLp100km, fontWidth);

        this->_board->setCursor((_SCREEN_WIDTH - fuelLabelWidth)>>1, 34);
        this->_board->print(fuelLabel);
        this->_board->print('.');
        this->_board->print(fuelInLp100kmFloat);
        this->_board->print(F(" l/100"));
    }
    
    #if HUD_DISPLAY128X64_AREA_BLINK
    if (this->_frameIndex%2) this->_board->drawFrame(31, 2, _SCREEN_WIDTH-63, 34);
    #endif
    #if DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(34, 22-8, _SCREEN_WIDTH-64, 22);
    #endif
}


void Hudisplay128x64::_drawSpeed(void)
{
    char speedLabel[4];
    itoa(this->_getSpeedValue(), speedLabel, 10);
    const uint8_t xoffset = this->_offsetLabel3(this->_getSpeedValue(), 18);

    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    /*
    this->_board->setFont(u8g2_font_logisoso24_tn); // 28x35(24)
    this->_board->drawStr(40, 38, speedLabel)*/
    this->_board->setFont(u8g2_font_logisoso28_tn);
    this->_board->drawStr(37+xoffset, 42, speedLabel);
    
    #if HUD_DISPLAY128X64_AREA_BLINK
    if (this->_frameIndex%2) this->_board->drawFrame(39, 42-28, _SCREEN_WIDTH- 2*39, 28);
    #endif
    #if DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(39, 42-28, _SCREEN_WIDTH- 2*39, 28);
    #endif
}


void Hudisplay128x64::_drawMaxSpeed(void)
{
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    this->_board->setFont(u8g2_font_profont12_tr);

    const uint8_t fontWidth = 4;

    const uint8_t speedMax = this->_retrieveMax(this->_speedList);

    if (this->_getSpeedValue() < speedMax - this->_speedMax_delta) {
        char speedLabel[4];
        itoa(speedMax, speedLabel, 10);
        const uint8_t xoffset = this->_offsetLabel3(speedMax, fontWidth);

        this->_board->drawStr(111+xoffset, 22, speedLabel);
    }
    
    #if HUD_DISPLAY128X64_AREA_BLINK
    if (this->_frameIndex%2) this->_board->drawFrame(111, 22-8, _SCREEN_WIDTH-111, 8);
    #endif
    #if DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(111, 22-8, _SCREEN_WIDTH-111, 8);
    #endif
}


void Hudisplay128x64::_drawGearPosition(void)
{
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    this->_board->setFont(u8g2_font_profont12_tr);

    const uint8_t fontWidth = 4;

    const uint8_t gearPosition = this->_getGearPosition();

    // TODO
    switch (gearPosition) {
        case 1:
            this->_board->drawStr(123, 32, "1");
            break;
        case 2:
            this->_board->drawStr(105, 32, "2");
            break;
        case 3:
            this->_board->drawStr(18, 22, "3");
            break;
        case 4:
            this->_board->drawStr(0, 22, "4");
            break;
        case 5:
            this->_board->drawStr(0, 32, "5");
            break;
        case 8: // TODO NEUTRE?
            this->_board->drawStr(18, 32, "N");
            break;
        case 9: // TODO REVERSE?
            this->_board->drawStr(18, 32, "R");
            break;
    }
    
    #if HUD_DISPLAY128X64_AREA_BLINK
    if (this->_frameIndex%2) this->_board->drawFrame(105, 32-8, _SCREEN_WIDTH-105, 8);
    if (this->_frameIndex%2) this->_board->drawFrame(0, 22-8, 23, 18);
    #endif
    #if DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(105, 32-8, _SCREEN_WIDTH-105, 8);
    this->_board->updateDisplayArea(0, 22-8, 23, 18);
    #endif
}


void Hudisplay128x64::_drawTrip(void)
{
    const u8g2_uint_t y0 = 51; // 51 if Fuel Consumption

    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    this->_board->setFont(u8g2_font_profont10_tn);

    const uint8_t fontWidth = 4;

    {
        {
            const unsigned short lengthInHm = this->_getDistance();
            const unsigned short kilometers = (lengthInHm / 10);
            const uint8_t hectometers = (lengthInHm - (10 * kilometers));

            this->_board->setCursor(0, y0);
            this->_board->print(kilometers);
            this->_board->print('.');
            this->_board->print(hectometers);
        }

        if (this->_page == Hudisplay::PAGE_IDLE) {
            this->_board->print(F(" +"));
            const unsigned long kilometers = (this->_firstOdometerValue / 10);
            const uint8_t hectometers = (this->_firstOdometerValue - (10 * kilometers));
            this->_board->print(kilometers);
            this->_board->print('.');
            this->_board->print(hectometers);
        }
    }

    {
        const unsigned short durationInMinutes = this->_getDuration() / 15;
        const uint8_t hours = (durationInMinutes / 60);
        const uint8_t minutes = (durationInMinutes - (60 * hours));

        const uint8_t xoffset = this->_offsetLabel2(hours, fontWidth);

        this->_board->setCursor(103+xoffset, y0);
        this->_board->print(hours);
        this->_board->print(':');
        this->_board->print(u8x8_u8toa(minutes, 2));
    }
    #if HUD_DISPLAY128X64_AREA_BLINK
    if (this->_frameIndex%2) this->_board->drawFrame(0, y0-6, _SCREEN_WIDTH, 6);
    #endif
    #if DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(0, y0-6, _SCREEN_WIDTH, 6); // TODO (0, 50) if fuel consumption
    #endif
}


void Hudisplay128x64::_drawTank(void)
{
    const u8g2_uint_t y0 = _SCREEN_HEIGHT - _GAUGE_HEIGHT;

    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    this->_board->drawVLine(0, y0+1, _GAUGE_HEIGHT-2);
    this->_board->drawVLine(_SCREEN_WIDTH-1, y0+1, _GAUGE_HEIGHT-2);
    this->_board->drawHLine(1, y0, _SCREEN_WIDTH-2);
    this->_board->drawHLine(1, _SCREEN_HEIGHT-1, _SCREEN_WIDTH-2);

    const u8g2_uint_t firstSeparatorLength = (this->_firstTankLoad * (_SCREEN_WIDTH-1)) >> 8;
    this->_board->drawVLine(firstSeparatorLength, y0+1, _GAUGE_HEIGHT-2);

    const u8g2_uint_t currentSeparatorLength = (this->_getTankLoad() * (_SCREEN_WIDTH-1)) >> 8;
    if (currentSeparatorLength > 1) {
        this->_board->drawBox(1, y0+1, currentSeparatorLength -1, _GAUGE_HEIGHT-2);
    }

    this->_board->drawVLine(currentSeparatorLength, y0+2, _GAUGE_HEIGHT-4);

    // instant fuel consumption
    // https://github.com/oesmith/obdgpslogger/blob/master/doc/mpg-calculation
    // instant = 710.7 * VSS / MAF


    if (this->_page == Hudisplay::PAGE_DRIVING) {
        const u8g2_uint_t y0 = _SCREEN_HEIGHT - _GAUGE_HEIGHT;

        const uint8_t min = this->_retrieveMin(this->_consumptionList);
        const uint8_t max = this->_retrieveMax(this->_consumptionList);

        // min is visible? 
        if (currentSeparatorLength > min) {
            const u8g2_uint_t xmin = currentSeparatorLength - min;

            // right border
            this->_board->setDrawColor(0); // 0, 1, 2=XOR
            this->_board->drawVLine(xmin, y0+3, _GAUGE_HEIGHT-6);

            if (min < max) {
                // right round border
                this->_board->drawPixel(xmin-1, y0+3);
                this->_board->drawPixel(xmin-1, _SCREEN_HEIGHT-4);
            }

            // max is visible?
            if (currentSeparatorLength > max) {
                const u8g2_uint_t xmax = currentSeparatorLength - max;
                
                // left border
                this->_board->drawVLine(xmax, y0+3, _GAUGE_HEIGHT-6);

                if (min +1 < max) {
                    // left round border
                    this->_board->drawPixel(xmax+1, y0+3);
                    this->_board->drawPixel(xmax+1, _SCREEN_HEIGHT-4);

                    // H bar
                    this->_board->drawHLine(xmax+1, y0+2, xmin-xmax-1);
                    this->_board->drawHLine(xmax+1, _SCREEN_HEIGHT-3, xmin-xmax-1);
                }

            } else {
                // H bar
                this->_board->drawHLine(0, y0+2, xmin);
                this->_board->drawHLine(0, _SCREEN_HEIGHT-3, xmin);
            }
           
            if (currentSeparatorLength > this->_getFuelConsumptionValue()) {
                const u8g2_uint_t x1 = currentSeparatorLength - this->_getFuelConsumptionValue();

                this->_board->drawVLine(x1, y0+1, _GAUGE_HEIGHT-2);
            }
        } else {
            // HURRY!
        }
    
    } else {
        this->_board->setFont(u8g2_font_profont10_tn);

        const uint8_t fontWidth = 5;

        {
            char fuelLeftLabel[4];
            const uint8_t fuelLeft = this->_getTankValue();
            itoa(fuelLeft, fuelLeftLabel, 10);
            this->_board->setDrawColor(0); // 0, 1, 2=XOR
            this->_board->drawStr(2, _SCREEN_HEIGHT-2, fuelLeftLabel);
        }

        {
            char fuelBurntLabel[4];
            const uint8_t fuelBurnt = this->_getConsumptionInL();
            itoa(fuelBurnt, fuelBurntLabel, 10);
            this->_board->setDrawColor(2); // 0, 1, 2=XOR
            this->_board->drawStr(firstSeparatorLength < _SCREEN_WIDTH - (fontWidth*strlen("##") + 3) ? firstSeparatorLength +3 : firstSeparatorLength - (fontWidth*strlen("##") + 3), _SCREEN_HEIGHT-2, fuelBurntLabel);
        }
    }

    #if HUD_DISPLAY128X64_AREA_BLINK
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    if (this->_frameIndex%2) this->_board->drawFrame(0, _SCREEN_HEIGHT-_GAUGE_HEIGHT-3, _SCREEN_WIDTH, _GAUGE_HEIGHT+3);
    #endif
    #if DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(0, _SCREEN_HEIGHT-_GAUGE_HEIGHT-3, _SCREEN_WIDTH, _GAUGE_HEIGHT+3);
    #endif
}


const uint8_t Hudisplay128x64::_offsetLabel2(const uint8_t value, const uint8_t charWidth) {
    if (value < 10) {
        return 1* charWidth;
    }

    return 0;
}


const uint8_t Hudisplay128x64::_offsetLabel3(const uint8_t value, const uint8_t charWidth) {
    if (value < 10) {
        return 2* charWidth;
    }

    if (value < 100) {
        return 1* charWidth;
    }

    return 0;
}


const uint8_t Hudisplay128x64::_offsetLabel4(const unsigned short value, const uint8_t charWidth) {
    if (value < 10) {
        return 3* charWidth;
    }

    if (value < 100) {
        return 2* charWidth;
    }

    if (value < 1000) {
        return 1* charWidth;
    }

    return 0;
}


const uint8_t Hudisplay128x64::_offsetLabel6(const unsigned long value, const uint8_t charWidth) {
    /*
    if (value < 10) {
        return 5* charWidth;
    }

    if (value < 100) {
        return 4* charWidth;
    }
    if (value < 1000) {
        return 3* charWidth;
    }
    */
    if (value < 10000) {
        return 2* charWidth;
    }

    if (value < 100000) {
        return 1* charWidth;
    }

    return 0;
}
