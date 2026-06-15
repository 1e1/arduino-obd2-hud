// Native unit tests for the pure display-limit math (_hudmath.h). Build:
//   c++ -std=c++11 -Wall -Itest test/test_hud.cpp -o test_hud && ./test_hud
//
// Focus: LIMITS / boundaries of each pure helper (overfull, /0 guards,
// saturation, rollover wrap, max-field values) - the cases that used to be
// inlined in the U8g2-coupled Hudisplay code.
#include "../vw-hud/_hudmath.h"
#include <cstdio>

static int failures = 0;

#define CHECK(name, got, want) do {                                  \
    const long long _g = (long long)(got);                          \
    const long long _w = (long long)(want);                         \
    if (_g != _w) { printf("FAIL %-30s got %lld want %lld\n", name, _g, _w); ++failures; } \
    else          { printf("ok   %-30s\n", name); }                 \
} while (0)

#define CHECK_F(name, got, want) do {                                \
    const double _g = (double)(got);                                 \
    const double _w = (double)(want);                                \
    const double _d = _g - _w;                                       \
    if ((_d < -1e-4) || (_d > 1e-4)) { printf("FAIL %-30s got %f want %f\n", name, _g, _w); ++failures; } \
    else          { printf("ok   %-30s\n", name); }                 \
} while (0)

