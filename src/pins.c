#include <avr/io.h>
#include "pins.h"

void setupPins() {
    // set output pins (high)
    LED_R_DDR |= (1 << LED_R);
    LED_G_DDR |= (1 << LED_G);
    LED_B_DDR |= (1 << LED_B);
    TWELVE_V_SWITCH_DDR |= (1 << TWELVE_V_SWITCH);
}

void turnTwelveVoltsOn() {
    GOHI(TWELVE_V_SWITCH_PORT, TWELVE_V_SWITCH);
}
