//
// STC15F204EA blinky example
// inspired by http://jjmz.free.fr/?p=179 http://jjmz.free.fr/?p=191
// and stc15f204ea datasheet
//

#include <stc12.h>
#include <stdint.h>
#include <stdio.h>

/* ------------------------------------------------------------------------- */
/* Printing functions */
/* ------------------------------------------------------------------------- */

#include "./soft_serial/serial.h"
#define printf printf_small     // see sdcc user guide

// P0.1 called P5.5 on my board?
#define LED P1_6     

/* ------------------------------------------------------------------------- */

// counter
int temp = 0;

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

const char* startstring = "\nSTC15F204EA starting up...\n";

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
        WDT_CONTR |= 1 << 4; // clear wdt
    }
}
/* ------------------------------------------------------------------------- */
