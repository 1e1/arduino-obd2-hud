#ifndef wiring_H_
#define wiring_H_

#define __USE_NANO__


// Nano
#ifdef __USE_NANO__
#define DISPLAY_SPI_SCK             13
#define DISPLAY_SPI_MOSI            11
#define DISPLAY_SPI_MISO            12
#define DISPLAY_I2C_SCL             19
#define DISPLAY_I2C_SDA             18
#define DISPLAY_CHIP_SELECT_CANBUS  9
#define DISPLAY_CHIP_SELECT_SCREEN  8
#define DISPLAY_DATA_COMMAND        5
#define DISPLAY_RESET               4
#define DISPLAY_INT_CANBUS          2
#define SERIAL_PORT                 Serial
#define SERIAL_SPEED                9600
#define DISPLAY_FULLBUFFER          0
#endif

// XIAO
#ifdef __USE_XIAO__
#define DISPLAY_SPI_SCK             8
#define DISPLAY_SPI_MOSI            10
#define DISPLAY_SPI_MISO            9
#define DISPLAY_I2C_SCL             5
#define DISPLAY_I2C_SDA             4
#define DISPLAY_CHIP_SELECT_CANBUS  5
#define DISPLAY_CHIP_SELECT_SCREEN  4
#define DISPLAY_DATA_COMMAND        4
#define DISPLAY_RESET               2
#define DISPLAY_INT_CANBUS          1
#define SERIAL_PORT                 Serial
#define SERIAL_SPEED                115200
#define DISPLAY_FULLBUFFER          1
#endif


#define LOWPOWER_INTERRUPT_PIN      digitalPinToInterrupt(DISPLAY_INT_CANBUS)


#endif