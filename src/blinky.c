
#include <stc12.h>
#include <stdint.h>
#include <stdio.h>

/* ------------------------------------------------------------------------- */
/* Printing functions */
/* ------------------------------------------------------------------------- */
#include "./soft_serial/serial.h"
#define printf printf_small

// P0.1 called P5.5 on my board?
#define LED P1_6     

/* ------------------------------------------------------------------------- */

int temp = 0;

void _delay_ms(unsigned char ms)
{	
    // i,j selected for fosc 11.0592MHz, using oscilloscope
    // the stc-isp tool gives unusable values (perhaps for C51 vs sdcc?)
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

const char* startstring = "\n TX Device Ready!\n";

int main()
{
    /* init the software uart */
    UART_INIT();

    /* simple greeting message */
    printf("%s", startstring);
    
    LED = 1;
    
    while(1)
    {                
        LED = 0;
		_delay_ms(100);
        //LED = 1;
        //_delay_ms(100);
        //LED = 0;
        //_delay_ms(100);
        LED = 1;        
        _delay_ms(255);
        _delay_ms(255);
        printf("counter: %d \n", temp);
        temp++;
        //CLR_WDT = 0b1;   // service WDT // for some reason - this doesnt work right
        WDT_CONTR |= 1 << 4; // clear wdt
    }
}
/* ------------------------------------------------------------------------- */
