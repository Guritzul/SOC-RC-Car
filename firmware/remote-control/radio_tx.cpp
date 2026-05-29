#include "radio_tx.h"
#include "hal.h"
#include <Arduino.h>

#define NRF_REG_CONFIG     0x00
#define NRF_REG_EN_AA      0x01
#define NRF_REG_EN_RXADDR  0x02
#define NRF_REG_SETUP_AW   0x03
#define NRF_REG_SETUP_RETR 0x04
#define NRF_REG_RF_CH      0x05
#define NRF_REG_RF_SETUP   0x06
#define NRF_REG_STATUS     0x07
#define NRF_REG_RX_ADDR_P0 0x0A
#define NRF_REG_TX_ADDR    0x10
#define NRF_REG_RX_PW_P0   0x11
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_DYNPD      0x1C
#define NRF_REG_FEATURE    0x1D

#define NRF_CMD_R_REGISTER   0x00
#define NRF_CMD_W_REGISTER   0x20
#define NRF_CMD_R_RX_PAYLOAD 0x61
#define NRF_CMD_W_TX_PAYLOAD 0xA0
#define NRF_CMD_FLUSH_TX     0xE1
#define NRF_CMD_FLUSH_RX     0xE2

class INrf24Tx
{
public:
    virtual ~INrf24Tx() {}
    virtual void init() = 0;
    virtual void setTxMode(const uint8_t *txAddress) = 0;
    virtual bool sendPayload(const void *buf, uint8_t size) = 0;
    virtual bool performSelfTest() = 0;
    virtual uint8_t readReg(uint8_t reg) = 0;
};

class Nrf24RegisterTxDriver : public INrf24Tx
{
private:
    ISpi  &_spi;
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

    void flushTx()
    {
        _spi.select();
        _spi.transfer(NRF_CMD_FLUSH_TX);
        _spi.deselect();
    }

    void flushRx()
    {
        _spi.select();
        _spi.transfer(NRF_CMD_FLUSH_RX);
        _spi.deselect();
    }

public:
    Nrf24RegisterTxDriver(ISpi &spi, IGpio &ce)
        : _spi(spi), _ce(ce)
    {
    }

    void init() override
    {
        // Setăm preventiv pinii CE și CSN în stările corecte înainte de orice delay de SPI
        _ce.initOutput();
        _ce.writeLow();

        _spi.init();

        // Soft-reset pe SPI trimițând un NOP (ajută la resincronizarea modulelor blocate)
        _spi.select();
        _spi.transfer(0xFF);
        _spi.deselect();
        delay(2);

        // Forțăm modulul în Power Down pentru a curăța erorile de latch-up
        writeRegister(NRF_REG_CONFIG, 0x00);
        delay(15);

        writeRegister(NRF_REG_STATUS, 0x70);

        flushTx();
        flushRx();
    }

    void setTxMode(const uint8_t *txAddress) override
    {
        _ce.writeLow();

        writeRegister(NRF_REG_EN_AA, 0x01);
        writeRegister(NRF_REG_EN_RXADDR, 0x01);
        writeRegister(NRF_REG_SETUP_AW, 0x03);

        // Delay retransmitere: 500us (retry delay must be > payload_size * 8 / data_rate), 15 încercări
        writeRegister(NRF_REG_SETUP_RETR, 0x1F);

        // Canalul RF la 115 (2.515 GHz - în afara benzii Wi-Fi pentru zero interferențe)
        writeRegister(NRF_REG_RF_CH, 115);

        // RF_SETUP: 1 Mbps, -18dBm (puterea scăzută previne brownout la emisie fără condensator de filtrare)
        writeRegister(NRF_REG_RF_SETUP, 0x00);

        writeRegisterBuf(NRF_REG_TX_ADDR, txAddress, 5);
        writeRegisterBuf(NRF_REG_RX_ADDR_P0, txAddress, 5);
        writeRegister(NRF_REG_RX_PW_P0, sizeof(Payload));

        writeRegister(NRF_REG_DYNPD, 0x00);
        writeRegister(NRF_REG_FEATURE, 0x00);

        // CONFIG: EN_CRC = 1, CRCO = 1, PWR_UP = 1, PRIM_RX = 0
        writeRegister(NRF_REG_CONFIG, 0x0E);

        writeRegister(NRF_REG_STATUS, 0x70);

        // PowerDown -> Standby-I (safe timing = 15ms pentru clone)
        delay(15);
    }

