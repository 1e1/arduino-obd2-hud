#ifndef constants_H_
#define constants_H_


#define TANK_CAPACITY_MAX           70
#define SCREEN_BRIGHTNESS_DAY       200
#define SCREEN_BRIGHTNESS_NIGHT     100

#define K_RATIO_FUEL_AIR_HYDROGEN   34.0
#define K_RATIO_FUEL_AIR_NATURALGAS 17.2
#define K_RATIO_FUEL_AIR_PROPANE    15.5
#define K_RATIO_FUEL_AIR_GASOLINE   14.7
#define K_RATIO_FUEL_AIR_DIESEL     14.6
#define K_RATIO_FUEL_AIR_ETHANOL    09.0
#define K_RATIO_FUEL_AIR_METHANOL   06.4

#define OBD_PID_TANK_CAPACITY       "UNKNOWN"
#define OBD_PID_RPM_VALUE           0x0C /* 2-bytes */
#define OBD_PID_SPEED_VALUE         0x0D /* 1-byte */
#define OBD_PID_MAF_VALUE           0x10 /* 2-bytes */
#define OBD_PID_TANK_LOAD           0x2F /* 1-byte */
#define OBD_PID_ENGINE_FUEL_RATE    0x5E /* 2-bytes */
#define OBD_PID_TORQUE_LOAD         0x62 /* 1-byte */
#define OBD_PID_GEAR_POSITION       0xA4 /* 4-bytes */
#define OBD_PID_ODOMETER_VALUE      0xA6 /* 4-bytes */


#endif