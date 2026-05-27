#include "radio_rx.h"
#include <Arduino.h>

// ============================================================
//  Constante nRF24L01 (Regiștri și Comenzi)
// ============================================================
#define NRF_REG_CONFIG 0x00
#define NRF_REG_EN_AA 0x01
#define NRF_REG_EN_RXADDR 0x02
#define NRF_REG_SETUP_AW 0x03
#define NRF_REG_SETUP_RETR 0x04
#define NRF_REG_RF_CH 0x05
#define NRF_REG_RF_SETUP 0x06
#define NRF_REG_STATUS 0x07
#define NRF_REG_RX_ADDR_P0 0x0A
#define NRF_REG_RX_PW_P0 0x11
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_DYNPD 0x1C
#define NRF_REG_FEATURE 0x1D

#define NRF_CMD_R_REGISTER 0x00
#define NRF_CMD_W_REGISTER 0x20
#define NRF_CMD_R_RX_PAYLOAD 0x61
#define NRF_CMD_W_TX_PAYLOAD 0xA0
#define NRF_CMD_FLUSH_TX 0xE1
#define NRF_CMD_FLUSH_RX 0xE2

// ============================================================
//  Interfețe Abstracte (SOLID - ISP / DIP)
// ============================================================

// Interfață pentru comunicarea SPI
class ISpi
{
public:
    virtual ~ISpi() {}
    virtual void init() = 0;
    virtual void select() = 0;
    virtual void deselect() = 0;
    virtual uint8_t transfer(uint8_t data) = 0;
};

// Interfață pentru controlul pinilor GPIO (ex: CE)
class IGpio
{
public:
    virtual ~IGpio() {}
    virtual void initOutput() = 0;
    virtual void writeHigh() = 0;
    virtual void writeLow() = 0;
};

// Interfață pentru driverul de nRF24L01
class INrf24
{
public:
    virtual ~INrf24() {}
    virtual void init() = 0;
    virtual void setRxMode(const uint8_t *rxAddress, uint8_t payloadSize) = 0;
    virtual bool isDataAvailable() = 0;
    virtual void readPayload(void *buf, uint8_t size) = 0;
    virtual bool performSelfTest() = 0;
    virtual uint8_t readReg(uint8_t reg) = 0;
};

// ============================================================
//  Implementări Concrete pe Registri ATmega328P (SOLID - LSP)
// ============================================================

// Clasa concreta pentru SPI pe registri ATmega328P
// Pini hardware SPI (ficsi de silicon): MOSI (PB3/D11), MISO (PB4/D12), SCK (PB5/D13)
// SS hardware (PB2/D10) - tinut permanent HIGH (elibereaza OC1B pentru ESC)
// CSN nRF24L01 -> PB0 (D8) - gestionat extern prin IGpio injectat
class Atm328Spi : public ISpi
{
private:
    IGpio &_csn;  // CSN extern pe PB0 (D8)

public:
    explicit Atm328Spi(IGpio &csn) : _csn(csn) {}

    void init() override
    {
        // 1. Seteaza MOSI (PB3), SCK (PB5) ca OUTPUT
        DDRB |= (1 << DDB3) | (1 << DDB5);

        // 2. Seteaza MISO (PB4) ca INPUT
        DDRB &= ~(1 << DDB4);

        // 3. SS hardware (PB2/D10) tinut permanent HIGH
        //    Daca SS ar cobori in LOW in modul Master, ATmega ar comuta in mod Slave
        //    si ar opri perifericul SPI. PB2/OC1B este acum folosit de ESC (Timer 1).
        //    SS hardware NU trebuie sa fie controlat de ESC - ESC controleaza doar OCR1B.
        //    Pinul fizic PB2 este output Timer1 OC1B (PWM). PORTB2 e ignorat de Timer.
        //    Totusi setam DDRB2 si PORTB2 HIGH pentru siguranta initializarii SPI.
        DDRB |= (1 << DDB2);
        PORTB |= (1 << PORTB2);  // SS permanent HIGH

        // 4. Configureaza SPI Control Register (SPCR):
        //    - SPE = 1 (SPI Enable)
        //    - MSTR = 1 (Master select)
        //    - CPOL = 0, CPHA = 0 (SPI Mode 0 - cerut de nRF24L01)
        //    - Viteza SPI fosc/4 = 4 MHz (SPR1/0 = 00, SPI2X = 0)
        SPCR = (1 << SPE) | (1 << MSTR);
        SPSR &= ~(1 << SPI2X);

        // 5. Initializeaza CSN extern (PB0/D8) - incepe HIGH (deselectat)
        _csn.initOutput();
        _csn.writeHigh();
    }

