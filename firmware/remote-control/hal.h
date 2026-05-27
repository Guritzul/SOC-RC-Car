#pragma once

#include <stdint.h>
#include <avr/io.h>

// ============================================================
//  HAL Abstraction Interfaces (SOLID - ISP & DIP)
// ============================================================

// Interface for general-purpose input/output (GPIO) pin control
class IGpio
{
public:
    virtual ~IGpio() {}
    virtual void initOutput() = 0;
    virtual void initInputPullup() = 0;
    virtual void writeHigh() = 0;
    virtual void writeLow() = 0;
    virtual bool read() = 0;
};

// Interface for hardware SPI communication
class ISpi
{
public:
    virtual ~ISpi() {}
    virtual void init() = 0;
    virtual void select() = 0;
    virtual void deselect() = 0;
    virtual uint8_t transfer(uint8_t data) = 0;
};

// Interface for Analog-to-Digital Converter (ADC) readings
class IAdc
{
public:
    virtual ~IAdc() {}
    virtual void init() = 0;
    virtual uint16_t readChannel(uint8_t channel) = 0;
};

// ============================================================
//  ATmega328P Register-Level Concrete Implementations (SOLID - LSP)
// ============================================================

// Concrete GPIO driver manipulating DDRx, PORTx, and PINx registers directly
class Atm328Gpio : public IGpio
{
private:
    volatile uint8_t *_ddr;
    volatile uint8_t *_port;
    volatile uint8_t *_pin;
    uint8_t _pinMask;

public:
    Atm328Gpio(volatile uint8_t *ddr, volatile uint8_t *port, volatile uint8_t *pin, uint8_t pinBit)
        : _ddr(ddr), _port(port), _pin(pin), _pinMask(1 << pinBit)
    {
    }

    void initOutput() override
    {
        *_ddr |= _pinMask;
    }

    void initInputPullup() override
    {
        *_ddr &= ~_pinMask;
        *_port |= _pinMask;
    }

    void writeHigh() override
    {
        *_port |= _pinMask;
    }

    void writeLow() override
    {
        *_port &= ~_pinMask;
    }

    bool read() override
    {
        return (*_pin & _pinMask) != 0;
    }
};

// Concrete SPI driver utilizing ATmega328P hardware SPI peripheral registers
// Fixed hardware pins: SS/CSN (PB2), MOSI (PB3), MISO (PB4), SCK (PB5)
class Atm328Spi : public ISpi
{
public:
    void init() override
    {
        // 1. Set SS (PB2), MOSI (PB3), SCK (PB5) as OUTPUT
        DDRB |= (1 << DDB2) | (1 << DDB3) | (1 << DDB5);

        // 2. Set MISO (PB4) as INPUT
        DDRB &= ~(1 << DDB4);

        // 3. Ensure SS/CSN starts HIGH (deselect)
        PORTB |= (1 << PORTB2);

        // 4. Configure SPI Control Register (SPCR):
        //    - SPE = 1 (SPI Enable)
        //    - MSTR = 1 (Master mode)
        //    - SPI Speed: F_CPU / 4 = 4 MHz (on 16 MHz ATmega328P, SPR1/0 = 00, SPI2X in SPSR = 0)
        //    - Mode 0: CPOL = 0, CPHA = 0 (Required by nRF24L01)
        SPCR = (1 << SPE) | (1 << MSTR);
        SPSR &= ~(1 << SPI2X);
    }

    void select() override
    {
        // Drive CSN (PB2) LOW (active)
        PORTB &= ~(1 << PORTB2);
    }

    void deselect() override
    {
        // Drive CSN (PB2) HIGH (inactive)
        PORTB |= (1 << PORTB2);
    }

    uint8_t transfer(uint8_t data) override
    {
        // Load data into standard SPI Data Register (SPDR)
        SPDR = data;

        // Wait until transmission completes (poll SPI Interrupt Flag - SPIF)
        while (!(SPSR & (1 << SPIF)))
        {
            // Busy wait loop
        }

        // Return received byte
        return SPDR;
    }
};

// Concrete ADC driver utilizing ATmega328P internal ADC peripheral registers
class Atm328Adc : public IAdc
{
public:
    void init() override
    {
        // Enable ADC (ADEN = 1) and set prescaler to 128 (ADPS2:0 = 111)
        // 16 MHz / 128 = 125 kHz ADC clock (ideal speed for 10-bit resolution)
        ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    }

    uint16_t readChannel(uint8_t channel) override
    {
        // Clear existing MUX channel bits (bits 3:0 of ADMUX) and set new channel
        ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

        // Use AVcc as reference (REFS0 = 1, REFS1 = 0)
        ADMUX |= (1 << REFS0);
        ADMUX &= ~(1 << REFS1);

        // Start Conversion (ADSC = 1)
        ADCSRA |= (1 << ADSC);

        // Wait for conversion to finish (ADSC is cleared automatically by hardware)
        while (ADCSRA & (1 << ADSC))
        {
            // Busy wait
        }

        // Return 10-bit conversion result from ADC data register
        return ADC;
    }
};
