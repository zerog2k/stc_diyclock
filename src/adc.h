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

typedef unsigned char BYTE;
typedef unsigned int WORD;
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
Software delay function
----------------------------*/
void _delay_ms(unsigned char ms);
void Delay(unsigned int n);

/*----------------------------
Get ADC result - 10 bits
----------------------------*/
unsigned int getADCResult(char chan);

/*----------------------------
Get ADC result - 8 bits
----------------------------*/
unsigned char getADCResult8(char chan);

/*----------------------------
Initialize ADC sfr
----------------------------*/
void InitADC(char chan);