    bool sendPayload(const void *buf, uint8_t size) override
    {
        writeRegister(NRF_REG_STATUS, 0x70);

        const uint8_t *p = (const uint8_t *)buf;
        _spi.select();
        _spi.transfer(NRF_CMD_W_TX_PAYLOAD);
        for (uint8_t i = 0; i < size; ++i)
        {
            _spi.transfer(p[i]);
        }
        _spi.deselect();

        // Puls CE de minim 10us pentru a declanșa transmisia
        _ce.writeHigh();
        delayMicroseconds(15);
        _ce.writeLow();

        // Polling în STATUS până când TX_DS (succes) sau MAX_RT (limită încercări) se setează (timeout 100ms)
        uint8_t status = 0;
        uint32_t startTime = millis();
        while (true)
        {
            status = readRegister(NRF_REG_STATUS);
            if (status & ((1 << 5) | (1 << 4)))
            {
                break;
            }
            if (millis() - startTime > 100)
            {
                status = 0x10;
                break;
            }
        }

        writeRegister(NRF_REG_STATUS, status & 0x70);

        if (status & (1 << 4))
        {
            flushTx();
            return false;
        }

        return true;
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

// CSN: PB0 (D8) - gestionat extern prin SPI Master
static Atm328Gpio csnPin(&DDRB, &PORTB, &PINB, PORTB0);
static Atm328Spi spiDriver(csnPin);
static Atm328Gpio cePin(&DDRD, &PORTD, &PIND, PORTD7); // CE: PD7 (D7)

static Nrf24RegisterTxDriver nrf24(spiDriver, cePin);

static const uint8_t txAddress[5] = {0x30, 0x30, 0x30, 0x30, 0x31}; // "00001"

static const uint8_t MAX_REINIT_ATTEMPTS = 3;
static const uint32_t REINIT_COOLDOWN_MS = 50;

namespace RadioTx
{
    void init()
    {
        nrf24.init();
        nrf24.setTxMode(txAddress);

        bool spiOk = nrf24.performSelfTest();
        if (spiOk)
        {
            Serial.println("[RadioTx] SPI OK - modulul nRF24L01 raspunde.");
        }
        else
        {
            Serial.println("[RadioTx] EROARE SPI! nRF24L01 nu raspunde.");
        }

        uint8_t rfSetup = nrf24.readReg(NRF_REG_RF_SETUP);
        uint8_t rfCh    = nrf24.readReg(NRF_REG_RF_CH);
        uint8_t config  = nrf24.readReg(NRF_REG_CONFIG);
        uint8_t status  = nrf24.readReg(NRF_REG_STATUS);

        Serial.print("[RadioTx] RF_SETUP=0x");
        Serial.print(rfSetup, HEX);
        Serial.print(" RF_CH=");
        Serial.print(rfCh);
        Serial.print(" CONFIG=0x");
        Serial.print(config, HEX);
        Serial.print(" STATUS=0x");
        Serial.println(status, HEX);
        Serial.println("[RadioTx] Asteptat: RF_SETUP=0x06  RF_CH=115  CONFIG=0x0E  STATUS=0x0E");

        Serial.print("[RadioTx] sizeof(Payload) = ");
        Serial.print(sizeof(Payload));
        Serial.println(" bytes");

        Serial.println("[RadioTx] Gata de transmisie pe adresa '00001'.");
    }

    bool send(const Payload &data)
    {
        if (nrf24.sendPayload(&data, sizeof(Payload)))
        {
            return true;
        }

        // --- Recuperare la eroare de transmisie ---
        // Dacă transmisia eșuează (MAX_RT / brownout), reinițializăm modulul complet.
        for (uint8_t attempt = 0; attempt < MAX_REINIT_ATTEMPTS; ++attempt)
        {
            Serial.print("[RadioTx] Transmisie esuata! Reinitializare nRF24L01 (incercare ");
            Serial.print(attempt + 1);
            Serial.print("/");
            Serial.print(MAX_REINIT_ATTEMPTS);
            Serial.println(")...");

            delay(REINIT_COOLDOWN_MS);

            nrf24.init();
            nrf24.setTxMode(txAddress);

            if (!nrf24.performSelfTest())
            {
                continue;
            }

            if (nrf24.sendPayload(&data, sizeof(Payload)))
            {
                Serial.println("[RadioTx] Comunicare restabilita dupa reinitializare.");
                return true;
            }
        }

        Serial.println("[RadioTx] EROARE CRITICA: Transmisia a esuat dupa toate reinitializarile.");
        return false;
    }
}
