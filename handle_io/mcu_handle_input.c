#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <avr/interrupt.h>

#define DIAG PB0
#define WR   PB1
#define CSH  PB2
#define DIS  PB3

#define CSI  PD5
#define RD   PD6

#define di1 PINA0
#define di2 PINA1
#define di3 PINA2
#define di4 PINA3
#define di5 PINA4
#define di6 PINA5
#define di7 PINA6
#define di8 PINA7

#define do1 PINC0
#define do2 PINC1
#define do3 PINC2
#define do4 PINC3
#define do5 PINC4
#define do6 PINC5
#define do7 PINC6
#define do8 PINC7

#define RISING  0x01
#define FALLING 0x02
#define CHANGE  0x03

typedef struct node {
    uint8_t mode;
    uint8_t pin;
    void (*func)(void);
} input_t;

uint8_t bits = 0x00, i, oldinputbits = 0x00;
volatile uint16_t tot_overflow;
input_t pins[8];
volatile uint8_t ready = 0;

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
    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
    /* Enable receiver and transmitter */
    UCSR0B = (1<<RXEN0) | (1<<TXEN0);
}

void do1_func(void) { PORTC ^= (1 << do1); }
void do2_func(void) { PORTC ^= (1 << do2); }
void do3_func(void) { PORTC ^= (1 << do3); }
void do4_func(void) { PORTC ^= (1 << do4); }
void do5_func(void) { PORTC ^= (1 << do5); }
void do6_func(void) { PORTC ^= (1 << do6); }
void do7_func(void) { PORTC ^= (1 << do7); }

void
do8_func(void) {
    /* Enable overflow int */
    TIMSK1 |= (1 << TOIE1);
    /* Init ovf counter */
    tot_overflow = 0;

}

ISR(TIMER1_OVF_vect)
{
    tot_overflow++;
    /* 5 sec */
    if (tot_overflow >= 150) {
        PORTC ^= (1 << do8);
        write_outputs();
        tot_overflow = 0;
        /* Disable ovf int */
        TIMSK1 &= ~(1 << TOIE1);
    }
}

void
enterIdleMode(void)
{
    SMCR |= (1 << SE);
    asm volatile("sleep");
}

void
disableIdleMode(void)
{
    SMCR &= ~(1 << SE);
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

    /* Prescaler = 8 (32.768 ms) */
    TCCR1B |= (1 << CS11);
    /* Init counter */
    TCNT1 = 0;

    pins[0].mode = RISING;
    pins[0].pin  = 0x01;
    pins[0].func = do1_func;

    pins[1].mode = RISING;
    pins[1].pin  = 0x02;
    pins[1].func = do2_func;

    pins[2].mode = RISING;
    pins[2].pin  = 0x04;
    pins[2].func = do3_func;

    pins[3].mode = FALLING;
    pins[3].pin  = 0x08;
    pins[3].func = do4_func;

    pins[4].mode = FALLING;
    pins[4].pin  = 0x10;
    pins[4].func = do5_func;

    pins[5].mode = FALLING;
    pins[5].pin  = 0x20;
    pins[5].func = do6_func;

    pins[6].mode = CHANGE;
    pins[6].pin  = 0x40;
    pins[6].func = do7_func;

    pins[7].mode = RISING;
    pins[7].pin  = 0x80;
    pins[7].func = do8_func;

    USART0_Init();
    sei();

    while(1) {
        enterIdleMode();
        disableIdleMode();

        if (ready) {
            uint8_t newportbits = PINA;
            USART0_Transmit(newportbits);
            uint8_t change = newportbits ^ oldinputbits;
            uint8_t rising = change & newportbits;
            uint8_t falling = change & oldinputbits;
            oldinputbits = newportbits;

            for (i = 0; i < 8; i++) {
                if ((pins[i].pin & rising) &&
                    (pins[i].mode == RISING ||
                     pins[i].mode == CHANGE)) {
                    pins[i].func();
                    write_outputs();
                }

                if ((pins[i].pin & falling) &&
                    (pins[i].mode == FALLING ||
                     pins[i].mode == CHANGE)) {
                    pins[i].func();
                    write_outputs();
                }
            }

            ready = 0;
        }
    }

    return 0;
}
