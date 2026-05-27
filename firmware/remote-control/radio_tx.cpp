#include "radio_tx.h"
#include "hal.h"
#include <Arduino.h>

// ============================================================
//  nRF24L01 Register Addresses and Commands
// ============================================================
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

// ============================================================
//  Transmitter Abstract Interface (SOLID - ISP / DIP)
// ============================================================
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

// ============================================================
//  Concrete Register-Level nRF24L01 Driver (SOLID - LSP)
// ============================================================
class Nrf24RegisterTxDriver : public INrf24Tx
{
private:
    ISpi  &_spi;
    IGpio &_ce;

    // Helper: Write a value to an nRF24 register
    void writeRegister(uint8_t reg, uint8_t val)
    {
        _spi.select();
        _spi.transfer(NRF_CMD_W_REGISTER | reg);
        _spi.transfer(val);
        _spi.deselect();
    }

    // Helper: Write a buffer (e.g., address) to an nRF24 register
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

    // Helper: Read a value from an nRF24 register
    uint8_t readRegister(uint8_t reg)
    {
        _spi.select();
        _spi.transfer(NRF_CMD_R_REGISTER | reg);
        uint8_t val = _spi.transfer(0xFF);
        _spi.deselect();
        return val;
    }

    // Helper: Flush the TX hardware FIFO
    void flushTx()
    {
        _spi.select();
        _spi.transfer(NRF_CMD_FLUSH_TX);
        _spi.deselect();
    }

    // Helper: Flush the RX hardware FIFO
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
        _spi.init();
        _ce.initOutput();
        _ce.writeLow();

        // 1. Clear any pending interrupt flags in STATUS
        writeRegister(NRF_REG_STATUS, 0x70);

