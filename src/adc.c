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
#include "stc15.h"
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
	uint8_t upper8;
	upper8 = getADCResult8(chan);
	return  upper8 << 2 | (ADC_RESL & 0b11) ;  //Return ADC result, 10 bits
}

uint8_t getADCResult8(uint8_t chan)
{
	ADC_CONTR = ADC_POWER | ADC_SPEEDHH | ADC_START | chan;
	_nop_;       //Must wait before inquiry
	while (!(ADC_CONTR & ADC_FLAG));  //Wait complete flag
	ADC_CONTR &= ~ADC_FLAG;           //Close ADC
	return  ADC_RES;  //Return ADC result, 8 bits
}


