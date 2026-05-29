#pragma once

#include <stdint.h>
#include <avr/io.h>
#include <Arduino.h>

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

class ISpi
{
public:
    virtual ~ISpi() {}
    virtual void init() = 0;
    virtual void select() = 0;
    virtual void deselect() = 0;
    virtual uint8_t transfer(uint8_t data) = 0;
};

class IAdc
{
public:
    virtual ~IAdc() {}
    virtual void init() = 0;
    virtual uint16_t readChannel(uint8_t channel) = 0;
};

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

// SPI hardware pins: MOSI -> PB3 (D11), MISO -> PB4 (D12), SCK -> PB5 (D13), SS -> PB2 (D10)
// SS is kept HIGH to prevent ATmega slave-mode transition.
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

        // Dacă SS coboară în LOW în modul master, perifericul SPI al ATmega se comută în mod Slave.
        DDRB |= (1 << DDB2);
        PORTB |= (1 << PORTB2);

        // Configureaza SPCR: SPE=1, MSTR=1, Speed: F_CPU/16 = 1 MHz (pentru module clone), Mode 0
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

class Atm328Adc : public IAdc
{
public:
    void init() override
    {
        // ADCSRA: ADEN=1, ADPS2:0 = 111 (Prescaler 128 -> 16 MHz / 128 = 125 kHz clock ADC)
        ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    }

    uint16_t readChannel(uint8_t channel) override
    {
        ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

        // AVcc ca referinta
        ADMUX |= (1 << REFS0);
        ADMUX &= ~(1 << REFS1);

        ADCSRA |= (1 << ADSC);

        while (ADCSRA & (1 << ADSC)) {}

        return ADC;
    }
};