        // 2. Flush FIFO buffers
        flushTx();
        flushRx();
    }

    void setTxMode(const uint8_t *txAddress) override
    {
        _ce.writeLow();

        // 1. Enable Auto-Acknowledgment on Pipe 0 (required to receive ACK packets)
        writeRegister(NRF_REG_EN_AA, 0x01);

        // 2. Enable RX on Pipe 0 (required to receive ACK packets)
        writeRegister(NRF_REG_EN_RXADDR, 0x01);

        // 3. Set address width to 5 bytes (0x03)
        writeRegister(NRF_REG_SETUP_AW, 0x03);

        // 4. Set Retries: 500us delay (0x1 in high nibble), 15 retries (0xF in low nibble) -> 0x1F
        //    Note: retry delay must be > payload_size * 8 / data_rate to avoid collision with ACK
        writeRegister(NRF_REG_SETUP_RETR, 0x1F);

        // 5. Set RF channel to 115 (2.515 GHz - completely outside Wi-Fi spectrum)
        writeRegister(NRF_REG_RF_CH, 115);

        // 6. Set RF_SETUP: 1 Mbps, -18 dBm PA level
        //    NOTE: 0x00 on nRF24L01+ clones sets 250kbps (different bit layout than original)
        //    Correct 1Mbps value is 0x06: RF_DR=1Mbps, LNA_HCURR=1 (bit 0), PA=-18dBm
        writeRegister(NRF_REG_RF_SETUP, 0x06);

        // 7. Write transmitter address (5 bytes)
        writeRegisterBuf(NRF_REG_TX_ADDR, txAddress, 5);

        // 8. Write receiver address on Pipe 0 (must match TX address for Auto-Ack)
        writeRegisterBuf(NRF_REG_RX_ADDR_P0, txAddress, 5);

        // 9. Configure Pipe 0 payload width (required for Auto-Ack packet validation on Pipe 0)
        writeRegister(NRF_REG_RX_PW_P0, sizeof(Payload));

        // Explicitly disable dynamic payload length and features (required for clone compatibility)
        writeRegister(NRF_REG_DYNPD, 0x00);
        writeRegister(NRF_REG_FEATURE, 0x00);

        // 10. Configure CONFIG register:
        //    - EN_CRC = 1 (CRC enabled)
        //    - CRCO = 1 (2-byte CRC)
        //    - PWR_UP = 1 (Power up transceiver)
        //    - PRIM_RX = 0 (Transmit mode)
        //    Value: 0x0E
        writeRegister(NRF_REG_CONFIG, 0x0E);

        // Clear interrupt flags in STATUS again
        writeRegister(NRF_REG_STATUS, 0x70);

        // Allow nRF24 to transition from PowerDown -> Standby-I (datasheet: min 1.5ms, safe = 5ms)
        delay(5);
    }

    bool sendPayload(const void *buf, uint8_t size) override
    {
        // 1. Clear status flags before starting transmission
        writeRegister(NRF_REG_STATUS, 0x70);

        // 2. Write payload byte-by-byte into nRF24 TX FIFO
        const uint8_t *p = (const uint8_t *)buf;
        _spi.select();
        _spi.transfer(NRF_CMD_W_TX_PAYLOAD);
        for (uint8_t i = 0; i < size; ++i)
        {
            _spi.transfer(p[i]);
        }
        _spi.deselect();

        // 3. Pulse CE pin HIGH for >10us to trigger the transmission sequence
        _ce.writeHigh();
        delayMicroseconds(15);
        _ce.writeLow();

        // 4. Poll STATUS register until either TX_DS (successful ACK) or MAX_RT (retry limit) is set
        uint8_t status = 0;
        uint32_t startTime = millis();
        while (true)
        {
            status = readRegister(NRF_REG_STATUS);
            // Bit 5 is TX_DS, Bit 4 is MAX_RT
            if (status & ((1 << 5) | (1 << 4)))
            {
                break;
            }
            if (millis() - startTime > 100) // 100ms safety timeout
            {
                status = 0x10; // Pretend it failed (MAX_RT) to clear and return
                break;
            }
        }

        // 5. Clear status flags (write 1 to clear)
        writeRegister(NRF_REG_STATUS, status & 0x70);

        // 6. If MAX_RT (transmission failed), flush the TX FIFO
        if (status & (1 << 4))
        {
            flushTx();
            return false;
        }

        return true; // Successfully sent and ACK received
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
//  Static Instance Injection (SOLID - DIP / ISP)
//  Pini nRF24L01 pe telecomanda:
//    MOSI -> D11 (PB3) - hardware SPI
//    MISO -> D12 (PB4) - hardware SPI
//    SCK  -> D13 (PB5) - hardware SPI
//    CE   -> D7  (PD7)
//    CSN  -> D8  (PB0)
//    SS   -> D10 (PB2) - tinut HIGH permanent (nu folosit ca CSN)
// ============================================================

// CSN: PB0 (D8) - bit 0 din Port B
static Atm328Gpio csnPin(&DDRB, &PORTB, &PINB, PORTB0);

// CSN este injectat in SPI; SPI gestioneaza CSN extern, nu SS hardware
static Atm328Spi spiDriver(csnPin);

// CE: PD7 (D7) - bit 7 din Port D
static Atm328Gpio cePin(&DDRD, &PORTD, &PIND, PORTD7);

static Nrf24RegisterTxDriver nrf24(spiDriver, cePin);

// TX address "00001" (matching RX side)
static const uint8_t txAddress[5] = {0x30, 0x30, 0x30, 0x30, 0x31};

// ============================================================
//  Fault Tolerance Configuration
// ============================================================
static const uint8_t MAX_REINIT_ATTEMPTS = 3;   // max incercari de reinitializare
static const uint32_t REINIT_COOLDOWN_MS = 50;  // pauza intre reinitializari (ms)

// ============================================================
//  Public API Namespace Implementation
// ============================================================
namespace RadioTx
{
    void init()
    {
        nrf24.init();
        nrf24.setTxMode(txAddress);

        // Self-test: verify SPI communication by writing and reading back RF channel
        bool spiOk = nrf24.performSelfTest();
        if (spiOk)
        {
            Serial.println("[RadioTx] SPI OK - modulul nRF24L01 raspunde.");
        }
        else
        {
            Serial.println("[RadioTx] EROARE SPI! nRF24L01 nu raspunde. Verificati cablajul SPI si alimentarea 3.3V!");
        }

        // Readback diagnostic
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
        Serial.println("[RadioTx] Pini: MOSI=D11(PB3) MISO=D12(PB4) SCK=D13(PB5) CE=D7(PD7) CSN=D8(PB0)");
    }

    bool send(const Payload &data)
    {
        // Prima incercare normala
        if (nrf24.sendPayload(&data, sizeof(Payload)))
        {
            return true;
        }

        // --- Recuperare la eroare de transmisie ---
        // Daca transmisia esueaza (MAX_RT sau timeout SPI), incercam reinitializarea modulului
        // si repetam transmisia pentru a mentine comunicarea activa.
        for (uint8_t attempt = 0; attempt < MAX_REINIT_ATTEMPTS; ++attempt)
        {
            Serial.print("[RadioTx] Transmisie esuata! Reinitializare nRF24L01 (incercare ");
            Serial.print(attempt + 1);
            Serial.print("/");
            Serial.print(MAX_REINIT_ATTEMPTS);
            Serial.println(")...");

            delay(REINIT_COOLDOWN_MS);

            // Reinitializam modulul radio complet
            nrf24.init();
            nrf24.setTxMode(txAddress);

            // Verificam daca SPI raspunde dupa reinit
            if (!nrf24.performSelfTest())
            {
                Serial.println("[RadioTx] SPI nu raspunde dupa reinit! Continuam fara date radio.");
                continue;
            }

            // Reincercam transmisia
            if (nrf24.sendPayload(&data, sizeof(Payload)))
            {
                Serial.println("[RadioTx] Comunicare restabilita dupa reinitializare.");
                return true;
            }
        }

        // Daca toate incercarile au esuat, logam si continuam (nu blocam)
        Serial.println("[RadioTx] EROARE CRITICA: Transmisia a esuat dupa toate reinitializarile. Continuam fara radio.");
        return false;
    }
}
