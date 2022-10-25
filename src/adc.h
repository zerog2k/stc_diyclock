// STC15 Series MCU A/D Conversion Demo from www.stcmcu.com
#include <stdint.h>

#include "stc15.h"

#define _nop_ __asm nop __endasm;

// Define ADC_CONTR operation constants
#define ADC_POWER 0x80    // ADC power control bit
#define ADC_FLAG 0x10     // ADC complete flag
#define ADC_START 0x08    // ADC start control bit
#define ADC_SPEEDLL 0x00  // 540 clocks
#define ADC_SPEEDL 0x20   // 360 clocks
#define ADC_SPEEDH 0x40   // 180 clocks
#define ADC_SPEEDHH 0x60  // 90 clocks

void InitADC(uint8_t chan);

// Get ADC result - 10 bits
uint16_t getADCResult(uint8_t chan);

// Get ADC result - 8 bits
uint8_t getADCResult8(uint8_t chan);
