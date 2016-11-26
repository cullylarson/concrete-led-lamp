#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "pins.h"

void setup();

int main(void) {
    setup();

    while(1) {
    }
}

void setup() {
    setupPins();

    sei();
}
