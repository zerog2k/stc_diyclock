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

void _delay_ms(unsigned char ms)
{	
    // i,j selected for fosc 11.0592MHz, using oscilloscope
    // the stc-isp tool gives inaccurate values (perhaps for C51 vs sdcc?)
    // max 255 ms
    unsigned char i, j;
    do {
    	i = 4;
    	j = 200;
    	do
    	{
    		while (--j);
    	} while (--i);
    } while (--ms);
}

/*----------------------------
Software delay function
----------------------------*/
void Delay(unsigned int n)
{
	unsigned int x;
	while (n--) {
		x = 5000;
		while (x--);
	}
}

/*----------------------------
Get ADC result - 10 bit
----------------------------*/
unsigned int getADCResult(char chan)
{
	ADC_CONTR = ADC_POWER | ADC_SPEEDHH | ADC_START | chan;
	_nop_;       //Must wait before inquiry
	while (!(ADC_CONTR & ADC_FLAG));  //Wait complete flag
	ADC_CONTR &= ~ADC_FLAG;           //Close ADC
	return  ADC_RES << 2 | (ADC_RESL & 0b11) ;  //Return ADC result
}

unsigned char getADCResult8(char chan)
{
	ADC_CONTR = ADC_POWER | ADC_SPEEDHH | ADC_START | chan;
	_nop_;       //Must wait before inquiry
	while (!(ADC_CONTR & ADC_FLAG));  //Wait complete flag
	ADC_CONTR &= ~ADC_FLAG;           //Close ADC
	return  ADC_RES;  //Return ADC result
}

/*----------------------------
Initial ADC sfr
----------------------------*/
void InitADC(char chan)
{
	P1ASF |= 1 << chan;             //enable channel ADC function
	ADC_RES = 0;                    //Clear previous result
	ADC_CONTR = ADC_POWER | ADC_SPEEDLL;
	Delay(2);                       //ADC power-on and delay
}

