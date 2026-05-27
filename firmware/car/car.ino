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
#include "radio_rx.h"
#include "steering.h"

static const int INTERVAL_SENZORI = 100;
static unsigned long _ultimaCitire = 0;

static float distFata  = 999.0;
static float distSpate = 999.0;

// Starea curenta a comenzilor radio (initilizata cu pozitii neutre)
static Payload _dateRadio = {512, 512, false, false, false};

// ============================================================
void setup() {
    Serial.begin(9600);
    Serial.println("=== Masina RC - pornire sistem ===");

    SenzorFata::init();
    SenzorSpate::init();
    SenzorLumina::init();
    Buzzer::init();
    Leduri::init();
    RadioRx::init();
    Steering::init();

    Serial.println("=== Sistem gata ===");
}

// ============================================================
void loop() {
    // --- Citire Radio non-blocking (frecventa maxima de interogare) ---
    Payload dateNoi;
    if (RadioRx::receive(dateNoi)) {
        _dateRadio = dateNoi;

        // Actualizare instanta unghi servodirectie (Graupner C 577) pe registri Timer 1
        Steering::setAngle(_dateRadio.steering);

        Serial.print("[Radio] Pachet primit | THR: ");
        Serial.print(_dateRadio.throttle);
        Serial.print(" | STR: ");
        Serial.print(_dateRadio.steering);
        Serial.print(" | BUZ: ");
        Serial.print(_dateRadio.buzz);
        Serial.print(" | SWL: ");
        Serial.print(_dateRadio.swLeft);
        Serial.print(" | SWR: ");
        Serial.println(_dateRadio.swRight);
    }

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

    // --- Buzzer non-blocking (activat de obstacole sau manual din butonul de pe telecomanda) ---
    bool pericol = SenzorFata::estePericol() || SenzorSpate::estePericol();
    bool atentie = SenzorFata::esteAtentie() || SenzorSpate::esteAtentie();
    Buzzer::update(pericol || _dateRadio.buzz, atentie);
}