int main()
{
    // ---- fuel gauge pixel: (litres, capacity, barWidth) ----------------------
    // normal mid-scale matches the original (litres*(W-1))/capacity
    CHECK("fuel mid 35/70 W128",   hudmath_fuel_px(35, 70, 128), (35 * 127) / 70);
    CHECK("fuel empty",            hudmath_fuel_px(0, 70, 128),  0);
    CHECK("fuel full == span",     hudmath_fuel_px(70, 70, 128), 127);
    // overfull: value > capacity must clamp to span (W-1), never overflow
    CHECK("fuel overfull clamp",   hudmath_fuel_px(200, 70, 128), 127);
    CHECK("fuel overfull max u8",  hudmath_fuel_px(255, 1, 128),  127);
    // capacity == 0 guard (no div/0; treated as 1 -> any nonzero load is overfull)
    CHECK("fuel cap0 load5",       hudmath_fuel_px(5, 0, 128),   127);
    CHECK("fuel cap0 load0",       hudmath_fuel_px(0, 0, 128),   0);
    // barWidth == 0 edge -> span 0
    CHECK("fuel barWidth0",        hudmath_fuel_px(5, 70, 0),    0);
    // narrow bar still bounded
    CHECK("fuel narrow clamp",     hudmath_fuel_px(60, 50, 10),  9);

    // ---- consumption ring byte scale: clamp(round(l/100 * 2), 0, 255) --------
    CHECK("cons scale 0",          hudmath_cons_scale_byte(0.0f),   0);
    CHECK("cons scale neg->0",     hudmath_cons_scale_byte(-3.0f),  0);
    CHECK("cons scale 5.0->10",    hudmath_cons_scale_byte(5.0f),   10);
    CHECK("cons scale round 5.24", hudmath_cons_scale_byte(5.24f),  10); // 10.48 -> 10
    CHECK("cons scale round 5.30", hudmath_cons_scale_byte(5.30f),  11); // 10.60 -> 11
    CHECK("cons scale fullscale",  hudmath_cons_scale_byte(127.5f), 255);
    // saturation at 255: anything above full-scale clamps, never wraps
    CHECK("cons scale sat 200",    hudmath_cons_scale_byte(200.0f), 255);
    CHECK("cons scale sat 1e6",    hudmath_cons_scale_byte(1000000.0f), 255);

    // ---- RPM needle px: rawRpm >> shift, clamped to barWidth -----------------
    CHECK("rpm 8000>>8",           hudmath_rpm_px(8000, 8, 0xFF), 8000 >> 8); // 31
    CHECK("rpm zero",              hudmath_rpm_px(0, 8, 0xFF),    0);
    // max 16-bit raw >> 8 = 255 fits exactly (no clamp triggered at 0xFF)
    CHECK("rpm max raw",           hudmath_rpm_px(0xFFFF, 8, 0xFF), 255);
    // explicit clamp when bar narrower than the shifted value
    CHECK("rpm clamp narrow bar",  hudmath_rpm_px(0xFFFF, 8, 100), 100);
    CHECK("rpm no clamp under bar",hudmath_rpm_px(0x0A00, 8, 100), 0x0A);  // 2560>>8=10

    // ---- label x-offset: (value, fieldDigits, charWidth) ---------------------
    // 2-digit field (was _offsetLabel2)
    CHECK("label2 v5 cw4",         hudmath_label_offset(5, 2, 4),   4);
    CHECK("label2 v42 cw4",        hudmath_label_offset(42, 2, 4),  0);
    // 3-digit field (was _offsetLabel3)
    CHECK("label3 v5 cw6",         hudmath_label_offset(5, 3, 6),   12);
    CHECK("label3 v42 cw6",        hudmath_label_offset(42, 3, 6),  6);
    CHECK("label3 v123 cw6",       hudmath_label_offset(123, 3, 6), 0);
    // 4-digit field (was _offsetLabel4)
    CHECK("label4 v5 cw4",         hudmath_label_offset(5, 4, 4),    12);
    CHECK("label4 v9999 cw4",      hudmath_label_offset(9999, 4, 4), 0);
    // values wider than the field never go negative
    CHECK("label over-wide",       hudmath_label_offset(123456, 3, 6), 0);
    // boundary digit-count transitions
    CHECK("label3 v9 cw1",         hudmath_label_offset(9, 3, 1),   2);
    CHECK("label3 v10 cw1",        hudmath_label_offset(10, 3, 1),  1);
    CHECK("label3 v99 cw1",        hudmath_label_offset(99, 3, 1),  1);
    CHECK("label3 v100 cw1",       hudmath_label_offset(100, 3, 1), 0);

    // ---- average speed km/h: 60 * km / minutes, durationMinutes==0 guard -----
    CHECK("avgspd 90km 60min",     hudmath_avg_speed_kmh(90, 60),  90);
    CHECK("avgspd dur0 guard",     hudmath_avg_speed_kmh(100, 0),  0);  // no div/0
    CHECK("avgspd 0km",            hudmath_avg_speed_kmh(0, 30),   0);
    CHECK("avgspd trunc",          hudmath_avg_speed_kmh(10, 7),   (60UL*10)/7); // 85

    // ---- average consumption l/100: 100 * L / km, distanceKm==0 guard --------
    CHECK_F("avgcons 7L 100km",    hudmath_avg_cons_lp100(7.0f, 100),  7.0);
    CHECK_F("avgcons dist0 guard", hudmath_avg_cons_lp100(7.0f, 0),    0.0); // no div/0
    CHECK_F("avgcons 0L",          hudmath_avg_cons_lp100(0.0f, 50),   0.0);

    // ---- µl counter delta across the 15-bit rollover -------------------------
    CHECK("delta simple",          hudmath_cons_delta(100, 40, 0x7FFF), 60);
    CHECK("delta zero",            hudmath_cons_delta(40, 40, 0x7FFF),  0);
    // rollover wrap: cur=2, last=0x7FFE -> delta 4 (2 - 32766 mod 2^15)
    CHECK("delta rollover 2/7FFE", hudmath_cons_delta(2, 0x7FFE, 0x7FFF), 4);
    // wrap exactly at the top of the 15-bit range
    CHECK("delta wrap 0/7FFF",     hudmath_cons_delta(0, 0x7FFF, 0x7FFF), 1);
    CHECK("delta wrap 7FFF/0",     hudmath_cons_delta(0x7FFF, 0, 0x7FFF), 0x7FFF);
    // a stray 16th bit on `cur` is masked off (counter is only 15-bit)
    CHECK("delta masks bit15",     hudmath_cons_delta(0x8002, 0, 0x7FFF), 2);

    printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
