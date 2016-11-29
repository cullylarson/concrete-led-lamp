#include <avr/io.h>

#ifndef PINS_H
#define PINS_H

#define LED_R       PB2
#define LED_R_PORT  PORTB
#define LED_R_DDR   DDRB

#define LED_G       PA7
#define LED_G_PORT  PORTA
#define LED_G_DDR   DDRA

#define LED_B       PA6
#define LED_B_PORT  PORTA
#define LED_B_DDR   DDRA

// write digital "high" to pin <pn> on port <port>
#define GOHI(port, pn) port |= (1<<pn)

// write digital "low" to pin <pn> on port <port>
#define GOLO(port, pn) port &= ~(1<<pn)

#define TOGGLE(port, pn) port ^= (1<<pn)

// if that bit is zero, the switch is being pressed. if it's 1, not being pressed
#define READ(port, pn) (port & (1<<pn)) == 0

void setupPins();

#endif
