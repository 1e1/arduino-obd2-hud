#include <Arduino.h>

#define HUD_LOG_LEVEL 0
#define HUD_USE_LED 1
#define HUD_LOG_LED 1
#include "_wiring.h"
#include "macro.h"



#include <mcp_can.h>
#include <U8g2lib.h>
#include "_constants.h"
#include "CarEventObd2.h"
#include "Hudisplay.h"
#include "Workflow.h"





// ================================================
Workflow process;



void setup() {

  // =============================
  // CarEventObd2
  // =============================
  {
    /* * /
    CarEventObd2Stream bus;
    SERIAL_PORT.begin(SERIAL_SPEED);
    bus.setStream(SERIAL_PORT);
    /* */
    /* * /
    CarEventObd2Hardware bus;
    bus.setWiring(A0, A1);
    /* */
    /* */
    CarEventObd2Can bus;
    MCP_CAN mcp2515(DISPLAY_CHIP_SELECT_CANBUS);
    mcp2515.begin(MCP_STD, CAN_500KBPS, MCP_8MHZ);
    bus.setBoard(&mcp2515);
    /* */

    process.setCarEventObd2(&bus);
  }
  // =============================

  // =============================
  // Hudisplay
  // =============================
  {
    /* */
    HudisplayStream hud;
    SERIAL_PORT.begin(SERIAL_SPEED);
    hud.setStream(SERIAL_PORT);
    /* */
    /* * /
    Hudisplay128x64 hud;
    //U8G2_SSD1309_128X64_NONAME0_1_4W_SW_SPI hudBoard(U8G2_R0, DISPLAY_SPI_SCK, DISPLAY_SPI_MOSI, DISPLAY_CHIP_SELECT, DISPLAY_DATA_COMMAND, DISPLAY_RESET);
    U8G2_SSD1309_128X64_NONAME0_2_4W_HW_SPI hudBoard(U8G2_R0, DISPLAY_CHIP_SELECT_SCREEN, DISPLAY_DATA_COMMAND, DISPLAY_RESET);
    hud.setBoard(&hudBoard);
    /* */
    /* * /
    Hudisplay128x64 hud;
    //U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI hudBoard(U8G2_R0, DISPLAY_SPI_SCK, DISPLAY_SPI_MOSI, DISPLAY_CHIP_SELECT, DISPLAY_DATA_COMMAND, DISPLAY_RESET);
    U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI hudBoard(U8G2_R0, DISPLAY_CHIP_SELECT_SCREEN, DISPLAY_DATA_COMMAND, DISPLAY_RESET);
    hud.setBoard(&hudBoard);
    /* */

    process.setHudisplay(&hud);
  }
  // =============================
}


void loop() {
  process.update();
}

