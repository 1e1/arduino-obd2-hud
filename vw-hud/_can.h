#ifndef can_H_
#define can_H_

#include <Arduino.h>

/**
 * Native VW PQ CAN signal map (from opendbc/vw_pq.dbc).
 * Vehicle: VW Sharan TSI 1.5 (platform PQ). Standard 11-bit arbitration IDs.
 * Signals are little-endian / Intel (@1+ in the DBC).
 *
 * !!! ALL IDs, bit offsets and scales below are to be CONFIRMED ON BENCH
 *     (discodb2) against this exact car before trusting them. !!!
 */

// --- message arbitration IDs ---
#define VH_CAN_ID_MOTOR_1      0x280UL  // RPM
#define VH_CAN_ID_KOMBI_1      0x320UL  // speed + fuel level + handbrake lamp
#define VH_CAN_ID_MOTOR_5      0x480UL  // fuel consumption counter (µl)
#define VH_CAN_ID_KOMBI_3      0x520UL  // odometer (km)
#define VH_CAN_ID_GETRIEBE_2   0x540UL  // engaged gear (cluster display)
#define VH_CAN_ID_ZAS_1        0x572UL  // ignition (Klemme 15)

// --- signal positions {start bit, length}, little-endian ---
// Motor_1
#define VH_SIG_RPM_START         16
#define VH_SIG_RPM_LEN           16   // raw = rpm*4 (DBC factor 0.25 U/min) -> same scaling as OBD
// Kombi_1
#define VH_SIG_SPEED_START       25
#define VH_SIG_SPEED_LEN         15   // raw * 0.01 km/h
#define VH_SIG_TANK_START        16
#define VH_SIG_TANK_LEN           7   // litres (0-126), direct
#define VH_SIG_HANDBRAKE_START    1
#define VH_SIG_HANDBRAKE_LEN      1   // Handbremserinnerung lamp (1 = on)
// Motor_5
#define VH_SIG_CONS_START        16
#define VH_SIG_CONS_LEN          15   // MO5_Verbrauch: cumulative µl counter (wraps at 0x7FFF)
// Kombi_3
#define VH_SIG_ODO_START         40
#define VH_SIG_ODO_LEN           20   // Kilometerstand: km (integer)
// Getriebe_2
#define VH_SIG_GEAR_START        56
#define VH_SIG_GEAR_LEN           4   // Ganganzeige_Kombi
// ZAS_1
#define VH_SIG_IGNITION_START     1
#define VH_SIG_IGNITION_LEN       1   // Klemme_15 (1 = ignition on)


/**
 * Little-endian (Intel) bit extraction from a CAN data buffer.
 * Reads `len` bits starting at bit `start` (bit 0 = LSB of byte 0), LSB-first.
 */
static inline unsigned long canExtractLE(const unsigned char* d, const uint8_t start, const uint8_t len)
{
    unsigned long value = 0;

    for (uint8_t i = 0; i < len; ++i) {
        const uint8_t bit = start + i;
        if (d[bit >> 3] & (uint8_t)(1u << (bit & 7))) {
            value |= (unsigned long)1 << i;
        }
    }

    return value;
}


#endif
