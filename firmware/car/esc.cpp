#include <avr/io.h>
#include "Arduino.h"
#include "esc.h"

// OCR1B limits in ticks Timer 1 (0.5µs/tick)
static const uint16_t ESC_STOP    = 3000;   // 1.5 ms - neutru
static const uint16_t ESC_FWD_MAX = 4000;   // 2.0 ms - inainte maxim
static const uint16_t ESC_REV_MAX = 2000;   // 1.0 ms - inapoi maxim

class IEsc
{
public:
    virtual ~IEsc() {}
    virtual void init() = 0;
    virtual void setThrottle(int rawValue) = 0;
    virtual void stop() = 0;
};

class Atm328Timer1Esc : public IEsc
{
public:
    void init() override
    {
        DDRB |= (1 << DDB2);

        // Activăm canalul OC1B pe Timer 1 fără a perturba OC1A ( Steering )
        TCCR1A |= (1 << COM1B1);

        OCR1B = ESC_STOP;

        Serial.println("[ESC] Armare... (semnal neutru 1.5ms timp de 4s)");
        delay(4000);
        Serial.println("[ESC] Initializat pe registri (Timer 1 OC1B, Pin D10/PB2, 50Hz)");
    }

    void setThrottle(int rawValue) override
    {
        if (rawValue < 0)    rawValue = 0;
        if (rawValue > 1023) rawValue = 1023;

        // Mapare [0, 1023] în [ESC_REV_MAX, ESC_FWD_MAX]:
        // OCR1B = ESC_REV_MAX + (rawValue * 2000) / 1023
        uint32_t calculatedTicks = (uint32_t)ESC_REV_MAX +
                                   ((uint32_t)rawValue * 2000UL) / 1023UL;

        OCR1B = (uint16_t)calculatedTicks;
    }

    void stop() override
    {
        OCR1B = ESC_STOP;
    }
};

static Atm328Timer1Esc escHardware;

namespace Esc
{
    void init()
    {
        escHardware.init();
    }

    void setThrottle(int rawValue)
    {
        escHardware.setThrottle(rawValue);
    }

    void stop()
    {
        escHardware.stop();
    }
}
