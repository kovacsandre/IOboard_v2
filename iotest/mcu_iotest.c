#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <avr/interrupt.h>

#define DIAG PB1
#define WR   PB1
#define CSH  PB2
#define DIS  PB3

#define CSI  PD5
#define RD   PD6

uint8_t i;
uint8_t bits = 0x00;
volatile uint8_t ready = 0;

ISR(PCINT0_vect) {
    ready = 1;
}

void
USART0_Init(void)
{
    /* Set baud rate Asynchronous Normal mode
     * defined in setbaud.h you can change baudrate in Makefile
     */
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    /* Set frame format: 8data, no parity & 1 stop bits */
    UCSR0C = /*(0<<UMSEL0) | (0<<UPM00) | (0<<USBS0) |*/ (1<<UCSZ01) | (1<<UCSZ00);
    /* Enable receiver and transmitter */
    UCSR0B = (1<<RXEN0) | (1<<TXEN0);
}

void
USART0_Transmit(uint8_t data)
{
    /* Wait for empty transmit buffer */
    while (!(UCSR0A & (1<<UDRE0)));
    /* Put data into buffer, sends the data */
    UDR0 = data;
}
void
write_outputs(void)
{
    PORTB &= ~(1 << CSH | 1 << WR);
    _delay_ms(1);
    PORTB |= (1 << CSH | 1 << WR);
}

int
main(void)
{
    DDRB |= (1 << WR | 1 << CSH | 1 << DIS);
    DDRC |= 0xFF;
    PORTC = 0x00;
    PORTB |= (1 << DIS);
    PORTB |= (1 << CSH | 1 << WR);

    DDRD |= (1 << RD | 1 << CSI);
    DDRA = 0x00;
    PCMSK0 = 0xFF;
    PCICR |= (1 << PCIE0);
    USART0_Init();
    sei();

    while(1) {
        for (i=0; i<8; i++) {
            PORTC = (1 << i);
            write_outputs();
            if (ready) {
                bits = PINA;
                USART0_Transmit(bits);
                ready = 0;
            }
            _delay_ms(1000);
        }

    }
    return 0;
}
