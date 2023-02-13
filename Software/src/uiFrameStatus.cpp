#include "uiFrameStatus.h"
#include "uiGlobal.h"

#include <ESP8266WiFi.h>

void uiFrameStatus(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    String statusText = String();

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(ArialMT_Plain_10);

    if (WiFi.getMode() == WIFI_OFF) {
        statusText = "WIFI OFF";
    } else if (WiFi.getMode() == WIFI_STA) {
        statusText = "IP: ";
        statusText += WiFi.localIP().toString();
        statusText += ", " + String(WiFi.RSSI());
    } else if (WiFi.getMode() == WIFI_AP) {
        statusText = "AP IP: ";
        statusText += WiFi.softAPIP().toString();
    } else {
        statusText = "WIFI ERROR ";
        statusText += String(WiFi.getMode());
    }
    display->drawString(0 + x, 11 + y, statusText);

    statusText = "CNT: " + String((*(uiGlobalObject_s *)(state->userData)).impulse_p->GetImpulseCount());
    statusText += " W: " + String((*(uiGlobalObject_s *)(state->userData)).impulse_p->GetInstantWattUsgage());
    display->drawString(0 + x, 22 + y, statusText);
}