    void select() override
    {
        // Pune CSN (PB0/D8) in LOW (activare)
        _csn.writeLow();
    }

    void deselect() override
    {
        // Pune CSN (PB0/D8) in HIGH (dezactivare)
        _csn.writeHigh();
    }

    uint8_t transfer(uint8_t data) override
    {
        // Scrie datele in SPI Data Register
        SPDR = data;

        // Asteapta finalizarea transmisiei (verifica bitul SPIF din SPI Status Register)
        while (!(SPSR & (1 << SPIF)))
        {
            // Bucla de asteptare
        }

        // Returneaza datele receptionate in timpul transferului
        return SPDR;
    }
};

// Clasa concreta pentru GPIO pe registri ATmega328P (suport initOutput + writeHigh/Low)
class Atm328Gpio : public IGpio
{
private:
    volatile uint8_t *_ddr;
    volatile uint8_t *_port;
    uint8_t _pinMask;

public:
    Atm328Gpio(volatile uint8_t *ddr, volatile uint8_t *port, uint8_t pinBit)
        : _ddr(ddr), _port(port), _pinMask(1 << pinBit)
    {
    }

    void initOutput() override
    {
        *_ddr |= _pinMask;
    }

    void writeHigh() override
    {
        *_port |= _pinMask;
    }

    void writeLow() override
    {
        *_port &= ~_pinMask;
    }
};

// ============================================================
//  Implementare Driver nRF24L01 (SOLID - DIP)
// ============================================================
class Nrf24RegisterDriver : public INrf24
{
private:
    ISpi &_spi;
    IGpio &_ce;

    // Helper: scriere registru individual
    void writeRegister(uint8_t reg, uint8_t val)
    {
        _spi.select();
        _spi.transfer(NRF_CMD_W_REGISTER | reg);
        _spi.transfer(val);
        _spi.deselect();
    }

    // Helper: scriere buffer în registru (ex: adrese)
    void writeRegisterBuf(uint8_t reg, const uint8_t *buf, uint8_t len)
    {
        _spi.select();
        _spi.transfer(NRF_CMD_W_REGISTER | reg);
        for (uint8_t i = 0; i < len; ++i)
        {
            _spi.transfer(buf[i]);
        }
        _spi.deselect();
    }

    // Helper: citire registru individual
    uint8_t readRegister(uint8_t reg)
    {
        _spi.select();
        _spi.transfer(NRF_CMD_R_REGISTER | reg);
        uint8_t val = _spi.transfer(0xFF);
        _spi.deselect();
        return val;
    }

    // Helper: golire FIFO RX
    void flushRx()
    {
        _spi.select();
        _spi.transfer(NRF_CMD_FLUSH_RX);
        _spi.deselect();
    }

public:
    Nrf24RegisterDriver(ISpi &spi, IGpio &ce)
        : _spi(spi), _ce(ce)
    {
    }

    void init() override
    {
        _spi.init();
        _ce.initOutput();
        _ce.writeLow();

        // 1. Șterge flag-urile de întrerupere din STATUS (scriem 1 pentru a șterge)
        writeRegister(NRF_REG_STATUS, 0x70);

        // 2. Golește bufferele FIFO
        _spi.select();
        _spi.transfer(NRF_CMD_FLUSH_TX);
        _spi.deselect();

        flushRx();
    }

