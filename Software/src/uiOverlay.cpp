#include "uiGlobal.h"
#include "uiOverlay.h"

#include <ESP8266WiFi.h>

//=============================================================================
// Pulse sprite
//=============================================================================

const uint8_t pulseDetected[] PROGMEM = {

    B01111110,
    B10000001,
    B10011001,
    B10111101,
    B10011001,
    B10000001,
    B01111110,
};

//=============================================================================
// Log sprite
//=============================================================================

const uint8_t logWritten[] PROGMEM = {

    B11111111,
    B10000001,
    B10000001,
    B10111001,
    B10110101,
    B10110101,
    B11111110,
};

//=============================================================================
// Wifi sprite
//=============================================================================

const uint8_t wifiConnected[] PROGMEM = {

    B00111100,
    B11000011,
    B00111100,
    B01000010,
    B00011000,
    B00100100,
    B00011000,
};

void uiOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
    String overlayText = String();

    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    display->setFont(ArialMT_Plain_10);

    overlayText = "vBat: ";
    overlayText += ((analogRead(A0) / 1023.0) * 4.43);
    overlayText += "V, UiFm: ";
    overlayText += state->currentFrame;

    display->drawString(128, 0, overlayText);

    if (WiFi.status() == WL_CONNECTED) {
        display->drawXbm(0, 2, 8, 7, wifiConnected);
    }

    if ((*(uiGlobalObject_s *)(state->userData)).impulse_p->GetUIImpulseStatus() == true) {
        display->drawXbm(11, 2, 8, 7, pulseDetected);
    }

    if (*(*(uiGlobalObject_s *)(state->userData)).logUpdate_p == true) {
        display->drawXbm(22, 2, 8, 7, logWritten);
    }
}
