#include "radio_rx.h"
#include <Arduino.h>

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

class ISpi
{
public:
    virtual ~ISpi() {}
    virtual void init() = 0;
    virtual void select() = 0;
    virtual void deselect() = 0;
    virtual uint8_t transfer(uint8_t data) = 0;
};

class IGpio
{
public:
    virtual ~IGpio() {}
    virtual void initOutput() = 0;
    virtual void writeHigh() = 0;
    virtual void writeLow() = 0;
};

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

// SPI hardware pins: MOSI (PB3/D11), MISO (PB4/D12), SCK (PB5/D13)
// SS hardware (PB2/D10) is kept permanent HIGH to release OC1B for ESC
class Atm328Spi : public ISpi
{
private:
    IGpio &_csn;

public:
    explicit Atm328Spi(IGpio &csn) : _csn(csn) {}

    void init() override
    {
        // Asteapta stabilizarea alimentarii nRF24L01 (minim 10.3ms conform datasheet)
        delay(15);

        DDRB |= (1 << DDB3) | (1 << DDB5);
        DDRB &= ~(1 << DDB4);

        // SS hardware (PB2) trebuie ținut HIGH. Dacă ar coborî în LOW, SPI-ul ar trece în mod Slave.
        // PB2/OC1B este acum folosit de ESC, dar setăm DDRB2 și PORTB2 HIGH pentru inițializare sigură.
        DDRB |= (1 << DDB2);
        PORTB |= (1 << PORTB2);

        // Configureaza SPI Master (Mode 0, F_CPU/16 = 1 MHz pentru clone)
        SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
        SPSR &= ~(1 << SPI2X);

        _csn.initOutput();
        _csn.writeHigh();
    }

    void select() override
    {
        _csn.writeLow();
    }

    void deselect() override
    {
        _csn.writeHigh();
    }

    uint8_t transfer(uint8_t data) override
    {
        SPDR = data;
        while (!(SPSR & (1 << SPIF))) {}
        return SPDR;
    }
};

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

class Nrf24RegisterDriver : public INrf24
{
private:
    ISpi &_spi;
    IGpio &_ce;

    void writeRegister(uint8_t reg, uint8_t val)
    {
        _spi.select();
        _spi.transfer(NRF_CMD_W_REGISTER | reg);
        _spi.transfer(val);
        _spi.deselect();
    }

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

    uint8_t readRegister(uint8_t reg)
    {
        _spi.select();
        _spi.transfer(NRF_CMD_R_REGISTER | reg);
        uint8_t val = _spi.transfer(0xFF);
        _spi.deselect();
        return val;
    }

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

        writeRegister(NRF_REG_STATUS, 0x70);

        _spi.select();
        _spi.transfer(NRF_CMD_FLUSH_TX);
        _spi.deselect();

        flushRx();
    }

    void setRxMode(const uint8_t *rxAddress, uint8_t payloadSize) override
    {
        _ce.writeLow();

        writeRegister(NRF_REG_EN_AA, 0x01);
        writeRegister(NRF_REG_EN_RXADDR, 0x01);
        writeRegister(NRF_REG_SETUP_AW, 0x03);

        // Canalul RF la 115 (2.515 GHz - în afara benzii Wi-Fi pentru zero interferențe)
        writeRegister(NRF_REG_RF_CH, 115);

        // RF_SETUP: 1 Mbps, -18dBm (putere redusă pentru a evita brownout)
        writeRegister(NRF_REG_RF_SETUP, 0x00);

        writeRegisterBuf(NRF_REG_RX_ADDR_P0, rxAddress, 5);
        writeRegister(NRF_REG_RX_PW_P0, payloadSize);

        writeRegister(NRF_REG_DYNPD, 0x00);
        writeRegister(NRF_REG_FEATURE, 0x00);

        // CONFIG: EN_CRC = 1, CRCO = 1, PWR_UP = 1, PRIM_RX = 1
        writeRegister(NRF_REG_CONFIG, 0x0F);

        // Așteaptă tranziția PowerDown -> Standby-I (min 1.5ms)
        delay(5);

        writeRegister(NRF_REG_STATUS, 0x70);
        flushRx();

        _ce.writeHigh();

        // Așteaptă stabilizarea receptorului (Standby-I -> RX Mode: min 130us)
        delayMicroseconds(200);
    }

    bool isDataAvailable() override
    {
        uint8_t status = readRegister(NRF_REG_STATUS);
        if (status & (1 << 6))
        {
            return true;
        }

        uint8_t fifoStatus = readRegister(NRF_REG_FIFO_STATUS);
        if (!(fifoStatus & 0x01)) // Bit 0 is RX_EMPTY
        {
            return true;
        }

        return false;
    }

    void readPayload(void *buf, uint8_t size) override
    {
        uint8_t *p = (uint8_t *)buf;
        _spi.select();
        _spi.transfer(NRF_CMD_R_RX_PAYLOAD);
        for (uint8_t i = 0; i < size; ++i)
        {
            p[i] = _spi.transfer(0xFF);
        }
        _spi.deselect();

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

// CSN: PB0 (D8) - mutat de pe D10 (PB2) pentru a elibera OC1B pentru ESC
static Atm328Gpio csnPin(&DDRB, &PORTB, PORTB0);
static Atm328Spi spiDriver(csnPin);
static Atm328Gpio cePin(&DDRD, &PORTD, PORTD7); // CE: D7 (PD7)

static Nrf24RegisterDriver nrf24(spiDriver, cePin);

static const uint8_t rxAddress[5] = {0x30, 0x30, 0x30, 0x30, 0x31}; // "00001"

namespace RadioRx
{
    void init()
    {
        nrf24.init();

        // Self-test SPI înainte de setRxMode (stare Standby-I)
        if (nrf24.performSelfTest())
        {
            Serial.println("[RadioRx] SPI OK - modulul nRF24L01 raspunde pe masina.");
        }
        else
        {
            Serial.println("[RadioRx] EROARE SPI! nRF24L01 nu raspunde.");
        }

        nrf24.setRxMode(rxAddress, sizeof(Payload));

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
    }

    bool receive(Payload &outPayload)
    {
        if (nrf24.isDataAvailable())
        {
            nrf24.readPayload(&outPayload, sizeof(Payload));
            return true;
        }
        return false;
    }
}
