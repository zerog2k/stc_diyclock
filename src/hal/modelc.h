#ifndef HWCONFIG_H
#define HWCONFIG_H

/*
 * The variant based on MCU stc15f204 and described here:
 * https://github.com/zerog2k/stc_diyclock/issues/20
 */

// alias for relay and buzzer outputs, using relay to drive led for indication of main loop status
#define RELAY
#define BUZZER     P3_3
#define BUZZER_ON  BUZZER = 0
#define BUZZER_OFF BUZZER = 1

// 7-seg led port setup

// which port the segments are connected to
#define LED_SEGMENT_PORT P2
// which port controls the digits
#define LED_DIGITS_PORT  P3

// offset where the digits start on LED_DIGITS_PORT
#define LED_DIGITS_PORT_BASE   4

// setup macro mask to turn off digits
#define LED_DIGITS_OFF()    ( LED_DIGITS_PORT |= (0b1111 << LED_DIGITS_PORT_BASE))
// setup macro to turn on single digit
#define LED_DIGIT_ON(digit) (LED_DIGITS_PORT &= ~((1<<LED_DIGITS_PORT_BASE) << digit))

// adc channels for sensors, P1_n
#define ADC_LIGHT 3
#define ADC_TEMP  6

// button switch aliases
#define SW1     P3_1
#define SW2     P3_0
#define NUM_SW 2

// ds1302 pins
#define DS_CE    P0_0
#define DS_IO    P0_1
#define DS_SCLK  P3_2
// needed for asm optimizations
#define _DS_IO   _P0_1
#define _DS_SCLK _P3_2

#endif // #ifndef HWCONFIG_H

