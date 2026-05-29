#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

volatile uint8_t gearbox_ticks = 141; // Treapta 2 sweet spot

void UART_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

unsigned char UART_receive(void) {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

void UART_printString(const char* str) {
    while (*str) UART_transmit(*str++);
}

void UART_readBuffer(char* buffer, uint8_t max_len) {
    uint8_t index = 0;
    while (index < max_len - 1) {
        char c = UART_receive();
        if (c == '\r' || c == '\n') {
            if (index == 0) continue;
            break;
        }
        buffer[index++] = c;
        UART_transmit(c);
    }
    buffer[index] = '\0';
    UART_printString("\r\n");
}

void hardware_init(void) {
    DDRB |= (1 << PB1) | (1 << PB2); // D9 (Direcție) și D10 (ESC)
    DDRD |= (1 << PD5);              // D5 (Servo Cutie)

    // Configurare TIMER1 (50Hz Fast PWM pe D9 și D10, Prescaler 8)
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1 = 39999; 

    TIMSK1 |= (1 << TOIE1); // Activare software PWM pe D5

    TCCR2A = 0; TCCR2B = 0; 
    sei();
}

ISR(TIMER1_OVF_vect) {
    PORTD |= (1 << PD5);                
    OCR2A = gearbox_ticks;              
    TCNT2 = 0;                          
    TCCR2B = (1 << CS22) | (1 << CS20); 
    TIMSK2 |= (1 << OCIE2A);            
}

ISR(TIMER2_COMPA_vect) {
    PORTD &= ~(1 << PD5);     
    TCCR2B = 0;               
    TIMSK2 &= ~(1 << OCIE2A); 
}

int main(void) {
    hardware_init();
    UART_init(103); // 9600 baud

    char inputBuffer[16];

    UART_printString("=== SISTEM DE CALIBRARE AVANSAT ===");
    UART_printString("\r\nArmare ESC standard (STOP)...");
    OCR1A = 3000;
    OCR1B = 3000;
    _delay_ms(4000);
    
    UART_printString("\r\nPregatit! Comenzi disponibile:\r\n");
    UART_printString("  '1'     -> Porneste motorul (Viteza Medie)\r\n");
    UART_printString("  '0'     -> Opreste complet motorul\r\n");
    UART_printString("  'numar' -> Seteaza ticks direct pentru servo (Interval nou: 80 - 250)\r\n\r\n");

    while (1) {
        UART_printString("Comanda: ");
        UART_readBuffer(inputBuffer, sizeof(inputBuffer));

        if (strcmp(inputBuffer, "1") == 0) {
            OCR1B = 3200;
            UART_printString(">> MOTOR PORNIT (Viteza de test)\r\n");
        }
        else if (strcmp(inputBuffer, "0") == 0) {
            OCR1B = 3000;
            UART_printString(">> MOTOR OPRIT\r\n");
        }
        else {
            int valoare = atoi(inputBuffer);
            if (valoare >= 0 && valoare <= 250) {
                gearbox_ticks = valoare;
                UART_printString(">> Servo Cutie setat la valoarea: ");
                UART_printString(inputBuffer);
                UART_printString("\r\n");
            } else {
                UART_printString(">> Eroare: Introduceti un numar intre 80 si 250, sau comanda 1/0!\r\n");
            }
        }
        UART_printString("----------------------------------------\r\n");
    }
}