 /*---------------------------------------------------------------------------------*/
/* --- STC MCU International Limited -------------------------------------
*/
/* --- STC 15 Series MCU A/D Conversion Demo -----------------------
*/
/* --- Mobile: (86)13922805190 --------------------------------------------
*/
/* --- Fax: 86-755-82944243 -------------------------------------------------*/
/* --- Tel: 86-755-82948412 -------------------------------------------------
*/
/* --- Web: www.STCMCU.com --------------------------------------------
*/
/* If you want to use the program or the program referenced in the  ---*/
/* article, please specify in which data and procedures from STC    ---
*/
/*----------------------------------------------------------------------------------*/
#include <stc12.h>
#include <stdint.h>

#define _nop_ __asm nop __endasm;

/*Define ADC operation const for ADC_CONTR*/
#define ADC_POWER   0x80            //ADC power control bit
#define ADC_FLAG    0x10            //ADC complete flag
#define ADC_START   0x08            //ADC start control bit
#define ADC_SPEEDLL 0x00             //540 clocks
#define ADC_SPEEDL  0x20            //360 clocks
#define ADC_SPEEDH  0x40            //180 clocks
#define ADC_SPEEDHH 0x60            //90 clocks

/*----------------------------
Initialize ADC sfr
----------------------------*/
void InitADC(uint8_t chan);

/*----------------------------
Get ADC result - 10 bits
----------------------------*/
uint16_t getADCResult(uint8_t chan);

/*----------------------------
Get ADC result - 8 bits
----------------------------*/
uint8_t getADCResult8(uint8_t chan);



