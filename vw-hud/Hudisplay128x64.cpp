#include "Hudisplay128x64.h"
#include "_hudmath.h"
#include "PowerManager.h"



static const uint8_t _GAUGE_HEIGHT = 10;
static const uint8_t _SCREEN_WIDTH = 128;
static const uint8_t _SCREEN_HEIGHT = 64;



#define UD_DISPLAY128X64_AREA_BLINKING_ON  (this->_frameIndex <  (this->HISTORY_FRAME_SIZE >> 1))
#define UD_DISPLAY128X64_AREA_BLINKING_OFF (this->_frameIndex >= (this->HISTORY_FRAME_SIZE >> 1))



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

    #if VH_DISPLAY_FULLBUFFER != 0
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


void Hudisplay128x64::_setNightMode(const bool isNight)
{
    this->_board->setContrast(isNight ? SCREEN_BRIGHTNESS_NIGHT : SCREEN_BRIGHTNESS_DAY);
}


void Hudisplay128x64::_cacheFrameScalars(void)
{
    // Computed ONCE per frame; read by the per-band draws. capacity 70 L; the
    // fuel px helper guards capacity==0 and clamps overfull (see _hudmath.h).
    const uint8_t capacity = this->_getTankCapacity();
    this->_cachedFirstFuelPx   = hudmath_fuel_px(this->_firstTankLoad, capacity, _SCREEN_WIDTH);
    this->_cachedCurrentFuelPx = hudmath_fuel_px(this->_getTankLoad(), capacity, _SCREEN_WIDTH);
    this->_cachedConsMin = this->_retrieveMin(this->_consumptionList);
    this->_cachedConsMax = this->_retrieveMax(this->_consumptionList);
}


void Hudisplay128x64::_update(void)
{
    #if VH_DISPLAY_FULLBUFFER == 0
    this->_cacheFrameScalars(); // hoist per-frame scalars out of the band loop
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

            case Hudisplay::PAGE_NONE:
            case Hudisplay::PAGE_BOOT:  // boot is rendered by bootFrame(), not here
                break;
        }

        this->_drawTrip(); // low fps
        this->_drawTank(); // high fps if Fuel Consumption

    } while (this->_board->nextPage());

    #else
    this->_cacheFrameScalars(); // _drawTank/_drawMaxSpeed read the cached scalars
    this->_board->clearBuffer();

    switch (this->_page) {
        case Hudisplay::PAGE_DRIVING:
            // high fps: 1/1
            this->_drawRpm();
            
            // avg fps: 1/2
            if (this->_frameIndex & 0b1) {
                this->_drawSpeed();
            } else {
                // low fps: 1/8
                switch (this->_frameIndex & 0b110) {
                    case 0b000:
                        this->_drawMaxSpeed();
                        break;
                    case 0b010:
                        this->_drawTrip();
                        break;
                    case 0b100:
                        this->_drawGearPosition();
                        break;
                    case 0b110:
                        this->_drawTank();
                        break;
                }
            }
            break;

        case Hudisplay::PAGE_IDLE:
            // high fps: 1/1
            // -- not yet
            
            // avg fps: 1/2
            if (this->_frameIndex & 0b1) {
                this->_drawStats();
            } else {
                // low fps: 1/4
                switch (this->_frameIndex & 0b10) {
                    case 0b00:
                        this->_drawTrip();
                        break;
                    case 0b10:
                        this->_drawTank();
                        break;
                }
            }
            break;

        case Hudisplay::PAGE_NONE:
        case Hudisplay::PAGE_BOOT:  // boot is rendered by bootFrame(), not here
            break;
    }

    // this->_drawTank(); // high fps if Fuel Consumption
    #endif
}


// ==============================================
// BOOT STARFIELD (integer / fixed-point only)
// ==============================================

// Projection scale: screenX = 64 + (x * SC) / z ; with z in Q8 (256 == 1.0):
//   screenX = 64 + (x * SC * 256) / z_q8
static const int32_t _BOOT_SC = 26;
static const uint16_t _BOOT_ZNEAR = 0x000A;   // ~0.04 in Q8 -> respawn threshold
static const uint16_t _BOOT_ZONE  = 0x0100;   // 1.0 in Q8


