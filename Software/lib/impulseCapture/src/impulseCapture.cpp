#include "impulseCapture.h"

static uint32_t _impulseCount;
static uint32_t _lastImpulseTime;
static uint32_t _instantenousWatt;

//=============================================================================
// Object constructors
//=============================================================================

ImpulseCapture::ImpulseCapture(uint8_t impulsePin) {
    _pin = impulsePin;
}

//=============================================================================
// Interrupt handler
//=============================================================================

void IRAM_ATTR ImpulseSensorInterrupt(void) {
    
    uint32_t currentImpulseTime = millis();
    uint32_t intervalImpulseTime = currentImpulseTime - _lastImpulseTime;

    if ((intervalImpulseTime < MAXIMUM_MILLI_SECOND_BTWN_IMPULSES) && (intervalImpulseTime >= MIMIMUM_MILLI_SECOND_BTWN_IMPULSES)) {
        _instantenousWatt = 3600000.0 / ((intervalImpulseTime / 1000.0) * 10000.0);
        //(float)(PULSES_PER_WATT_SECOND * 1000.0) / (float)intervalImpulseTime;
    }
    else {
        _instantenousWatt = 0;
    }

    _lastImpulseTime = currentImpulseTime;
    _impulseCount++;
}

//=============================================================================
// Public functions
//=============================================================================

void ImpulseCapture::Init(void) {
    pinMode(_pin, INPUT_PULLUP);

    _impulseCount = 0;
    _lastImpulseTime = 0;
    _instantenousWatt = 0;

    attachInterrupt(_pin, ImpulseSensorInterrupt, RISING);

    _lastUpdateTime = millis();
    _uiImpulseStatus = false;
    _lastImpulseCount = 0;
}

void ImpulseCapture::Update(void) {

    uint32_t currentTime = millis();
    uint32_t impulseCount = GetImpulseCount();

    if (impulseCount - _lastImpulseCount > UI_MINIMUM_IMPULSES_FOR_STATUS) {
        _lastImpulseCount = impulseCount;
        _lastUpdateTime = currentTime;
        _uiImpulseStatus = true;
    }
    else {
        if ((currentTime - _lastUpdateTime) >= UI_IMPULSE_STATUS_DEBOUNCE_MS) {
            _uiImpulseStatus = false;
        }
    }
}

bool ImpulseCapture::GetUIImpulseStatus(void) {
    return _uiImpulseStatus;
}

uint32_t ImpulseCapture::GetImpulseCount(void) {
    uint32_t impulseCount;

    noInterrupts();
    impulseCount = _impulseCount;
    interrupts();

    return impulseCount;
}

uint32_t ImpulseCapture::GetInstantWattUsgage(void) {
    uint32_t instantenousWatt;
    
    noInterrupts();
    instantenousWatt = _instantenousWatt; 
    interrupts();

    return instantenousWatt;
}

void ImpulseCapture::ClearInstantWattUsage(void) {

    noInterrupts();
    _instantenousWatt = 0;
    _impulseCount = 0;
    _lastImpulseCount = 0;
    interrupts();  
}