#ifndef HWCONFIG_H
#define HWCONFIG_H

// hardware definitions

// as we start to discover different hw variants with various features, chipsets, pinouts, etc
// we can capture things like HW_MODEL_C to describe this, in absence of any more descriptive naming

// WITH_RELAY -> stc15f204ea or stc15w404as revision with relay
// WITH_VOICE -> stc15w408as revision with voice chip
// HW_MODEL_C described @ https://github.com/zerog2k/stc_diyclock/issues/20


// alias for relay and buzzer outputs, using relay to drive led for indication of main loop status
// only for revision with stc15f204ea
#if defined stc15f204ea || defined stc15w404as
#define WITH_RELAY
#define WITH_BUZZER
#elif defined HW_MODEL_C
#define WITH_BUZZER
#elif defined stc15w408as && defined WITH_VOICE
#define WITH_3BTN
#else
// normal clock with buzzer
#define WITH_BUZZER
#endif

#ifdef WITH_RELAY
#define RELAY P1_4
#else
#define RELAY
#endif

// revision with stc15w408as (with voice chip and three buttons)
#ifdef WITH_VOICE
#define LED P1_5
#endif

#ifdef WITH_BUZZER
#  ifdef HW_MODEL_C
#  define BUZZER P3_3
   // additional pins on P3 header: P3_6 P3_7
#  else
#  define BUZZER P1_5
#  endif
#define BUZZER_ON  BUZZER = 0
#define BUZZER_OFF BUZZER = 1
#else
#define BUZZER_ON
#define BUZZER_OFF
#endif

// 7-seg LED port setup
#define LED_SEGMENT_PORT P2
#define LED_DIGITS_PORT P3

// offset where the digits start on LED_DIGITS_PORT
#ifdef HW_MODEL_C
#define LED_DIGITS_PORT_BASE 4
#else
#define LED_DIGITS_PORT_BASE 2
#endif

// setup macro mask to turn off digits
#define LED_DIGITS_OFF() (LED_DIGITS_PORT |= (0b1111 << LED_DIGITS_PORT_BASE))
// setup macro to turn on single digit
#define LED_DIGIT_ON(digit) (LED_DIGITS_PORT &= ~((1 << LED_DIGITS_PORT_BASE) << digit))

// adc channels for sensors, P1_n
#if defined HW_MODEL_C
#define ADC_LIGHT 3
#define ADC_TEMP 6
#else
#define ADC_LIGHT 6
#define ADC_TEMP 7
#endif

// button switch aliases
// SW3 only for revision with stc15w408as and voice
#ifdef WITH_3BTN
#define SW3 P1_4
#define NUM_SW 3
#else
#define NUM_SW 2
#endif
#define SW2 P3_0
#define SW1 P3_1

// DS1302 pins
#if defined HW_MODEL_C
#define DS_CE P0_0
#define DS_IO P0_1
#define DS_SCLK P3_2
// needed for asm optimizations
#define _DS_IO _P0_1
#define _DS_SCLK _P3_2
#else // not HW_MODEL_C
#define DS_CE P1_0
#define DS_IO P1_1
#define DS_SCLK P1_2
// needed for asm optimizations
#define _DS_IO _P1_1
#define _DS_SCLK _P1_2
#endif // not HW_MODEL_C

#endif // HWCONFIG_H