uint16_t Hudisplay128x64::_bootRand(void)
{
    // xorshift32 -> 16-bit; cheap, deterministic, no float, no <random>
    uint32_t x = this->_bootRng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    this->_bootRng = x;
    return (uint16_t)(x >> 8);
}


void Hudisplay128x64::_bootSpawn(BootStar* s, const bool anyZ)
{
    s->x = (int8_t)(this->_bootRand() & 0xFF);   // wraps into full int8 range [-128,127]
    s->y = (int8_t)(this->_bootRand() & 0xFF);
    // avoid the dead centre (x==y==0 never moves on screen)
    if (s->x == 0) s->x = 1;
    if (s->y == 0) s->y = 1;
    // anyZ: scatter across depth on first init; else spawn far (z ~= 1.0)
    s->z = anyZ ? (uint16_t)(0x000C + (this->_bootRand() % 0x00F4)) // 0.046 .. ~1.0
                : _BOOT_ZONE;
}


void Hudisplay128x64::_bootFrame(void)
{
    if (!this->_bootStarsInit) {
        for (uint8_t i = 0; i < HUD_BOOT_STARS; ++i) {
            this->_bootSpawn(&this->_bootStars[i], true);
        }
        this->_bootStarsInit = true;
    }

    // --- target velocity (Q8 z-decrement per frame) from warp/outcome ---
    // build-up: warp 0..255 -> step ~2..18 (capped below max by Workflow's cap)
    // DRIVE: burst (fast); IDLE: settle (stars almost freeze)
    uint16_t target;
    switch (this->_bootOutcome) {
        case BOOT_OUTCOME_DRIVE: target = 48; break;            // warp burst
        case BOOT_OUTCOME_IDLE:  target = 1;  break;            // decelerate / settle
        default:                 target = 2 + ((uint16_t)this->_bootWarp >> 4); break; // 2..17
    }

    // ease toward target (faster on DRIVE) — integer low-pass, no float
    const uint8_t ease = (this->_bootOutcome == BOOT_OUTCOME_DRIVE) ? 1 : 2; // >> shift
    if (target > this->_bootVel) {
        this->_bootVel += (target - this->_bootVel + ((1 << ease) - 1)) >> ease;
    } else {
        this->_bootVel -= (this->_bootVel - target) >> ease;
    }

    // --- update positions ONCE per frame ---
    for (uint8_t i = 0; i < HUD_BOOT_STARS; ++i) {
        BootStar* s = &this->_bootStars[i];
        if (s->z <= this->_bootVel + _BOOT_ZNEAR) {
            this->_bootSpawn(s, false);
            continue;
        }
        s->z -= this->_bootVel;
    }

    // --- draw inside the page loop (U8g2 clips each 128x8 band) ---
    this->_board->firstPage();
    do {
        this->_bootDrawStarfield();
    } while (this->_board->nextPage());
}


