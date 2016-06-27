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

#define clearTmpDisplay() { dot0=0; dot1=0; dot2=0; dot3=0; tmpbuf[0]=tmpbuf[1]=tmpbuf[2]=tmpbuf[3]=LED_BLANK; }

#define filldisplay(pos,val,dp) { tmpbuf[pos]=(uint8_t)(val); if (dp) dot##pos=1;}
#define dotdisplay(pos,dp) { if (dp) dot##pos=1;}

#define updateTmpDisplay() { uint8_t val,tmp; \
                        val=tmpbuf[0]; tmp=~ledtable[val]; if (dot0) tmp&=0x7F; dbuf[0]=tmp; \
                        val=tmpbuf[1]; tmp=~ledtable[val]; if (dot1) tmp&=0x7F; dbuf[1]=tmp; \
                        val=tmpbuf[3]; tmp=~ledtable[val]; if (dot3) tmp&=0x7F; dbuf[3]=tmp; \
                        val=tmpbuf[2]; tmp=~ledtable[val]; tmp=( (tmp&0xC0)|(tmp&0x38)>>3|(tmp&0x07)<<3 ); if (dot2) tmp&=0x7F; dbuf[2]=tmp; }
                        
