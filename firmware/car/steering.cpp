#include "steering.h"
#include <Arduino.h>

class ISteering
{
public:
    virtual ~ISteering() {}
    virtual void init() = 0;
    virtual void setAngle(int rawValue) = 0;
};

class Atm328Timer1Steering : public ISteering
{
public:
    void init() override
    {
        DDRB |= (1 << DDB1) | (1 << DDB2);

        // Timer 1 (16-bit) configurat pentru servomotor (50Hz / 20ms perioadă):
        // Prescaler 8 -> tact 2 MHz, adică 0.5 µs per tick.
        // TCCR1A: COM1A1=1 (curăță OC1A la comparare), COM1B1=1 (pentru ESC pe OC1B), WGM11=1 (Fast PWM, ICR1 ca TOP)
        TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);

        // TCCR1B: WGM13=1, WGM12=1 (Fast PWM), CS11=1 (Prescaler 8)
        TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

        // ICR1 (TOP): 20 ms / 0.5 µs = 40.000 pași (valoare TOP 39999)
        ICR1 = 39999;

        // OCR1A (implicit centru): 1.5 ms = 1500 µs / 0.5 µs = 3000 pași
        OCR1A = 3000;
        OCR1B = 3000;

        Serial.println("[Steering] Initializat cu succes pe registri (Timer 1 OC1A+OC1B, Pin D9/PB1, Freq=50Hz)");
    }

    void setAngle(int rawValue) override
    {
        if (rawValue < 0) rawValue = 0;
        if (rawValue > 1023) rawValue = 1023;

        // Mapare [0, 1023] în [2000, 4000] ticks (1.0ms - 2.0ms):
        // Formula: OCR1A = 2000 + (steering * 2000) / 1023
        uint32_t calculatedTicks = 2000 + (((uint32_t)rawValue * 2000) / 1023);
        
        OCR1A = (uint16_t)calculatedTicks;
    }
};

static Atm328Timer1Steering steeringHardware;

namespace Steering
{
    void init()
    {
        steeringHardware.init();
    }

    void setAngle(int rawValue)
    {
        steeringHardware.setAngle(rawValue);
    }
}
