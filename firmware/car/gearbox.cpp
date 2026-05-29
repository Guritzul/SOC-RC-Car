#include "gearbox.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>

volatile uint8_t gearbox_ticks = 230; // Pornim din viteza întâi (230)
static Gearbox::Gear currentGear = Gearbox::GEAR_1;

class IGearbox
{
public:
    virtual ~IGearbox() {}
    virtual void init() = 0;
    virtual void setGear(Gearbox::Gear gear) = 0;
    virtual Gearbox::Gear getGear() = 0;
    virtual void shiftUp() = 0;
    virtual void shiftDown() = 0;
};

class Atm328InterruptGearbox : public IGearbox
{
public:
    void init() override
    {
        DDRD |= (1 << DDD6);
        PORTD &= ~(1 << PORTD6);

        // Mod CTC (Clear Timer on Compare Match)
        TCCR2A = (1 << WGM21);
        TCCR2B = 0;
        TCNT2 = 0;
        
        TIMSK2 |= (1 << OCIE2A);

        // Timer 1 este deja configurat la 50Hz (20ms) de Steering
        TIMSK1 |= (1 << TOIE1);

        Serial.println("[Gearbox] Initializat cu succes (Timer1 OVF + Timer2 COMPA non-blocant)");
    }

    void setGear(Gearbox::Gear gear) override
    {
        currentGear = gear;
        switch (gear)
        {
            case Gearbox::GEAR_1:
                gearbox_ticks = 230;
                Serial.println("[Gearbox] Schimbat in treapta 1 (ticks: 230)");
                break;
            case Gearbox::GEAR_2:
                gearbox_ticks = 141;
                Serial.println("[Gearbox] Schimbat in treapta 2 (ticks: 141)");
                break;
            case Gearbox::GEAR_3:
                gearbox_ticks = 90;
                Serial.println("[Gearbox] Schimbat in treapta 3 (ticks: 90)");
                break;
        }
    }

    Gearbox::Gear getGear() override
    {
        return currentGear;
    }

    void shiftUp() override
    {
        if (currentGear == Gearbox::GEAR_1)
        {
            setGear(Gearbox::GEAR_2);
        }
        else if (currentGear == Gearbox::GEAR_2)
        {
            setGear(Gearbox::GEAR_3);
        }
    }

    void shiftDown() override
    {
        if (currentGear == Gearbox::GEAR_3)
        {
            setGear(Gearbox::GEAR_2);
        }
        else if (currentGear == Gearbox::GEAR_2)
        {
            setGear(Gearbox::GEAR_1);
        }
    }
};

static Atm328InterruptGearbox gearboxHardware;

namespace Gearbox
{
    void init()
    {
        gearboxHardware.init();
    }

    void setGear(Gear gear)
    {
        gearboxHardware.setGear(gear);
    }

    void shiftUp()
    {
        gearboxHardware.shiftUp();
    }

    void shiftDown()
    {
        gearboxHardware.shiftDown();
    }

    Gear getGear()
    {
        return gearboxHardware.getGear();
    }
}

// Apelat la fiecare Overflow al Timer 1 (la fiecare 20 ms / 50 Hz)
ISR(TIMER1_OVF_vect)
{
    PORTD |= (1 << PORTD6);
    TCNT2 = 0;
    OCR2A = gearbox_ticks;
    
    // Prescaler 128 (1 tick = 8 µs)
    TCCR2B = (1 << CS22) | (1 << CS20);
}

// Apelat când Timer 2 atinge valoarea OCR2A
ISR(TIMER2_COMPA_vect)
{
    PORTD &= ~(1 << PORTD6);
    TCCR2B = 0;
}
