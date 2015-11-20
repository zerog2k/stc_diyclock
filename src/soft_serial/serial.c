/*--------------------------------------------------------------------------------*/
/* --- STC MCU International Limited -----------------------------------*/
/* --- STC 15 Series I/O simulate serial port ----------------------------*/
/* --- Web: www.STCMCU.com -----------------------------------------*/
/* If you want to use the program or the program referenced in the */
/* article, please specify in which data and procedures from STC */
/*-------------------------------------------------------------------------------*/
#include <stc12.h>
#define SYSCLOCK    11059200
#define BAUDRATE    9600
#define BAUDTMR     ((0 - (SYSCLOCK / 3 / BAUDRATE)) & 0xFFFF)
//#define BAUDTMR 	0xFE80 	// 9600bps @ 11.0592MHz


#define RXB P3_0
#define _RXB _P3_0
#define TXB P3_1
#define _TXB _P3_1
		
//define UART TX/RX port
typedef __bit BOOL;
typedef unsigned char BYTE;
typedef unsigned int WORD;

__data __at 0x08 volatile BYTE TBUF;
__data __at 0x09 volatile BYTE RBUF;
__data __at 0x0A volatile BYTE TDAT;
__data __at 0x0B volatile BYTE RDAT;
__data __at 0x0C volatile BYTE TCNT;
__data __at 0x0D volatile BYTE RCNT;
__data __at 0x0E volatile BYTE TBIT;
__data __at 0x0F volatile BYTE RBIT;

volatile BOOL TING,RING;
volatile BOOL TEND,REND;

void UART_INIT();

void putchar(unsigned char c)
{
	while (!TEND) ;
	TBUF=c; TEND=0; TING=1;	
}

#ifdef UART_TEST
BYTE t, r;
BYTE buf[16];

void uart_printf(const unsigned char *str)
{
	while (*str) putchar(*str++);
}
	
void main()
{
	UART_INIT();

	uart_printf("Testing UART :\n");

	while (1)
	{
	//user's function
	if (REND)
		{
		REND = 0;
		buf[r++ & 0x0f] = RBUF;
		}
		
	if (TEND)
		{
		if (t != r)
			{
			TEND = 0;
			TBUF = buf[t++ & 0x0f];
			TING = 1;
			}
		}
	
	}
}
#endif

//-----------------------------------------
//Timer interrupt routine for UART
//__data __at 0x08 BYTE TBUF;	=> ar0
//__data __at 0x09 BYTE RBUF;
//__data __at 0x0A BYTE TDAT;
//__data __at 0x0B BYTE RDAT;
//__data __at 0x0C BYTE TCNT;
//__data __at 0x0D BYTE RCNT;
//__data __at 0x0E BYTE TBIT;
//__data __at 0x0F BYTE RBIT;	=> ar7
void _tm0() __interrupt 1 __using 1
{
	__asm
		jb	_RING,00002$	
		jb	_RXB,00000$	
		setb	_RING
		mov	r5,#4
		mov	r7,#9
		sjmp	00000$	
	00002$:
		djnz	r5, 00000$	;  if (--RCNT==0)
		mov	r5,#3		; RCNT=3
		djnz	r7, 00001$	;  if (--RBIT==0)
		mov	_RBUF,r3	; RBUF <= r3 (RDAT)
		clr	_RING
		setb	_REND
		sjmp	00000$
	00001$:
		mov	a,r3		; r3=RDAT	
		mov	C,_RXB		; (C+RDAT)>>1
		rrc	a
		mov	r3,a
	00000$:
	__endasm;
	
	__asm
		djnz	r4,00010$
		mov	r4,#3
		jnb	_TING, 00010$
		mov	a, r6
		jnz	00011$
		clr	_TXB
		mov	_TDAT,r0		; TDAT=r2 <= r0=TBUF
		mov	r6,#9
		sjmp	00010$
	00011$:
		mov	a,r2
		setb	c
		rrc	a
		mov	r2,a
		mov	_TXB,c
		djnz	r6, 00010$
		clr	_TING
		setb	_TEND
	00010$:
	__endasm;
}

//-----------------------------------------
//initial UART module variable
void UART_INIT()
{
	TING = 0;
	RING = 0;
	TEND = 1;
	REND = 0;
	TCNT = 0;
	RCNT = 0;

	TMOD = 0x00;
	AUXR = 0x80;
	TL0 = BAUDTMR & 0xFF;
	TH0 = (BAUDTMR & 0xFF00) >> 8;
	TR0 = 1;
	ET0 = 1;
	PT0 = 1;
	EA = 1;
}

