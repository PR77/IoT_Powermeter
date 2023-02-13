#ifndef IMPULSE_CAPTURE_H
#define IMPULSE_CAPTURE_H

#include "Arduino.h"

//=============================================================================
// Defines
//=============================================================================

#define UI_IMPULSE_STATUS_DEBOUNCE_MS       2000
#define UI_MINIMUM_IMPULSES_FOR_STATUS      1

#define PULSES_PER_KILOWATT_HOUR            10000
#define WATTS_PER_KILOWATT                  1000

#define SECONDS_PER_HOUR                    3600
#define PULSES_PER_WATT_SECOND              SECONDS_PER_HOUR * (WATTS_PER_KILOWATT / PULSES_PER_KILOWATT_HOUR)

#define MAXIMUM_WATT_SUPPORTED              15000
#define MINIMUM_WATT_SUPPORTED              1
#define MIMIMUM_MILLI_SECOND_BTWN_IMPULSES  25      // (1 / (MAXIMUM_WATT_SUPPORTED / 360)) * 1000
#define MAXIMUM_MILLI_SECOND_BTWN_IMPULSES  360000  // (1 / (MINIMUM_WATT_SUPPORTED / 360)) * 1000

//=============================================================================
// Classes
//=============================================================================

class ImpulseCapture
{
    public:
        ImpulseCapture(uint8_t impulsePin);

        void Init(void);
        void Update(void);
        bool GetUIImpulseStatus(void);
        uint32_t GetImpulseCount(void);
        uint32_t GetInstantWattUsgage(void);
        void ClearInstantWattUsage(void);

    private:
        uint8_t _pin;
        bool _uiImpulseStatus;
        uint32_t _lastImpulseCount;
        uint32_t _lastUpdateTime;
};

#endif // IMPULSE_CAPTURE_H
