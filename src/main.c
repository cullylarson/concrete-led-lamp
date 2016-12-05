#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "pins.h"

// These are the RGB values of each "step" in the color transition (50 steps total).
// I just hard-coded them since it's easier than doing an algorithm, and this is a pretty smooth color transition
//
// The transitions are:
// o  black -> red
// o  red -> magenta
// o  magenta -> blue
// o  blue -> teal
// o  teal -> green
// o  green -> yellow
// o  yellow -> white
//
// Each transition takes 8 steps, which gives us 56 steps, but since each transition also shares a step with the
// one before, I removed the duplicates (6 duplicates gives a total of 50 steps)
//
// TODO -- maybe put the duplicate steps back in so that we stay longer on those main colors
const uint8_t reds[]   = {0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7};
const uint8_t greens[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
const uint8_t blues[]  = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7};

#define MAX_COLOR_IDX sizeof(reds) - 1

volatile uint8_t _red = 0;
volatile uint8_t _green = 0;
volatile uint8_t _blue = 0;

void setup();
void setupAdc();
void onDialUpdate();
void startAdcConversion();
void setupLedPwm();
uint8_t isPulseOn(uint8_t colorValue, uint16_t counterValue);

int main(void) {
    setup();

    while(1) {
    }
}

void onDialUpdate(uint16_t dialValue) {
    // we should have 10 bits for the dial value (0 - 1023), and we need to get a number between 0 and MAX_COLOR_IDX
    uint8_t stepNumber = (MAX_COLOR_IDX * dialValue) / 1023;

    _red   = reds[stepNumber];
    _green = greens[stepNumber];
    _blue  = blues[stepNumber];
}

void setup() {
    setupPins();
    setupAdc();
    setupLedPwm();

    sei();

    startAdcConversion();
}

void setupAdc() {
    // NOTE: Don't need to set the ADC pin as an input pin, though it doesn't change anything if you do

    // Vcc used as analog reference
    ADMUX &= ~((1 << REFS1) | (1 << REFS0));
    // Read from ADC3 (pin 10)
    ADMUX |= (1 << MUX1) | (1 << MUX0);

    // the adc needs to run sample at 50 - 200 kHz. Assuming the clock speed is 8MHz, will
    // need to scale down by 40. Closest, without sampling faster than 200 MHz is 64
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1);

    // free running mode
    ADCSRB &= ~((1 << ADTS2) | (1 << ADTS1) | (1 << ADTS0));

    // enable adc, enable adc interrupts, enable auto-triggering (since we're using free running mode, this just means conversion will automatically start again after each interrupt)
    ADCSRA |= (1 << ADEN) | (1 << ADIE) | (1 << ADATE);
}

void startAdcConversion() {
    ADCSRA |= (1 << ADSC); // start conversion
}

// triggered when ADC is done
ISR(ADC_vect) {
    uint8_t low = ADCL; // need to read ADCL first (according to the datasheet)
    uint16_t dialValue = (ADCH << 8) | low; // ADCH will have the left-most 2 bits, ADCL will have the right-most 8 bits (i.e. you get a 10-bit value)
    onDialUpdate(dialValue);
}

// timers that control PWM for the LEDs
//
// timer 1, register A will count the full cycle of our pulse.
//
// timer 0, register A counts divisions of that cycle and switches
// each channel (red, green, blue) on/off, depending on the duty cycle.
//
// each color is stored as a 3-bit value, which gives 8 different values. so, the division
// timer (timer 0, register A), needs to be 1/8 of the full cycle.
//
// NOTE: I tried doing this using OCR1A and OCR1B on timer 1, but for some reason, the COMPB
// event always fired at the same time as COMPA; not matter what the alues of OACR1A and OCR1B.
// So, just using both timers.
void setupLedPwm() {
    // Timer/Counter 1 -- used to count the full cycle of the pulse

    // CTC (Clear Timer on Compare). This will clear when the counter matches OCR1A (not OCR1B)
    TCCR1B |= (1 << WGM12);

    // Prescale 8
    TCCR1B |= (1 << CS11); // TODO -- tweak this (would tweak in order to get a precise voltage range, for the LED driver)

    OCR1A = 2047;

    // enable compare A match interrupt
    TIMSK1 |= (1 << OCIE1A);

    // Timer/Counter 0 -- used to count divisions of the full cycle

    // CTC (Clear Timer on Compare). This will clear when the counter matches OCR1A (not OCR1B)
    TCCR0B |= (1 << WGM01);

    // Prescale 8
    TCCR0B |= (1 << CS01); // TODO -- tweak this (would tweak in order to get a precise voltage range, for the LED driver)

    // counts 1/8 of the full cycle (it always needs to be 1/8 of the full cycle)
    OCR0A = 255;

    // enable compare A match interrupt
    TIMSK0 |= (1 << OCIE0A);
}

/**
 * Basically just divides the counter into 8 divisions. The colorValue indicates how many of
 * those divisions it's on for.
 *
 * Need to only output voltages between 0 and 2.5V (that's what the PicoBuck takes), so will need
 * to always leave the last half of the cycle low, and do our duty cycle within the first half. This
 * means that all counter values are dividied by 2
 *
 * @param colorValue uint8_t The value of the color to be displayed (e.g. _red, _greeen, _blue). Should be a number between 0 and 7
 * @param colorCounterValue uint8_t How far we've counted (assumes a maximum count of 2047)
 * @return uint8_t 1 if pulse is on, 0 if off.
 */
uint8_t isPulseOn(uint8_t colorValue, uint16_t counterValue) {
    // the last half of the cycle will always be low
    if(counterValue >= 1024) return 0;

    // a value of zero takes up 0 - 127 (256 / 2 - 1) on the counter
    if(colorValue == 0) return 0;
    else if(colorValue == 1 && counterValue < 256) return 1; // 512 / 2 = 256
    else if(colorValue == 2 && counterValue < 384) return 1; // 768 / 2 = 384
    else if(colorValue == 3 && counterValue < 512) return 1; // 1024 / 2 = 512
    else if(colorValue == 4 && counterValue < 640) return 1; // 1280 / 2 = 640
    else if(colorValue == 5 && counterValue < 768) return 1; // 1536 / 2 = 768
    else if(colorValue == 6 && counterValue < 896) return 1; // 1792 / 2 = 896
    else if(colorValue == 7) return 1;

    return 0;
}

// triggered at the beginning of a new cycle
ISR(TIM1_COMPA_vect) {
    // don't need to do anything, since duty cycle is handled in the timer 0 interrupt
}

// triggered at the beginning of each division of the cycle
ISR(TIM0_COMPA_vect) {
    // the count of timer 1 (the full cycle counter) is in TCNT1
    uint16_t counter = TCNT1; // store it, so that it's the same for all of the function calls below

    // TODO -- how long does each of these calls take? if too long, then the blue step won't get its correct duty
    // I think we're good since the delay is at least consistent (i.e. we delayed turning on, and we'll also delay
    // turning off)

    if(isPulseOn(_red, counter)) GOHI(LED_R_PORT, LED_R);
    else GOLO(LED_R_PORT, LED_R);

    if(isPulseOn(_green, counter)) GOHI(LED_G_PORT, LED_G);
    else GOLO(LED_G_PORT, LED_G);

    if(isPulseOn(_blue, counter)) GOHI(LED_B_PORT, LED_B);
    else GOLO(LED_B_PORT, LED_B);
}