    void setRxMode(const uint8_t *rxAddress, uint8_t payloadSize) override
    {
        // Punem CE în LOW înainte de a modifica configurațiile
        _ce.writeLow();

        // 1. Activare Auto-Acknowledgment pe Pipe 0 (identic cu TX - necesar pentru ACK automat)
        writeRegister(NRF_REG_EN_AA, 0x01);

        // 2. Activare receptor pe Pipe 0 (trebuie sa fie acelasi pipe pe care TX-ul transmite)
        writeRegister(NRF_REG_EN_RXADDR, 0x01);

        // 3. Setează lungimea adresei la 5 bytes (0x03 în SETUP_AW)
        writeRegister(NRF_REG_SETUP_AW, 0x03);

        // 4. Setează canalul RF la 115 (2.515 GHz - complet în afara benzii Wi-Fi pentru zero interferențe)
        writeRegister(NRF_REG_RF_CH, 115);

        // 5. Configurare RF_SETUP:
        //    - Rate = 1 Mbps (RF_DR_LOW=0, RF_DR_HIGH=0)
        //    - LNA_HCURR = 1 (bit 0) - necesar pe clone nRF24L01+
        //    NOTA: 0x00 pe clone nRF24L01+ activeaza 250kbps (layout de biti diferit fata de original)
        //    Valoarea corecta pentru 1Mbps este 0x06
        writeRegister(NRF_REG_RF_SETUP, 0x06);

        // 6. Configurare adresa de receptie pe Pipe 0 (trebuie sa fie IDENTICA cu TX_ADDR de pe transmitator)
        writeRegisterBuf(NRF_REG_RX_ADDR_P0, rxAddress, 5);

        // 7. Seteaza latimea payload-ului static pentru Pipe 0
        writeRegister(NRF_REG_RX_PW_P0, payloadSize);

        // Dezactivare explicita a lungimii dinamice a payload-ului si a functiilor speciale
        writeRegister(NRF_REG_DYNPD, 0x00);
        writeRegister(NRF_REG_FEATURE, 0x00);

        // 8. Configurare registru CONFIG:
        //    - EN_CRC = 1, CRCO = 1 (CRC activ, 2 bytes - identic cu TX)
        //    - PWR_UP = 1 (Alimentat)
        //    - PRIM_RX = 1 (Mod Receiver)
        //    Valoare: 0x0F
        writeRegister(NRF_REG_CONFIG, 0x0F);

        // Sterge din nou flag-urile din STATUS
        writeRegister(NRF_REG_STATUS, 0x70);

        // Goleste FIFO RX pentru a incepe curat
        flushRx();

        // 9. Ridica CE in HIGH pentru a incepe ascultarea in mod RX activ
        _ce.writeHigh();

        // Asteapta stabilizarea receptorului (Standby-I -> RX Mode: min 130us, marja siguranta 200us)
        delayMicroseconds(200);
    }

    bool isDataAvailable() override
    {
        uint8_t status = readRegister(NRF_REG_STATUS);

        // Verifică flag-ul RX_DR (Data Ready - Bit 6) din STATUS
        if (status & (1 << 6))
        {
            return true;
        }

        // De siguranță, verifică și dacă bufferul FIFO RX nu este gol
        uint8_t fifoStatus = readRegister(NRF_REG_FIFO_STATUS);
        if (!(fifoStatus & 0x01)) // Bit 0 este RX_EMPTY (1 = gol, 0 = conține date)
        {
            return true;
        }

        return false;
    }

    void readPayload(void *buf, uint8_t size) override
    {
        uint8_t *p = (uint8_t *)buf;

        // Citește efectiv datele prin SPI folosind comanda R_RX_PAYLOAD
        _spi.select();
        _spi.transfer(NRF_CMD_R_RX_PAYLOAD);
        for (uint8_t i = 0; i < size; ++i)
        {
            p[i] = _spi.transfer(0xFF);
        }
        _spi.deselect();

        // Șterge flag-ul de întrerupere RX_DR prin scrierea valorii 1 pe poziția bitului 6 din STATUS
        writeRegister(NRF_REG_STATUS, 0x40);
    }

