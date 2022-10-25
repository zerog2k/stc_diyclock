// STC15 Series MCU A/D Conversion Demo from www.stcmcu.com
#include "adc.h"

void InitADC(uint8_t chan) {
  P1ASF |= 1 << chan;  // enable channel ADC function
  ADC_RES = 0;         // Clear previous result
  ADC_CONTR = ADC_POWER | ADC_SPEEDLL;
  // Delay(2);         // ADC power-on and delay
}

// Get ADC result - 10 bits
uint16_t getADCResult(uint8_t chan) {
  uint8_t upper8 = getADCResult8(chan);
  return (upper8 << 2) | (ADC_RESL & 0b11);  // Return ADC result, 10 bits
}

// Get ADC result - 8 bits
uint8_t getADCResult8(uint8_t chan) {
  ADC_CONTR = ADC_POWER | ADC_SPEEDHH | ADC_START | chan;
  _nop_;  // Must wait before inquiry
  while (!(ADC_CONTR & ADC_FLAG))
    /* wait */;            // Wait complete flag
  ADC_CONTR &= ~ADC_FLAG;  // Close ADC
  return ADC_RES;          // Return ADC result, 8 bits
}