void Hudisplay128x64::_bootDrawStarfield(void)
{
    this->_board->setDrawColor(1);

    for (uint8_t i = 0; i < HUD_BOOT_STARS; ++i) {
        const BootStar* s = &this->_bootStars[i];

        const int32_t num_x = (int32_t)s->x * _BOOT_SC * 256;
        const int32_t num_y = (int32_t)s->y * _BOOT_SC * 256;

        const int16_t sx = 64 + (int16_t)(num_x / (int32_t)s->z);
        const int16_t sy = 32 + (int16_t)(num_y / (int32_t)s->z);

        if (sx < -4 || sx > _SCREEN_WIDTH + 4 || sy < -4 || sy > _SCREEN_HEIGHT + 4) {
            continue;
        }

        // previous (deeper) position -> streak; clamp prev z to <= 1.0 for the tail
        uint16_t zp = s->z + this->_bootVel;
        if (zp > _BOOT_ZONE) zp = _BOOT_ZONE;
        const int16_t px = 64 + (int16_t)(num_x / (int32_t)zp);
        const int16_t py = 32 + (int16_t)(num_y / (int32_t)zp);

        const int16_t dx = sx - px;
        const int16_t dy = sy - py;
        if ((dx * dx + dy * dy) < 2) {
            this->_board->drawPixel(sx, sy);
        } else {
            this->_board->drawLine(px, py, sx, sy);
        }
    }

    // --- climax text on a black plate (drawColor 0 box, then drawColor 1 text) ---
    const char* msg = nullptr;
    if (this->_bootOutcome == BOOT_OUTCOME_DRIVE) {
        // blink ~ every 128 ms of wall-clock (matches the demo's (Date.now()>>7)&1)
        if ((Energy.realMillis() >> 7) & 1UL) {
            msg = "READY > DRIVE";
        }
    } else if (this->_bootOutcome == BOOT_OUTCOME_IDLE && this->_bootVel <= 2) {
        msg = "PARKED";
    }

    if (msg != nullptr) {
        this->_board->setFont(u8g2_font_profont12_tr);
        const uint8_t pad = 2;
        const uint8_t th = 9;                                  // profont12 ~ 9px cap height
        const u8g2_uint_t tw = this->_board->getStrWidth(msg);
        const int16_t bx = (_SCREEN_WIDTH - (int16_t)tw) / 2;  // text origin x (left)
        const int16_t by = 32 + (th / 2);                      // baseline (vertically centred)

        this->_board->setDrawColor(0);
        this->_board->drawBox(bx - pad, by - th - pad, tw + 2 * pad, th + 2 * pad);
        this->_board->setDrawColor(1);
        this->_board->drawStr(bx, by, msg);
    }
}


void Hudisplay128x64::_drawRpm(void)
{   
    const u8g2_uint_t y0 = 0;
    const u8g2_uint_t x1 = this->_getRpmValueInDiv64();
    const u8g2_uint_t xmin = this->_rpmMinMax.min;
    const u8g2_uint_t xmax = this->_rpmMinMax.max;

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

    /*
    if (this->_getFuelConsumptionValue() < 0.1) { // TODO CONSTANTIZE
        this->_board->drawVLine(_SCREEN_WIDTH-1, y0+1, _GAUGE_HEIGHT-2);
        this->_board->drawHLine(x1+1, y0, _SCREEN_WIDTH-2);
        this->_board->drawHLine(x1+1, _SCREEN_HEIGHT-1, _SCREEN_WIDTH-2);
    }
    */

    #if HUD_DISPLAY128X64_AREA_BLINK
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    if (HUD_DISPLAY128X64_AREA_BLINKING_ON) this->_board->drawFrame(0, 0, _SCREEN_WIDTH, _GAUGE_HEIGHT+3);
    #endif
    #if VH_DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(0, 0, _SCREEN_WIDTH, _GAUGE_HEIGHT+2);
    #endif
}


