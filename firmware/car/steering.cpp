#include "steering.h"
#include <Arduino.h>

// ============================================================
//  Interfață Abstractă (SOLID - ISP / DIP)
// ============================================================
class ISteering
{
public:
    virtual ~ISteering() {}
    virtual void init() = 0;
    virtual void setAngle(int rawValue) = 0;
};

// ============================================================
//  Implementare Concretă pe Registri ATmega328P (SOLID - LSP)
// ============================================================
class Atm328Timer1Steering : public ISteering
{
public:
    void init() override
    {
        // 1. Setează pinul PB1 (Digital 9 / OC1A) ca OUTPUT
        DDRB |= (1 << DDB1);

        // 2. Configurare Timer 1 (16-bit) pentru servomotor (50Hz / perioadă de 20ms):
        //    - TCCR1A:
        //        - COM1A1 = 1 (Curăță pinul OC1A la comparare egală cu OCR1A, pune pe HIGH la capătul de jos)
        //        - WGM11  = 1 (Face parte din modul 14 - Fast PWM cu ICR1 ca TOP)
        TCCR1A = (1 << COM1A1) | (1 << WGM11);

        //    - TCCR1B:
        //        - WGM13 = 1, WGM12 = 1 (Mod 14 Fast PWM - ICR1 ca TOP)
        //        - CS11  = 1 (Prescaler clkI/O / 8 -> Tactul Timerului = 16 MHz / 8 = 2 MHz, adică 0.5 µs per tick)
        TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

        //    - ICR1 (Valoare TOP):
        //        - Perioada cerută de servomotor: 20 ms
        //        - Număr tick-uri = 20 ms / 0.5 µs = 40.000 de pași
        //        - Setează ICR1 la 39999 (deoarece numărarea pornește de la 0)
        ICR1 = 39999;

        //    - OCR1A (Valoare implicită inițială pe centru / neutru):
        //        - 1.5 ms = 1500 µs
        //        - Număr tick-uri = 1500 µs / 0.5 µs = 3000 de pași
        OCR1A = 3000;

        Serial.println("[Steering] Initializat cu succes pe registri (Timer 1, Pin 9 / PB1, Freq=50Hz)");
    }

    void setAngle(int rawValue) override
    {
        // Limitare de siguranță pentru semnal
        if (rawValue < 0) rawValue = 0;
        if (rawValue > 1023) rawValue = 1023;

        // Mapare liniară precisă a intervalului [0, 1023] în [2000, 4000] tick-uri pentru OCR1A:
        //    - 0    (Stânga maxim) -> 1.0 ms -> OCR1A = 2000
        //    - 512  (Centru)       -> 1.5 ms -> OCR1A = 3000
        //    - 1023 (Dreapta maxim)-> 2.0 ms -> OCR1A = 4000
        //
        // Formula: OCR1A = 2000 + (steering * 2000) / 1023
        uint32_t calculatedTicks = 2000 + (((uint32_t)rawValue * 2000) / 1023);
        
        // Scriem direct în registrul de comparare OCR1A
        OCR1A = (uint16_t)calculatedTicks;
    }
};

// Instanțiere statică a modulului
static Atm328Timer1Steering steeringHardware;

// ============================================================
//  Namespace Public expus către aplicație
// ============================================================
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
