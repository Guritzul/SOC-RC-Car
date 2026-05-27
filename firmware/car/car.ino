// ============================================================
//  car.ino
//  Masina RC - coordonator module
//
//  Module active:
//    - SenzorFata   (HC-SR04, Trig=2, Echo=3)
//    - SenzorSpate  (HC-SR04, Trig=4, Echo=5)
//    - SenzorLumina (HW-072, AO=A0)
//    - Buzzer       (Pin 8)
//    - Leduri       (Far=9, Stop=10)
// ============================================================

#include "senzor_fata.h"
#include "senzor_spate.h"
#include "senzor_lumina.h"
#include "buzzer.h"
#include "leduri.h"

static const int INTERVAL_SENZORI = 100;
static unsigned long _ultimaCitire = 0;

static float distFata  = 999.0;
static float distSpate = 999.0;

// ============================================================
void setup() {
    Serial.begin(9600);
    Serial.println("=== Masina RC - pornire sistem ===");

    SenzorFata::init();
    SenzorSpate::init();
    SenzorLumina::init();
    Buzzer::init();
    Leduri::init();

    Serial.println("=== Sistem gata ===");
}

// ============================================================
void loop() {
    unsigned long acum = millis();

    if (acum - _ultimaCitire >= INTERVAL_SENZORI) {
        _ultimaCitire = acum;

        // --- Senzori obstacole ---
        distFata  = SenzorFata::citeste();
        distSpate = SenzorSpate::citeste();
        SenzorLumina::citeste();

        Serial.print("FATA: ");
        Serial.print(distFata, 1);
        Serial.print(" cm  |  SPATE: ");
        Serial.print(distSpate, 1);
        Serial.print(" cm  |  LUMINA: ");
        Serial.println(SenzorLumina::citeste());

        // --- Far fata: aprindere automata dupa lumina ---
        if (SenzorLumina::esteIntuneric()) {
            Leduri::farFataOn();
        } else {
            Leduri::farFataOff();
        }

        // --- Stop spate ---
        // Prioritate: frana > lumini > stins
        if (SenzorSpate::estePericol()) {
            Leduri::stopFrana();           // obstacol spate -> frana automata
        } else if (SenzorLumina::esteIntuneric()) {
            Leduri::stopNormal();          // lumini aprinse -> stop slab (pozitie)
        } else {
            Leduri::stopOff();             // zi + fara frana -> stins
        }
    }

    // --- Buzzer non-blocking ---
    bool pericol = SenzorFata::estePericol() || SenzorSpate::estePericol();
    bool atentie = SenzorFata::esteAtentie() || SenzorSpate::esteAtentie();
    Buzzer::update(pericol, atentie);
}
