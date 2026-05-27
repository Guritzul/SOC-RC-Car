#include "gearbox.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>

// Variabilă globală volatilă actualizată de aplicație
volatile uint8_t gearbox_ticks = 232; // Pornim din viteza întâi (232)
static Gearbox::Gear currentGear = Gearbox::GEAR_1;

// ============================================================
//  Interfață Abstractă (SOLID - ISP / DIP)
// ============================================================
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

// ============================================================
//  Implementare Concretă pe Registri ATmega328P (SOLID - LSP)
// ============================================================
class Atm328InterruptGearbox : public IGearbox
{
public:
    void init() override
    {
        // 1. Setează pinul D6 (PD6) ca ieșire (OUTPUT)
        DDRD |= (1 << DDD6);

        // 2. Asigurăm că pinul pornește pe LOW
        PORTD &= ~(1 << PORTD6);

        // 3. Activăm întreruperea la Overflow pentru Timer 1 (TOIE1)
        //    Timer 1 este deja configurat la 50Hz (20ms perioadă) de Steering::init().
        TIMSK1 |= (1 << TOIE1);

        Serial.println("[Gearbox] Initializat cu succes (Soft-PWM pe D6 prin T1 OVF si nested delay)");
    }

    void setGear(Gearbox::Gear gear) override
    {
        currentGear = gear;
        switch (gear)
        {
            case Gearbox::GEAR_1:
                gearbox_ticks = 232;
                Serial.println("[Gearbox] Schimbat in treapta 1 (ticks: 232)");
                break;
            case Gearbox::GEAR_2:
                gearbox_ticks = 145;
                Serial.println("[Gearbox] Schimbat in treapta 2 (ticks: 145)");
                break;
            case Gearbox::GEAR_3:
                gearbox_ticks = 55;
                Serial.println("[Gearbox] Schimbat in treapta 3 (ticks: 55)");
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

// Instanțiere statică a modulului
static Atm328InterruptGearbox gearboxHardware;

// ============================================================
//  Namespace Public expus către aplicație
// ============================================================
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

// ============================================================
//  Rutină de Întrerupere (ISR)
// ============================================================

// Apelat la fiecare Overflow al Timer 1 (la fiecare 20 ms / 50 Hz)
ISR(TIMER1_OVF_vect)
{
    // Permitem întreruperilor imbricate (nested interrupts) să ruleze în paralel.
    // Astfel, întreruperile Timer 0 (care actualizează millis() la fiecare 1ms)
    // pot rula fără nicio întrerupere sau întârziere în sistem.
    sei();

    PORTD |= (1 << PORTD6);                // Pune pinul D6 (PD6) pe HIGH
    
    // Calculăm durata pulsului în microsecunde pe baza ticks (1 tick = 64 µs)
    uint16_t delay_us = (uint16_t)gearbox_ticks * 64;
    delayMicroseconds(delay_us);
    
    PORTD &= ~(1 << PORTD6);               // Pune pinul D6 (PD6) pe LOW
}
