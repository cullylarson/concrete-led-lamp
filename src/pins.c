#include <avr/io.h>
#include "pins.h"

void setupPins() {
    // set output pins (high)
    LED_R_DDR |= (1 << LED_R);
    LED_G_DDR |= (1 << LED_G);
    LED_B_DDR |= (1 << LED_B);
}
