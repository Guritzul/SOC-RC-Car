// ============================================================
//  esc.cpp
//  Driver ESC (Electronic Speed Controller) pentru motorul DC
//
//  Conexiuni:
//    Semnal PWM -> D10 (PB2 / OC1B)
//    Alimentare -> 6V-8.4V (baterie RC)
//    GND        -> GND comun
//
//  Timer 1 este configurat de Steering::init() la:
//    - Fast PWM, ICR1=39999 (TOP), Prescaler=8 -> 50Hz / 20ms perioada
//    - 1 tick = 0.5 µs
//
//  Intervale de semnal ESC standard (hobby):
//    - 1.0 ms (2000 ticks) -> inapoi maxim (reverse, daca ESC suporta)
//    - 1.5 ms (3000 ticks) -> stop / neutru (pozitie de armare)
//    - 2.0 ms (4000 ticks) -> inainte maxim
// ============================================================

#include <avr/io.h>
#include "Arduino.h"    // necesar pentru delay() la armare
#include "esc.h"

// --- Limite OCR1B (in ticks Timer 1 la 0.5µs/tick) ---
static const uint16_t ESC_STOP    = 3000;   // 1.5 ms - neutru / armare
static const uint16_t ESC_FWD_MAX = 4000;   // 2.0 ms - inainte maxim
static const uint16_t ESC_REV_MAX = 2000;   // 1.0 ms - inapoi maxim (reverse)

// --- Interfata Abstracta (SOLID - ISP / DIP) ---
class IEsc
{
public:
    virtual ~IEsc() {}
    virtual void init() = 0;
    virtual void setThrottle(int rawValue) = 0;
    virtual void stop() = 0;
};

// ============================================================
//  Implementare Concreta pe Registri ATmega328P (SOLID - LSP)
//  Foloseste Timer 1, canal B (OC1B) pe PB2 (D10)
// ============================================================
class Atm328Timer1Esc : public IEsc
{
public:
    void init() override
    {
        // 1. Configureaza PB2 (D10 / OC1B) ca OUTPUT
        DDRB |= (1 << DDB2);

        // 2. Activeaza OC1B pe Timer 1 (COM1B1=1):
        //    Timer 1 a fost deja initializat de Steering::init() cu:
        //      TCCR1A = COM1A1 | WGM11  (Fast PWM, OC1A activ)
        //      TCCR1B = WGM13 | WGM12 | CS11  (Prescaler 8, ICR1=TOP)
        //      ICR1 = 39999
        //    Adaugam COM1B1 pentru a activa si OC1B fara sa stricam OC1A
        TCCR1A |= (1 << COM1B1);

        // 3. Setam semnalul neutru (1.5 ms) pentru armare ESC
        //    ESC-ul trebuie sa primeasca semnal neutru la pornire
        //    pentru a iesi din modul de programare si a se arma
        OCR1B = ESC_STOP;

        Serial.println("[ESC] Armare... (semnal neutru 1.5ms timp de 3s)");
        delay(3000);  // Asteapta 3 secunde pentru armare ESC
        Serial.println("[ESC] Initializat pe registri (Timer 1 OC1B, Pin D10/PB2, 50Hz)");
    }

    void setThrottle(int rawValue) override
    {
        // Limitare de siguranta
        if (rawValue < 0)    rawValue = 0;
        if (rawValue > 1023) rawValue = 1023;

        // Mapare liniara a intervalului [0, 1023] in [ESC_REV_MAX, ESC_FWD_MAX]:
        //   0    -> 2000 ticks (1.0ms - reverse maxim)
        //   512  -> 3000 ticks (1.5ms - stop / neutru)
        //   1023 -> 4000 ticks (2.0ms - inainte maxim)
        //
        // Formula: OCR1B = ESC_REV_MAX + (rawValue * 2000) / 1023
        uint32_t calculatedTicks = (uint32_t)ESC_REV_MAX +
                                   ((uint32_t)rawValue * 2000UL) / 1023UL;

        OCR1B = (uint16_t)calculatedTicks;
    }

    void stop() override
    {
        OCR1B = ESC_STOP;
    }
};

// Instantiere statica
static Atm328Timer1Esc escHardware;

// ============================================================
//  Namespace Public expus catre aplicatie
// ============================================================
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
