// ============================================================
//  car.ino
//  Masina RC - coordonator module
//
//  Module active:
//    - SenzorFata   (HC-SR04, Echo=D2/PD2, Trig=D3/PD3)
//    - SenzorSpate  (HC-SR04, Echo=D4/PD4, Trig=D5/PD5)
//    - SenzorLumina (HW-072,  AO=A0/PC0/ADC0)
//    - Buzzer       (A3/PC3)
//    - Leduri       (Far=A1/PC1, Stop=A2/PC2 cu PWM software)
//    - RadioRx      (nRF24L01: MOSI=D11, MISO=D12, SCK=D13, CE=D7, CSN=D10)
//    - Steering     (Servo directie, D9/PB1/OC1A, Timer 1)
//    - Esc          (Motor principal, D10/PB2/OC1B, Timer 1 canal B)
//
//  Control:
//    - throttle (joystick stang, 0-1023) -> ESC -> motor
//    - steering (joystick drept, 0-1023) -> Servo directie
// ============================================================

#include "senzor_fata.h"
#include "senzor_spate.h"
#include "senzor_lumina.h"
#include "buzzer.h"
#include "leduri.h"
#include "radio_rx.h"
#include "steering.h"
#include "esc.h"
#include "gearbox.h"

static const int INTERVAL_SENZORI = 100;
static const unsigned long RX_TIMEOUT = 500;
static unsigned long _ultimaCitire = 0;
static unsigned long _ultimaRx = 0;
static bool _rxActive = false;

static float distFata = 999.0;
static float distSpate = 999.0;

// Starea curenta a comenzilor radio (initializata cu pozitii neutre)
static Payload _dateRadio = {512, 512, false, false, false};

// ============================================================
void setup()
{
    Serial.begin(9600);
    Serial.println("=== Masina RC - pornire sistem ===");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    SenzorFata::init();
    SenzorSpate::init();
    SenzorLumina::init();
    Buzzer::init();
    Leduri::init();
    RadioRx::init();

    // Steering se initializeaza PRIMUL (configureaza Timer 1 complet)
    // ESC se initializeaza AL DOILEA (adauga COM1B1 la TCCR1A existent + armare 3s)
    // Gearbox se initializeaza al treilea (adauga TOIE1 la TIMSK1)
    Steering::init();
    Esc::init();
    Gearbox::init();

    Serial.println("=== Sistem gata ===");
}

// ============================================================
void loop()
{
    unsigned long acum = millis();

    // --- Citire Radio non-blocking (frecventa maxima de interogare) ---
    Payload dateNoi;
    if (RadioRx::receive(dateNoi))
    {
        _dateRadio = dateNoi;
        _ultimaRx = acum;

        if (!_rxActive)
        {
            Serial.println("[Radio] Conexiune stabilita!");
            _rxActive = true;
        }

        digitalWrite(LED_BUILTIN, HIGH);

        // --- Servo directie: joystick drept (0-1023) -> OC1A (D9) ---
        Steering::setAngle(_dateRadio.steering);

        // --- Motor (ESC): joystick stang (0-1023) -> OC1B (D10) ---
        // Securitate: daca senzorul din fata detecteaza obstacol in zona de pericol,
        // blocam acceleratia inainte (rawValue > 512 = inainte)
        int throttleCmd = _dateRadio.throttle;
        if (SenzorFata::estePericol() && throttleCmd > 512)
        {
            throttleCmd = 512;  // fortam stop daca obstacol in fata
        }
        // Securitate: daca senzorul din spate detecteaza obstacol, blocam mersul inapoi
        if (SenzorSpate::estePericol() && throttleCmd < 512)
        {
            throttleCmd = 512;  // fortam stop daca obstacol in spate
        }
        Esc::setThrottle(throttleCmd);

        // --- Schimbator de viteze (Gearbox) pe D6 ---
        static bool lastSwLeft = false;
        static bool lastSwRight = false;

        if (_dateRadio.swLeft && !lastSwLeft)
        {
            Gearbox::shiftDown();
        }
        if (_dateRadio.swRight && !lastSwRight)
        {
            Gearbox::shiftUp();
        }
        lastSwLeft = _dateRadio.swLeft;
        lastSwRight = _dateRadio.swRight;

        Serial.print("[Radio] Pachet primit | THR: ");
        Serial.print(_dateRadio.throttle);
        Serial.print(" | STR: ");
        Serial.print(_dateRadio.steering);
        Serial.print(" | BUZ: ");
        Serial.print(_dateRadio.buzz);
        Serial.print(" | SWL: ");
        Serial.print(_dateRadio.swLeft);
        Serial.print(" | SWR: ");
        Serial.print(_dateRadio.swRight);
        Serial.print(" | TREAPTA: ");
        Serial.println(Gearbox::getGear());
    }
    else if (_rxActive && acum - _ultimaRx > RX_TIMEOUT)
    {
        _rxActive = false;
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("[Radio] Conexiune pierduta! Motor oprit - siguranta.");

        // Siguranta: oprim motorul la pierderea semnalului radio
        Esc::stop();
        Steering::setAngle(512);  // directie centru
    }

    if (acum - _ultimaCitire >= INTERVAL_SENZORI)
    {
        _ultimaCitire = acum;

        // --- Senzori obstacole ---
        distFata = SenzorFata::citeste();
        distSpate = SenzorSpate::citeste();
        SenzorLumina::citeste();

        Serial.print("FATA: ");
        Serial.print(distFata, 1);
        Serial.print(" cm  |  SPATE: ");
        Serial.print(distSpate, 1);
        Serial.print(" cm  |  LUMINA: ");
        Serial.println(SenzorLumina::citeste());

        // --- Far fata: aprindere automata dupa lumina ---
        if (SenzorLumina::esteIntuneric())
        {
            Leduri::farFataOn();
        }
        else
        {
            Leduri::farFataOff();
        }

        // --- Stop spate ---
        // Prioritate: frana > lumini > stins
        if (SenzorSpate::estePericol())
        {
            Leduri::stopFrana(); // obstacol spate -> frana automata
        }
        else if (SenzorLumina::esteIntuneric())
        {
            Leduri::stopNormal(); // lumini aprinse -> stop slab (pozitie)
        }
        else
        {
            Leduri::stopOff(); // zi + fara frana -> stins
        }
    }

    // --- PWM software Stop Spate (apelat cat mai des in loop) ---
    Leduri::update();

    // --- Buzzer non-blocking (activat de obstacole sau manual din butonul de pe telecomanda) ---
    bool pericol = SenzorFata::estePericol() || SenzorSpate::estePericol();
    bool atentie = SenzorFata::esteAtentie() || SenzorSpate::esteAtentie();
    Buzzer::update(pericol || _dateRadio.buzz, atentie);
}
