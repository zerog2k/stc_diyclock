//
// STC15F204EA blinky example
// inspired by http://jjmz.free.fr/?p=179 http://jjmz.free.fr/?p=191
// and stc15f204ea datasheet
//

#include <stc12.h>
#include <stdint.h>
#include <stdio.h>
#include "adc.h"

/* ------------------------------------------------------------------------- */
/* Printing functions */
/* ------------------------------------------------------------------------- */

#include "./soft_serial/serial.h"
#define printf printf_fast     // see sdcc user guide

#define RELAY   P1_4
#define BUZZER  P1_5
    
#define LED_A1  P3_2
#define LED_A2  P3_3
#define LED_A3  P3_4
#define LED_A4  P3_5

#define LED_a   P2_0
#define LED_b   P2_1
#define LED_c   P2_2
#define LED_d   P2_3
#define LED_e   P2_4
#define LED_f   P2_5
#define LED_g   P2_6
#define LED_dp  P2_7

#define ADC_LIGHT 6
#define ADC_TEMP  7

#define DS1302_RST  P1_0
#define DS1302_IO   P1_1
#define DS1302_CLK  P1_2

#define SW2     P3_0
#define SW1     P3_1



/* ------------------------------------------------------------------------- */

// counter
int temp = 0;

const char* startstring = "\nSTC15F204EA starting up...\n";

/*----------------------------
Send ADC result to UART
----------------------------*/
void ShowResult(char ch)
{
	unsigned int result = getADCResult(ch);
	printf("adc chan: %d, result: %03d\n", ch, result);                   
}

int main()
{
	  _delay_ms(100);
    /* init the software uart */
    UART_INIT();

    /* simple greeting message */
    printf("%s", startstring);
    
    RELAY = 1;
    //BUZZER = 1;
    while(1)
    {                
        RELAY = 0;
        //BUZZER = 0;
	      _delay_ms(250);
        
        RELAY = 1;
        //BUZZER = 1;
        _delay_ms(250);
        _delay_ms(250);
        _delay_ms(250);
        printf("counter: %d \n", temp);
        ShowResult(ADC_LIGHT);
        ShowResult(ADC_TEMP);
        temp++;
        WDT_CONTR |= 1 << 4; // clear wdt
    }
}
/* ------------------------------------------------------------------------- */
