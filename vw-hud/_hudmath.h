#ifndef _HUDMATH_H_
#define _HUDMATH_H_

// =============================================================================
// _hudmath.h - pure display-limit math for the VW-PQ HUD
// =============================================================================
//
// Dependency-free numeric helpers extracted from Hudisplay.cpp /
// Hudisplay128x64.cpp. NO Arduino.h / U8g2 required: only <stdint.h>, so this
// header compiles both in the firmware and natively under the test/ shim.
//
// All functions are `static inline`, branch-light, and integer where the
// originals are integer. They reproduce the exact arithmetic that used to be
// inlined in the drawing/stats code so on-screen behaviour is unchanged.
//
// GENERATED REGION: the bodies between the `// HUDMATH-BEGIN <name>` and
// `// HUDMATH-END` markers are the canonical source of truth. tools/gen_hudmath.py
// re-assembles this file from those marked blocks (idempotent). Edit the marked
// blocks here, then run `python3 tools/gen_hudmath.py` to normalise the file.
// See tools/README.md (no AI required).
// =============================================================================

#include <stdint.h>


// HUDMATH-BEGIN fuel_px
// Fuel-gauge fill pixel: litres -> pixel width along a bar `barWidth` px wide.
// Mirrors `(litres * (W-1)) / capacity` from Hudisplay128x64::_drawTank, but:
//   - guards capacity == 0 (treated as 1, matching the firmware's safeCapacity),
//   - clamps overfull (litres > capacity) to barWidth-1 so it never overflows
//     the bar / wraps the u8g2 coordinate.
// Note: callers pass barWidth = SCREEN_WIDTH (the firmware multiplies by W-1
// internally), so the usable span is [0 .. barWidth-1].
static inline uint16_t hudmath_fuel_px(uint16_t litres, uint16_t capacity, uint16_t barWidth)
{
    const uint16_t span = (barWidth == 0) ? 0 : (uint16_t)(barWidth - 1);
    const uint16_t safeCapacity = (capacity == 0) ? 1 : capacity;
    if (litres >= safeCapacity) {
        return span; // overfull -> clamp to the far edge, never past it
    }
    const uint16_t px = (uint16_t)(((uint32_t)litres * span) / safeCapacity);
    return (px > span) ? span : px;
}
// HUDMATH-END

// HUDMATH-BEGIN cons_scale_byte
// Consumption-ring byte scale: instantaneous l/100km -> ring byte.
//   byte = clamp(round(l_per_100 * 2), 0, 255)
// i.e. 0.5 l/100km per unit, full-scale 127.5 l/100km. Mirrors the
// `lPer100 * 2.0f + 0.5f` then >255 -> 255 clamp in Hudisplay::animationFrame.
static inline uint8_t hudmath_cons_scale_byte(float lPer100)
{
    if (lPer100 <= 0.0f) {
        return 0;
    }
    const float scaled = lPer100 * 2.0f + 0.5f; // *2 scale, +0.5 to round
    return (scaled > 255.0f) ? (uint8_t)255 : (uint8_t)scaled;
}
// HUDMATH-END

// HUDMATH-BEGIN rpm_px
// RPM needle pixel: raw CAN rpm value -> bar pixel.
//   px = rawRpm >> shift, clamped to barWidth (max usable pixel).
// Mirrors `current >> RPM_SHIFT` from Hudisplay::setRpmValue (shift==8) feeding
// the x1 needle in _drawRpm; the clamp keeps the needle inside the bar.
static inline uint8_t hudmath_rpm_px(uint16_t rawRpm, uint8_t shift, uint8_t barWidth)
{
    const uint16_t px = (uint16_t)(rawRpm >> shift);
    return (px > barWidth) ? barWidth : (uint8_t)px;
}
// HUDMATH-END

// HUDMATH-BEGIN label_offset
// Right-aligning label x-offset, generalising _offsetLabel2/3/4/6.
// `value` is rendered as decimal; `fieldDigits` is the widest field (e.g. 3 for
// "###"), `charWidth` the px per glyph. Offset = (fieldDigits - digits(value)) *
// charWidth, never negative (values wider than the field get offset 0).
static inline uint8_t hudmath_label_offset(uint32_t value, uint8_t fieldDigits, uint8_t charWidth)
{
    uint8_t digits = 1;
    uint32_t v = value;
    while (v >= 10) { v /= 10; ++digits; }
    if (digits >= fieldDigits) {
        return 0;
    }
    return (uint8_t)((uint16_t)(fieldDigits - digits) * charWidth);
}
// HUDMATH-END

// HUDMATH-BEGIN avg_speed_kmh
// Average speed: integer km over a duration in minutes -> km/h.
//   km/h = 60 * distanceKm / durationMinutes, guarding durationMinutes == 0.
// Mirrors Hudisplay::_getAverageSpeedInKmh (result truncated to uint8_t there).
static inline uint8_t hudmath_avg_speed_kmh(uint32_t distanceKm, uint16_t durationMinutes)
{
    if (durationMinutes == 0) {
        return 0;
    }
    return (uint8_t)(60UL * distanceKm / durationMinutes);
}
// HUDMATH-END

// HUDMATH-BEGIN avg_cons_lp100
// Average consumption: litres burnt over integer km -> l/100km.
//   l/100 = 100 * litresBurnt / distanceKm, guarding distanceKm == 0.
// Mirrors Hudisplay::_getAverageConsumptionInLp10km.
static inline float hudmath_avg_cons_lp100(float litresBurnt, uint32_t distanceKm)
{
    if (distanceKm == 0) {
        return 0.0f;
    }
    return 100.0f * litresBurnt / (float)distanceKm;
}
// HUDMATH-END

// HUDMATH-BEGIN cons_delta
// µl consumption counter delta across an N-bit rolling counter.
//   delta = (cur - last) & mask   (mask = 0x7FFF for the 15-bit MO5_Verbrauch)
// Mirrors the wrap-around difference in Hudisplay::setConsumptionCounter.
static inline uint16_t hudmath_cons_delta(uint16_t cur, uint16_t last, uint16_t mask)
{
    return (uint16_t)((cur - last) & mask);
}
// HUDMATH-END

#endif // _HUDMATH_H_
