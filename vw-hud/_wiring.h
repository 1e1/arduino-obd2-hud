#ifndef wiring_H_
#define wiring_H_

#define __USE_NANO__


// Nano
#ifdef __USE_NANO__
#define VH_SCK                      13
#define VH_SPI_COPI                 11  // MOSI
#define VH_SPI_CIPO                 12  // MISO
#define VH_I2C_SCL                  19
#define VH_I2C_SDA                  18
#define VH_SPI_CS_CANBUS            9
#define VH_SPI_CS_SCREEN            8
#define VH_DC0_SCREEN               5   // DC
#define VH_RESET_SCREEN             4
#define VH_INT_CANBUS               2

#define VH_SERIAL_PORT              Serial
#define VH_SERIAL_SPEED             9600
#define VH_DISPLAY_FULLBUFFER       0
#endif

// XIAO
#ifdef __USE_XIAO__
#define VH_SCK                      8
#define VH_SPI_COPI                 10  // MOSI
#define VH_SPI_CIPO                 9   // MISO
#define VH_I2C_SCL                  5
#define VH_I2C_SDA                  4
#define VH_SPI_CS_CANBUS            5
#define VH_SPI_CS_SCREEN            4
#define VH_DC0_SCREEN               4   // DC
#define VH_RESET_SCREEN             2
#define VH_INT_CANBUS               1

#define VH_SERIAL_PORT              Serial
#define VH_SERIAL_SPEED             115200
#define VH_DISPLAY_FULLBUFFER       1
#endif


#define LOWPOWER_INTERRUPT_PIN      digitalPinToInterrupt(VH_INT_CANBUS)


#endif