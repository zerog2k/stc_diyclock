// hardware definitions

// as we start to discover different hw variants with various features, chipsets, pinouts, etc
// we can capture things like HW_MODEL_C to describe this, in absence of any more descriptive naming

#ifndef HWCONFIG_H
#define HWCONFIG_H

/*
    HW_MODEL_C described @
    https://github.com/zerog2k/stc_diyclock/issues/20
*/

/*
    HW_WITH_GPS:
    uncomment for connecting NMEA GPS receiver to unit for time sync
    currently only working with STC15W408AS 
    (will not work on STC15F204EA - not enough code, memory, and no uart)
    currently reassigning uart pins to P3_6:rxd, P3_7:txd
*/
#define HW_WITH_GPS

// alias for relay and buzzer outputs, using relay to drive led for indication of main loop status
// only for revision with stc15f204ea
#if defined stc15f204ea || defined stc15w404as
 #define RELAY   P1_4
 #define BUZZER  P1_5
 #define BUZZER_ON  BUZZER = 0
 #define BUZZER_OFF BUZZER = 1

#elif defined HW_MODEL_C
// another model with STC15F204, but diff pinouts
 #define RELAY
 #define BUZZER     P3_3
 #define BUZZER_ON  BUZZER = 0
 #define BUZZER_OFF BUZZER = 1

 // additional pins on P3 header: P3_6 P3_7
#else // revision with stc15w408as (with voice chip)
 #define LED     P1_5
 #define BUZZER_ON
 #define BUZZER_OFF
#endif

// 7-seg led port setup

// which port the segments are connected to
#define LED_SEGMENT_PORT P2
// which port controls the digits
#define LED_DIGITS_PORT  P3

// offset where the digits start on LED_DIGITS_PORT
#if defined HW_MODEL_C
 #define LED_DIGITS_PORT_BASE   4
#else
 #define LED_DIGITS_PORT_BASE   2
#endif

// setup macro mask to turn off digits
#define LED_DIGITS_OFF()    ( LED_DIGITS_PORT |= (0b1111 << LED_DIGITS_PORT_BASE))
// setup macro to turn on single digit
#define LED_DIGIT_ON(digit) (LED_DIGITS_PORT &= ~((1<<LED_DIGITS_PORT_BASE) << digit))

// adc channels for sensors, P1_n
#if defined HW_MODEL_C
 #define ADC_LIGHT 3
 #define ADC_TEMP  6
#else
 #define ADC_LIGHT 6
 #define ADC_TEMP  7
#endif

// button switch aliases
// SW3 only for revision with stc15w408as
#ifdef stc15w408as
 #define SW3     P1_4
 #define NUM_SW 3
#else
 #define NUM_SW 2
#endif
#define SW2     P3_0
#define SW1     P3_1

// ds1302 pins
#if defined HW_MODEL_C
 #define DS_CE    P0_0
 #define DS_IO    P0_1
 #define DS_SCLK  P3_2
 // needed for asm optimizations
 #define _DS_IO   _P0_1
 #define _DS_SCLK _P3_2
#else
 #define DS_CE    P1_0
 #define DS_IO    P1_1
 #define DS_SCLK  P1_2
 // needed for asm optimizations
 #define _DS_IO   _P1_1
 #define _DS_SCLK _P1_2
#endif

#endif
