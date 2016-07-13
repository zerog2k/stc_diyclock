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
#include "adc.h"

/*----------------------------
Initial ADC sfr
----------------------------*/
void InitADC(uint8_t chan)
{
	P1ASF |= 1 << chan;             //enable channel ADC function
	ADC_RES = 0;                    //Clear previous result
	ADC_CONTR = ADC_POWER | ADC_SPEEDLL;
	//Delay(2);                       //ADC power-on and delay
}

/*----------------------------
Get ADC result - 10 bit
----------------------------*/
uint16_t getADCResult(uint8_t chan)
{
        uint8_t tmp=getADCResult8(chan);	// 8bit result is in DPL
__asm
        mov     r7,dpl
	; bits 7&6 will be bit 1&0 of 16bit MSB
        mov     a,#0xc0
        anl     a,r7
        rl      a
        rl      a
        mov     dph,a
	; bits 5..0 will be left shifted by 2
        anl     ar7,#0x3f
        mov     a,r7
        add     a,r7
        add     a,acc
        mov     r7,a
        ; add ADC_RESL bits 1&0
        mov     a,#0x03
        anl     a,_ADC_RESL
        orl     a,ar7
	; on 16bit result LSB
        mov     dpl,a
__endasm;
}

uint8_t getADCResult8(uint8_t chan)
{
	ADC_CONTR = ADC_POWER | ADC_SPEEDHH | ADC_START | chan;
	_nop_;       //Must wait before inquiry
	while (!(ADC_CONTR & ADC_FLAG));  //Wait complete flag
	ADC_CONTR &= ~ADC_FLAG;           //Close ADC
	return  ADC_RES;  //Return ADC result
}


