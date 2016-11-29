#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "pins.h"

// TODO -- could just use one timer, and use its second comparator (e.g. OCR1B)

// this will be a 10-bit value. since we need to divide those bits among three colors, we'll
// just ignore the least significant bit. this means the color will essentially change "every
// other bit", which should even be noticable. this gives us 3 bits per color.
volatile uint16_t _dialValue = 0;
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

void onDialUpdate() {
    _red   = (_dialValue & 0b0000000000001110) >> 1; // red is bits 1-3
    _green = (_dialValue & 0b0000000001110000) >> 4; // green is bits 4-6
    _blue  = (_dialValue & 0b0000001110000000) >> 7; // blue is bits 7-9
}

void setup() {
    setupPins();
    setupAdc();
    setupLedPwm();

    sei();

    startAdcConversion();
}

void setupAdc() {
    // Vcc used as analog reference
    ADMUX &= ~((1 << REFS1) | (1 << REFS0));
    // Read from ADC3 (pin 10)
    ADMUX |= (1 << MUX1) | (1 << MUX2);

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
    _dialValue = (ADCH << 8) | ADCL; // ADCH will have the left-most 2 bits, ADCL will have the right-most 8 bits (i.e. you get a 10-bit value)
    onDialUpdate();
}

// timers that control PWM for the LEDs
// 
// timer 1 will count the full cycle of our pulse. timer 0 counts divisions
// of that cycle and switches each chanel (red, green, blue) on/off, depending
// on the duty cycle.
//
// each color is stored as a 3-bit value, which gives 8 diff value. so, the
// division timer (timer 0), needs to bit 1/8 of the full cycle.
void setupLedPwm() {
    // Timer/Counter 1 -- used to count the full cycle of the pulse

    // CTC (Clear Timer on Compare)
    TCCR1B |= (1 << WGM12);

    // Prescale 8 (needs to be the same prescaler as timer 0)
    TCCR1B |= (1 << CS11); // TODO -- tweak this (would tweak in order to get a precise voltage range, for the LED driver)

    // count 11 bits
    OCR1A = 0b11111111111;

    // enable compare A match interrupt
    TIMSK1 |= (1 << OCIE1A);

    // Timer/Counter 0 -- used to count divisions of the full cycle

    // CTC (Clear Timer on Compare)
    TCCR0B |= (1 << WGM01);

    // Prescale 8 (needs to be the same prescaler as timer 1)
    TCCR0B |= (1 << CS01);

    // counts 8 bits (which is 1/8 of the full cycle)
    OCR0A = 0xff;

    // enable compare A match interrupt
    TIMSK0 |= (1 << OCIE0A);
}

/**
 * @param displayColorValue uint8_t The value of the color to be displayed (e.g. _red, _greeen, _blue)
 * @param colorCounterValue uint8_t How far we've counted (assumes a maximum count of 11 bits)
 * @return uint8_t 1 if pulse is on, 0 if off.  determines this
 */
uint8_t isPulseOn(uint8_t displayColorValue, uint16_t counterValue) {
    // a value of zero takes up 0 - 255 on the counter
    if(displayColorValue == 0) return 0;
    else if(displayColorValue == 1 && counterValue < 512) return 1;
    else if(displayColorValue == 2 && counterValue < 768) return 1;
    else if(displayColorValue == 3 && counterValue < 1024) return 1;
    else if(displayColorValue == 4 && counterValue < 1280) return 1;
    else if(displayColorValue == 5 && counterValue < 1536) return 1;
    else if(displayColorValue == 6 && counterValue < 1792) return 1;
    else if(displayColorValue == 7) return 1;

    return 0;
}

// triggered at the beginning of a new cycle
ISR(TIM1_COMPA) {
    // don't need to do anything, since duty cycle is handled in the timer 0 interrupt
}

// triggered at the beginning of each division of the cycle
ISR(TIM0_COMPA) {
    // the count of timer 1 (the full cycle counter) is in TCNT1

    if(isPulseOn(_red, TCNT1)) GOHI(LED_R_PORT, LED_R);
    else GOLO(LED_R_PORT, LED_R);

    if(isPulseOn(_green, TCNT1)) GOHI(LED_G_PORT, LED_G);
    else GOLO(LED_G_PORT, LED_G);

    if(isPulseOn(_blue, TCNT1)) GOHI(LED_B_PORT, LED_B);
    else GOLO(LED_B_PORT, LED_B);
}
