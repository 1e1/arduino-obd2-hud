#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>

#define MODE_PRODUCTION             0
#define MODE_MOCK                   1
#define MODE_HUMAN                  2

#define VH_SERIAL_PORT              Serial
#define VH_SERIAL_SPEED             115200
#define __USE_MEGA__                1
#define __USE_MODE__                MODE_PRODUCTION
#define FRAME_START                 0xF0
#define FRAME_END                   0xAA

#if __USE_NANO__
#define VH_SCK                      13
#define VH_SPI_COPI                 11  // MOSI
#define VH_SPI_CIPO                 12  // MISO
#define VH_SPI_CS_CANBUS            9
#define VH_INT_CANBUS               2
#endif

#if __USE_MEGA__
#define VH_SCK                      52
#define VH_SPI_COPI                 51  // MOSI
#define VH_SPI_CIPO                 50  // MISO
#define VH_SPI_CS_CANBUS            49
#define VH_INT_CANBUS               21
#endif


unsigned long rxId;
byte len = 0;
byte rxBuf[8];
#if __IS_HUMAN__
char buffer[50];
#else
byte idBuf[4];
#endif


MCP_CAN mcp2515(VH_SPI_CS_CANBUS);


void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  VH_SERIAL_PORT.begin(VH_SERIAL_SPEED);
  pinMode(VH_INT_CANBUS, INPUT);

  #if __USE_MODE__ == MODE_HUMAN
  VH_SERIAL_PORT.println(F("-- CANBUS init"));
  #endif

  while (mcp2515.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) != CAN_OK) {
    #if __USE_MODE__ == MODE_HUMAN
    VH_SERIAL_PORT.println(F("  KO..."));
    #endif
    delay(1000);
  }
  
  #if __USE_MODE__ == MODE_HUMAN
  VH_SERIAL_PORT.println(F("  OK!"));
  #endif

  mcp2515.setMode(MCP_LISTENONLY);
  
  #if __USE_MODE__ == MODE_HUMAN
  VH_SERIAL_PORT.println("-- GO!");
  #endif

  VH_SERIAL_PORT.flush();

  digitalWrite(LED_BUILTIN, LOW);
}


void loop()
{
  #if __USE_MODE__ == MODE_MOCK
  idBuf[0] = random(0, 0x100);
  idBuf[1] = random(0, 0x100);
  idBuf[2] = random(0, 0x100);
  idBuf[3] = random(0, 0x100);

  rxId = millis();

  rxBuf[0] = ((rxId >>  0) + random(-2,2)) & 0xFF;
  rxBuf[1] = ((rxId >>  8) + random(-2,2)) & 0xFF;
  rxBuf[2] = ((rxId >> 16) + random(-2,2)) & 0xFF;
  rxBuf[3] = ((rxId >> 24) + random(-2,2)) & 0xFF;
  rxBuf[4] = ((rxId >>  0) + random(-9,9)) & 0xFF;
  rxBuf[5] = ((rxId >>  8) + random(-9,9)) & 0xFF;
  rxBuf[6] = ((rxId >> 16) + random(-9,9)) & 0xFF;
  rxBuf[7] = ((rxId >> 24) + random(-9,9)) & 0xFF;

  VH_SERIAL_PORT.write(FRAME_START);
  VH_SERIAL_PORT.write(idBuf, 4);
  VH_SERIAL_PORT.write(rxBuf, 8);
  VH_SERIAL_PORT.write(FRAME_END);
  #else

  if (digitalRead(VH_INT_CANBUS) == LOW) {
    analogWrite(LED_BUILTIN, 63);
    mcp2515.readMsgBuf(&rxId, &len, rxBuf);
    digitalWrite(LED_BUILTIN, LOW);
    
    #if __USE_MODE__ == MODE_HUMAN
    sprintf(buffer, "ID: %.8lX  DATA: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n\r",
            rxId, rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3], rxBuf[4], rxBuf[5], rxBuf[6], rxBuf[7]);
    
    VH_SERIAL_PORT.println(buffer);
    #else
    idBuf[0] = (rxId >>  0) & 0xFF;
    idBuf[1] = (rxId >>  8) & 0xFF;
    idBuf[2] = (rxId >> 16) & 0xFF;
    idBuf[3] = (rxId >> 24) & 0xFF;

    VH_SERIAL_PORT.write(FRAME_START);
    VH_SERIAL_PORT.write(idBuf, 4);
    VH_SERIAL_PORT.write(rxBuf, 8);
    VH_SERIAL_PORT.write(FRAME_END);
    #endif
  }

  #endif
}