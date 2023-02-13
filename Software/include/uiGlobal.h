#ifndef UI_GLOBAL
#define UI_GLOBAL

#include "Arduino.h"

#include <OLEDDisplay.h>
#include <OLEDDisplayFonts.h>
#include <OLEDDisplayUi.h>

#include <DHTesp.h>
#include <batteryHistogram.h>
#include <impulseCapture.h>

//=============================================================================
// Types
//=============================================================================

typedef enum {

    uiGlobalUXActive = 0,

} uiGlobalState_e;

typedef struct {

    TempAndHumidity *dhtTempAndHumidity_p;
    BatteryHistogram *battery_p;
    ImpulseCapture *impulse_p;
    bool *logUpdate_p;

} uiGlobalObject_s;

//=============================================================================
// Externals
//=============================================================================

extern uiGlobalObject_s uiGlobalObject;

#endif // UI_GLOBAL