void Hudisplay128x64::_drawStats(void)
{
    // IDLE (arrêt) layout — fixed grid, matches www/hud-simulator.html drawIdle()
    // and doc/zones-idle.svg. The rule, O1, A1, A2 always render at the same
    // positions; only the V1 VIN text is conditional (HUD_VIN), no reflow.
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    this->_board->setFont(u8g2_font_profont12_tr);

    // V1 — VIN row, centred, baseline y=8 (board-gated; blank when HUD_VIN off).
    #if defined(HUD_VIN)
    if (this->_vin[0] != '\0') {
        const u8g2_uint_t vinWidth = this->_board->getStrWidth(this->_vin);
        this->_board->drawStr((_SCREEN_WIDTH - (int16_t)vinWidth) / 2, 8, this->_vin);
    }
    #endif

    // O1 — total odometer (km), centred, baseline y=18.
    {
        char odoLabel[14];
        ultoa(this->_getOdometerValue(), odoLabel, 10); // native CAN: km integer
        strcat_P(odoLabel, PSTR(" km"));
        const u8g2_uint_t odoWidth = this->_board->getStrWidth(odoLabel);
        this->_board->drawStr((_SCREEN_WIDTH - (int16_t)odoWidth) / 2, 18, odoLabel);
    }

    // Separator "vehicle | trip": thin full-width rule at y=20 (ALWAYS drawn).
    this->_board->drawHLine(0, 20, _SCREEN_WIDTH);

    // A1 — average speed (km/h), LEFT, baseline y=37.
    {
        char speedLabel[10];
        itoa(this->_getAverageSpeedInKmh(), speedLabel, 10);
        strcat_P(speedLabel, PSTR(" km/h"));
        this->_board->drawStr(0, 37, speedLabel);
    }

    // A2 — average consumption (l/100), RIGHT-aligned to 128, baseline y=37.
    {
        const float fuelInLp100km = this->_getAverageConsumptionInLp10km(); // true l/100km
        const uint8_t fuelInteger = (uint8_t)fuelInLp100km;
        const uint8_t fuelDecimal = (uint8_t)((fuelInLp100km - (float)fuelInteger) * 10.0f);

        char fuelLabel[12];
        itoa(fuelInteger, fuelLabel, 10);
        const uint8_t i = strlen(fuelLabel);
        fuelLabel[i]   = '.';
        fuelLabel[i+1] = (char)('0' + fuelDecimal);
        fuelLabel[i+2] = '\0';
        strcat_P(fuelLabel, PSTR(" l/100"));

        const u8g2_uint_t fuelWidth = this->_board->getStrWidth(fuelLabel);
        // +1px: profont12 advance is 6px but ink is 5px (1px trailing inter-char blank).
        // Anchoring at WIDTH leaves the last glyph 1px short (ink ends col 126, col 127 blank);
        // +1 pushes ink flush to col 127, the blank advance falling off-screen — no info lost.
        this->_board->drawStr(_SCREEN_WIDTH + 1 - (int16_t)fuelWidth, 37, fuelLabel);
    }

    #if HUD_DISPLAY128X64_AREA_BLINK
    if (HUD_DISPLAY128X64_AREA_BLINKING_ON) this->_board->drawFrame(0, 0, _SCREEN_WIDTH, 38);
    #endif
    #if VH_DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(0, 0, _SCREEN_WIDTH, 40);
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
    if (HUD_DISPLAY128X64_AREA_BLINKING_ON) this->_board->drawFrame(39, 42-28, _SCREEN_WIDTH- 2*39, 28);
    #endif
    #if VH_DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(39, 42-28, _SCREEN_WIDTH- 2*39, 28);
    #endif
}


void Hudisplay128x64::_drawMaxSpeed(void)
{
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    this->_board->setFont(u8g2_font_profont12_tr);

    const uint8_t fontWidth = 4;

    const uint8_t speedMax = this->_speedMinMax.max;

    // guard the uint8 underflow: only compute speedMax - delta when it can't wrap
    if (speedMax >= this->_speedMax_delta
        && this->_getSpeedValue() < (uint8_t)(speedMax - this->_speedMax_delta)) {
        char speedLabel[4];
        itoa(speedMax, speedLabel, 10);
        const uint8_t xoffset = this->_offsetLabel3(speedMax, fontWidth);

        this->_board->drawStr(111+xoffset, 22, speedLabel);
    }
    
    #if HUD_DISPLAY128X64_AREA_BLINK
    if (HUD_DISPLAY128X64_AREA_BLINKING_ON) this->_board->drawFrame(111, 22-8, _SCREEN_WIDTH-111, 8);
    #endif
    #if VH_DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(111, 22-8, _SCREEN_WIDTH-111, 8);
    #endif
}


