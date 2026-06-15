#include <Arduino.h>
#include <SPI.h>

#define HUD_LOG_LEVEL 0
#define HUD_USE_LED 1
#define HUD_LOG_LED 1

// Input backend selector. Default: native VW PQ CAN (CarEventCanVwPq).
// Define VH_INPUT_STREAM to drive the *real* OLED from the PC simulator (www/)
// over Serial instead — values are injected as "<sensorId> <value>\n" lines.
//#define VH_INPUT_STREAM
#define VH_STREAM_SPEED 115200   // simulator baud (must match the web page)

#include "_wiring.h"
#include "macro.h"



#include "_constants.h"
#include "CarEvent.h"
#include "Hudisplay.h"
#include "PowerManager.h"
#include "Workflow.h"



// ================================================
// Persistent hardware objects (file scope: must outlive setup()).
//
// STATIC memory placement (no heap): every backend lives at file scope, so it
// is zero-initialised in BSS at startup (deterministic — the old `new` without
// `()` default-init left scalars indeterminate; this is also why the earlier
// `_frameIndex` uninit-class bug cannot recur for globals). With no `new`,
// neither malloc nor the global operator new/delete are linked, and there is no
// heap fragmentation. The trade-off is accounting: the display rings/starfield
// and the CAN backend now show up in the linker's "Global variables" figure
// (they were on the heap before and therefore UNDER-reported) — the reported
// static RAM rises to the true, all-static value. That is the goal.
Workflow process;
// MCP_CAN board exists only on the MCP2515 transport boards (not the ESP32, which
// uses the native TWAI controller and has no VH_SPI_CS_CANBUS).
#if !defined(VH_INPUT_STREAM) && !defined(VH_INPUT_TWAI)
MCP_CAN canBoard(VH_SPI_CS_CANBUS);
#endif
// Display class chosen by the board-config buffer mode (VH_U8G2_BUFFER_MODE):
//   _2_ on 328 (byte-identical baseline), _F_ on the roomy boards.
VH_U8G2_DISPLAY_CLASS displayBoard(U8G2_R0, VH_SPI_CS_SCREEN, VH_DC0_SCREEN, VH_RESET_SCREEN);

// Input backend — only the SELECTED one is instantiated (don't allocate several).
#if defined(VH_INPUT_STREAM)
// PC simulator over Serial — drives the real OLED (www/)
CarEventStream busStream;
#elif defined(VH_INPUT_TWAI)
// ESP32 native CAN: TWAI controller transport + shared PQ decode (no MCP2515)
CarEventTwaiVwPq busTwai;
#else
// native VW PQ CAN backend (the car): generic MCP2515 transport + PQ decode
CarEventCanVwPq busCan;
#endif

// Hudisplay — 128x64 OLED
Hudisplay128x64 hud;



void setup() {
  Energy.free();
  //Energy.highCpu();
  delay(2000);
  Energy.begin();
  delay(1000);

  #ifdef VH_INT_CANBUS
  pinMode(VH_INT_CANBUS, INPUT);   // MCP2515 INT (deep-sleep wake); absent on the TWAI board
  #endif


  // =============================
  // CarEvent — input backend
  // =============================
  {
    #if defined(VH_INPUT_STREAM)
    // PC simulator over Serial — drives the real OLED (www/)
    VH_SERIAL_PORT.begin(VH_STREAM_SPEED);
    busStream.setStream(VH_SERIAL_PORT);
    process.setCarEvent(&busStream);
    #elif defined(VH_INPUT_TWAI)
    // ESP32 native CAN: install + start the TWAI driver (listen-only, 500 kbit/s)
    busTwai.begin();
    process.setCarEvent(&busTwai);
    #else
    // native VW PQ CAN backend (the car): generic MCP2515 transport + PQ decode
    canBoard.begin(MCP_STD, VH_CAN_BITRATE, VH_CAN_CRYSTAL);
    busCan.setBoard(&canBoard);
    process.setCarEvent(&busCan);
    #endif

    /* bench alternative — manual knob + button (declare a file-scope
       `CarEventHardware busHw;` next to the others above, then):
    busHw.setWiring(A0, A1);
    process.setCarEvent(&busHw);
    */
  }
  // =============================

  // =============================
  // Hudisplay — 128x64 OLED
  // =============================
  {
    hud.setBoard(&displayBoard);
    process.setHudisplay(&hud);

    /* bench alternative (declare a file-scope `HudisplayStream hudStream;`
       next to `hud` above, then):
    VH_SERIAL_PORT.begin(VH_SERIAL_SPEED);
    hudStream.setStream(VH_SERIAL_PORT);
    process.setHudisplay(&hudStream);
    */
  }
  // =============================
}


void loop() {
  process.update();
}
