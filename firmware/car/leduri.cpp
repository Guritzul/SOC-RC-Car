// Conexiuni: Far fata -> A1 (PC1), Stop spate -> A2 (PC2)
#include <avr/io.h>
#include "Arduino.h"
#include "leduri.h"

#define FAR_FATA_BIT    PC1
#define STOP_SPATE_BIT  PC2

// Ciclu PWM: PERIOD_MS ms total, LED aprins DUTY_ON_MS ms
static const unsigned int PWM_PERIOD_MS  = 20;    // frecventa ~50 Hz
static const unsigned int PWM_DUTY_ON_MS = 10;    // 50% duty cycle

typedef enum {
    STOP_OFF    = 0,
    STOP_NORMAL = 1,   // PWM software 50%
    STOP_FRANA  = 2    // 100% ON
} StopState;

typedef enum {
    FAR_OFF = 0,
    FAR_DRL = 1,       // PWM software 50%
    FAR_ON  = 2        // 100% ON
} FarState;

static StopState _stopState = STOP_OFF;
static FarState _farState = FAR_OFF;

static unsigned long _pwmLastTick = 0;
static bool _pwmPhase = false;  // false = OFF phase, true = ON phase

namespace Leduri {

    void init() {
        DDRC |= (1 << FAR_FATA_BIT);
        DDRC |= (1 << STOP_SPATE_BIT);

        PORTC &= ~(1 << FAR_FATA_BIT);
        PORTC &= ~(1 << STOP_SPATE_BIT);

        _stopState = STOP_OFF;
        _farState = FAR_OFF;
        _pwmLastTick = 0;
        _pwmPhase = false;

        Serial.println("[Leduri] initializat pe registri (Far=A1/PC1, Stop=A2/PC2)");
    }

    void farFataOn() {
        _farState = FAR_ON;
        PORTC |= (1 << FAR_FATA_BIT);
    }

    void farFataDrl() {
        _farState = FAR_DRL;
    }

    void farFataOff() {
        _farState = FAR_OFF;
        PORTC &= ~(1 << FAR_FATA_BIT);
    }

    void stopNormal() {
        _stopState = STOP_NORMAL;
    }

    void stopFrana() {
        _stopState = STOP_FRANA;
        PORTC |= (1 << STOP_SPATE_BIT);
    }

    void stopOff() {
        _stopState = STOP_OFF;
        PORTC &= ~(1 << STOP_SPATE_BIT);
    }

    void update() {
        // Logica digitala simpla (100% sau 0%) suprascrie faza curenta PWM pentru a avea reactie rapida
        if (_stopState == STOP_OFF)   PORTC &= ~(1 << STOP_SPATE_BIT);
        if (_stopState == STOP_FRANA) PORTC |= (1 << STOP_SPATE_BIT);
        
        if (_farState == FAR_OFF) PORTC &= ~(1 << FAR_FATA_BIT);
        if (_farState == FAR_ON)  PORTC |= (1 << FAR_FATA_BIT);

        // PWM Software sincronizat pentru DRL si StopNormal
        if (_stopState == STOP_NORMAL || _farState == FAR_DRL) {
            unsigned long acum = millis();
            unsigned long elapsed = acum - _pwmLastTick;

            if (!_pwmPhase) {
                if (elapsed >= (PWM_PERIOD_MS - PWM_DUTY_ON_MS)) {
                    _pwmPhase = true;
                    _pwmLastTick = acum;
                    if (_stopState == STOP_NORMAL) PORTC |= (1 << STOP_SPATE_BIT);
                    if (_farState == FAR_DRL)      PORTC |= (1 << FAR_FATA_BIT);
                }
            } else {
                if (elapsed >= PWM_DUTY_ON_MS) {
                    _pwmPhase = false;
                    _pwmLastTick = acum;
                    if (_stopState == STOP_NORMAL) PORTC &= ~(1 << STOP_SPATE_BIT);
                    if (_farState == FAR_DRL)      PORTC &= ~(1 << FAR_FATA_BIT);
                }
            }
        }
    }

} // namespace Leduri