void Hudisplay128x64::_drawGearPosition(void)
{
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    this->_board->setFont(u8g2_font_profont12_tr);

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
    if (HUD_DISPLAY128X64_AREA_BLINKING_ON) this->_board->drawFrame(105, 32-8, _SCREEN_WIDTH-105, 8);
    if (HUD_DISPLAY128X64_AREA_BLINKING_ON) this->_board->drawFrame(0, 22-8, 23, 18);
    #endif
    #if VH_DISPLAY_FULLBUFFER != 0
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
            const unsigned long distanceKm = this->_getDistance(); // native CAN: km integer

            this->_board->setCursor(0, y0);
            this->_board->print(distanceKm);
        }

        if (this->_page == Hudisplay::PAGE_IDLE) {
            this->_board->print(F(" +"));
            this->_board->print(this->_firstOdometerValue); // km integer
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
    if (HUD_DISPLAY128X64_AREA_BLINKING_ON) this->_board->drawFrame(0, y0-6, _SCREEN_WIDTH, 6);
    #endif
    #if VH_DISPLAY_FULLBUFFER != 0
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

    // fuel bar is litres-based: px = litres * (W-1) / capacity  (capacity 70 L).
    // Hoisted out of the band loop: _update() caches these once per frame.
    const u8g2_uint_t firstSeparatorLength = this->_cachedFirstFuelPx;
    this->_board->drawVLine(firstSeparatorLength, y0+1, _GAUGE_HEIGHT-2);

    const u8g2_uint_t currentSeparatorLength = this->_cachedCurrentFuelPx;
    if (currentSeparatorLength > 1) {
        this->_board->drawBox(1, y0+1, currentSeparatorLength -1, _GAUGE_HEIGHT-2);
    }

    this->_board->drawVLine(currentSeparatorLength, y0+2, _GAUGE_HEIGHT-4);

    // instant fuel consumption
    // https://github.com/oesmith/obdgpslogger/blob/master/doc/mpg-calculation
    // instant = 710.7 * VSS / MAF


    if (this->_page == Hudisplay::PAGE_DRIVING) {
        const u8g2_uint_t y0 = _SCREEN_HEIGHT - _GAUGE_HEIGHT;

        const uint8_t min = this->_cachedConsMin;
        const uint8_t max = this->_cachedConsMax;

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
            const uint8_t fuelBurnt = (uint8_t)this->_getFuelBurntInL(); // µl trip accumulator
            itoa(fuelBurnt, fuelBurntLabel, 10);
            this->_board->setDrawColor(2); // 0, 1, 2=XOR
            this->_board->drawStr(firstSeparatorLength < _SCREEN_WIDTH - (fontWidth*strlen("##") + 3) ? firstSeparatorLength +3 : firstSeparatorLength - (fontWidth*strlen("##") + 3), _SCREEN_HEIGHT-2, fuelBurntLabel);
        }
    }

    #if HUD_DISPLAY128X64_AREA_BLINK
    this->_board->setDrawColor(1); // 0, 1, 2=XOR
    if (HUD_DISPLAY128X64_AREA_BLINKING_ON) this->_board->drawFrame(0, _SCREEN_HEIGHT-_GAUGE_HEIGHT-3, _SCREEN_WIDTH, _GAUGE_HEIGHT+3);
    #endif
    #if VH_DISPLAY_FULLBUFFER != 0
    this->_board->updateDisplayArea(0, _SCREEN_HEIGHT-_GAUGE_HEIGHT-3, _SCREEN_WIDTH, _GAUGE_HEIGHT+3);
    #endif
}


uint8_t Hudisplay128x64::_offsetLabel2(const uint8_t value, const uint8_t charWidth) {
    return hudmath_label_offset(value, 2, charWidth);
}


uint8_t Hudisplay128x64::_offsetLabel3(const uint8_t value, const uint8_t charWidth) {
    return hudmath_label_offset(value, 3, charWidth);
}


uint8_t Hudisplay128x64::_offsetLabel4(const unsigned short value, const uint8_t charWidth) {
    return hudmath_label_offset(value, 4, charWidth);
}


uint8_t Hudisplay128x64::_offsetLabel6(const unsigned long value, const uint8_t charWidth) {
    // 6-char field, but (historically) the lower branches are disabled: any value
    // under 10000 is treated as 4 digits (offset 2*charWidth). Floor the value at
    // 1000 so hudmath_label_offset reproduces that exact two-step behaviour.
    const unsigned long floored = (value < 1000UL) ? 1000UL : value;
    return hudmath_label_offset(floored, 6, charWidth);
}
