// LED functions for 4-digit seven segment led

#include <stdint.h>

// index into ledtable[]
#define LED_a   0xa
#define LED_b   0xb
#define LED_c   0xc
#define LED_d   0xd
#define LED_e   0xe
#define LED_f   0xf
#define LED_BLANK  0x10
#define LED_DASH   0x11
#define LED_h   0x12
#define LED_dp  0x13

const uint8_t __at (0x1000) ledtable[]
 = {
    // digit to led digit lookup table
    // dp,g,f,e,d,c,b,a
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01100111, // 9
    0b01110111, // A
    0b01011110, // b
    0b00111001, // C
    0b01001111, // d
    0b01111100, // E
    0b01110001, // F
    0b00000000, // 0x10 - ' '
    0b01000000, // 0x11 - '-'
    0b01110100, // 0x12 - 'h'
    0b10000000, // 0x13 - '.'
};

uint8_t tmpbuf[4];
__bit   dot0;
__bit   dot1;
__bit   dot2;
__bit   dot3;

uint8_t dbuf[4];

uint8_t lval(uint8_t val) { uint8_t dg=~ledtable[val]; return dg; }
uint8_t l2val(uint8_t val) { uint8_t dg=ledtable[val]; dg=~((dg&0x40)|(dg&0x38)>>3|(dg&0x07)<<3); return dg; }

//#define filldisplay(pos,val,dp) { if (pos!=2) dbuf[pos]=(dp)?lval(val)&0x7F:lval(val); else dbuf[pos]=(dp)?l2val(val)&0x7f:l2val(val); }

#define clearTmpDisplay() { dot0=0; dot1=0; dot2=0; dot3=0; }

#define filldisplay(pos,val,dp) { tmpbuf[pos]=(uint8_t)(val); if (dp) dot##pos=1;}

#define updateTmpDisplay() { uint8_t val; \
                        val=tmpbuf[0]; dbuf[0]=dot0?lval(val)&0x7F:lval(val); \
                        val=tmpbuf[1]; dbuf[1]=dot1?lval(val)&0x7F:lval(val); \
                        val=tmpbuf[3]; dbuf[3]=dot3?lval(val)&0x7F:lval(val); \
                        val=tmpbuf[2]; dbuf[2]=dot2?l2val(val)&0x7F:l2val(val); }
                        
