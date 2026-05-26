#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

void UART_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    
    UCSR0B = (1 << TXEN0);
    
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void UART_printString(const char* str) {
    while (*str) {
        UART_transmit(*str++);
    }
}

void UART_printInt(uint16_t num) {
    char buffer[7];
    itoa(num, buffer, 10);
    UART_printString(buffer);
}

void ADC_init() {
    ADMUX = (1 << REFS0);
    
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_read() {
    ADCSRA |= (1 << ADSC);
    
    while (ADCSRA & (1 << ADSC));
    
    return ADC;
}

int main(void) {
    UART_init(103);
    ADC_init();
    
    UART_printString("--- ATmega328P Bare-Metal Joystick Test ---\r\n");
    
    while (1) {
        uint16_t xValue = ADC_read();
        
        UART_printString("Raw ADC Value (A0): ");
        UART_printInt(xValue);
        
        if (xValue < 400) {
            UART_printString(" [ LEFT ]");
        } else if (xValue > 600) {
            UART_printString(" [ RIGHT ]");
        } else {
            UART_printString(" [ CENTER ]");
        }
        
        UART_printString("\r\n");
        
        _delay_ms(250);
    }
}