    bool performSelfTest() override
    {
        writeRegister(NRF_REG_RF_CH, 115);
        uint8_t val = readRegister(NRF_REG_RF_CH);
        return val == 115;
    }

    uint8_t readReg(uint8_t reg) override
    {
        return readRegister(reg);
    }
};

// ============================================================
//  Instantierea Componentelor si Cablarea Statica (SOLID - DIP)
// ============================================================

// CSN: PB0 (D8) - mutat de pe D10 (PB2/OC1B) pentru a elibera OC1B pentru ESC
// SS hardware (PB2/D10) - tinut permanent HIGH de Atm328Spi::init()
static Atm328Gpio csnPin(&DDRB, &PORTB, PORTB0);

// SPI cu CSN extern injectat
static Atm328Spi spiDriver(csnPin);

// CE: D7 = PD7 (bit 7 din Port D)
static Atm328Gpio cePin(&DDRD, &PORTD, PORTD7);

// Injectam dependentele (SPI si CE) in driverul nRF24L01
// Pini SPI hardware: MOSI=D11(PB3), MISO=D12(PB4), SCK=D13(PB5), CSN=D8(PB0), CE=D7(PD7)
static Nrf24RegisterDriver nrf24(spiDriver, cePin);

// Adresa de recepție "00001" corespunzătoare celei configurate pe transmițător (TX)
static const uint8_t rxAddress[5] = {0x30, 0x30, 0x30, 0x30, 0x31}; // "00001" în ASCII

// ============================================================
//  Namespace Public expus către aplicație
// ============================================================
namespace RadioRx
{
    void init()
    {
        // 1. Initializeaza driverul nRF24L01 (configureaza SPI, pinii de control si FIFO-urile)
        nrf24.init();

        // 2. Self-test SPI INAINTE de setRxMode (CE=LOW = Standby-I = stare sigura pentru registri)
        if (nrf24.performSelfTest())
        {
            Serial.println("[RadioRx] SPI OK - modulul nRF24L01 raspunde pe masina.");
        }
        else
        {
            Serial.println("[RadioRx] EROARE SPI! nRF24L01 nu raspunde. Verificati cablajul SPI si alimentarea 3.3V!");
        }

        // 3. Trece nRF24L01 in mod receptie ascultand pe adresa specificata
        nrf24.setRxMode(rxAddress, sizeof(Payload));

        // 4. Readback diagnostic pentru depanare (CE=HIGH acum, dar citirile sunt ok)
        uint8_t rfSetup = nrf24.readReg(NRF_REG_RF_SETUP);
        uint8_t rfCh = nrf24.readReg(NRF_REG_RF_CH);
        uint8_t config = nrf24.readReg(NRF_REG_CONFIG);
        uint8_t status = nrf24.readReg(NRF_REG_STATUS);

        Serial.print("[RadioRx] RF_SETUP=0x");
        Serial.print(rfSetup, HEX);
        Serial.print(" RF_CH=");
        Serial.print(rfCh);
        Serial.print(" CONFIG=0x");
        Serial.print(config, HEX);
        Serial.print(" STATUS=0x");
        Serial.println(status, HEX);
        Serial.println("[RadioRx] Asteptat: RF_SETUP=0x06  RF_CH=115  CONFIG=0x0F  STATUS=0x0E");

        Serial.print("[RadioRx] sizeof(Payload) = ");
        Serial.print(sizeof(Payload));
        Serial.println(" bytes");

        Serial.println("[RadioRx] Initializat pe registri. Asculta pe adresa '00001'.");
        Serial.println("[RadioRx] Pini: MOSI=D11(PB3) MISO=D12(PB4) SCK=D13(PB5) CE=D7(PD7) CSN=D8(PB0) [D10/OC1B=ESC]");
    }

    bool receive(Payload &outPayload)
    {
        if (nrf24.isDataAvailable())
        {
            // Citim pachetul radio direct în structură
            nrf24.readPayload(&outPayload, sizeof(Payload));
            return true;
        }
        return false;
    }
}
