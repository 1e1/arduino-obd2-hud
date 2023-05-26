#include <Arduino.h>
#include <SPI.h>

#define HUD_LOG_LEVEL 0
#define HUD_USE_LED 1
#define HUD_LOG_LED 1
#include "_wiring.h"
#include "macro.h"



#include "_constants.h"
#include "CarEvent.h"
#include "Hudisplay.h"
#include "PowerManager.h"
#include "Workflow.h"





// ================================================
Workflow process;



void setup() {
  Energy.free();
  //Energy.highCpu();
  delay(2000);
  Energy.begin();
  delay(1000);


  // =============================
  // CarEvent
  // =============================
  {
    pinMode(VH_INT_CANBUS, INPUT);
    
    /* */
    CarEventStream* bus = new CarEventStream;
    VH_SERIAL_PORT.begin(VH_SERIAL_SPEED);
    bus->setStream(VH_SERIAL_PORT);
    /* */
    /* * /
    CarEventHardware* bus = new CarEventHardware;
    bus->setWiring(A0, A1);
    /* */
    /* * /
    CarEventCan* bus = new CarEventCan;
    MCP_CAN mcp2515(VH_SPI_CS_CANBUS);
    mcp2515.begin(MCP_STD, CAN_500KBPS, MCP_8MHZ);
    bus->setBoard(&mcp2515);
    /* */

    process.setCarEvent(bus);
  }
  // =============================

  // =============================
  // Hudisplay
  // =============================
  {
    /* * /
    HudisplayStream* hud = new HudisplayStream;
    VH_SERIAL_PORT.begin(VH_SERIAL_SPEED);
    hud->setStream(VH_SERIAL_PORT);
    /* */
    /* */
    Hudisplay128x64* hud = new Hudisplay128x64;
    //U8G2_SSD1309_128X64_NONAME0_1_4W_SW_SPI hudBoard(U8G2_R0, VH_SCK, VH_SPI_COPI, DISPLAY_CHIP_SELECT, VH_DC0_SCREEN, VH_RESET_SCREEN);
    U8G2_SSD1309_128X64_NONAME0_2_4W_HW_SPI hudBoard(U8G2_R0, VH_SPI_CS_SCREEN, VH_DC0_SCREEN, VH_RESET_SCREEN);
    hud->setBoard(&hudBoard);
    /* */
    /* * /
    Hudisplay128x64* hud = new Hudisplay128x64;
    //U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI hudBoard(U8G2_R0, VH_SCK, VH_SPI_COPI, DISPLAY_CHIP_SELECT, VH_DC0_SCREEN, VH_RESET_SCREEN);
    U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI hudBoard(U8G2_R0, VH_SPI_CS_SCREEN, VH_DC0_SCREEN, VH_RESET_SCREEN);
    hud->setBoard(&hudBoard);
    /* */

    process.setHudisplay(hud);
  }
  // =============================
}


void loop() {
  process.update();
}

