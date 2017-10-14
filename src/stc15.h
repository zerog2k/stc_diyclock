#ifndef _STC15_H_
#define _STC15_H_

#include <8051.h>
#include <compiler.h>

#ifdef REG8051_H
#undef REG8051_H
#endif

/*  P4  */
__sfr __at (0xC0) P4;
__sbit __at (0xC0) P4_0;
__sbit __at (0xC1) P4_1;
__sbit __at (0xC2) P4_2;
__sbit __at (0xC3) P4_3;
__sbit __at (0xC4) P4_4;
__sbit __at (0xC5) P4_5;
__sbit __at (0xC6) P4_6;
__sbit __at (0xC7) P4_7;

__sfr __at 0x94 P0M0;
__sfr __at 0x93 P0M1;
__sfr __at 0x92 P1M0; 
__sfr __at 0x91 P1M1; 
__sfr __at 0x96 P2M0;
__sfr __at 0x95 P2M1;
__sfr __at 0xB2 P3M0;
__sfr __at 0xB1 P3M1;
__sfr __at 0xB4 P4M0;
__sfr __at 0xB3 P4M1;
__sfr __at 0xCA P5M0;
__sfr __at 0xC9 P5M1;
__sfr __at 0xCC P6M0;
__sfr __at 0xCB P6M1;
__sfr __at 0xE2 P7M0;
__sfr __at 0xE1 P7M1;

__sfr __at 0x8E AUXR; 
__sfr __at 0xA2 AUXR1;
__sfr __at 0xA2 P_SW1;
__sfr __at 0x97 CLK_DIV;
__sfr __at 0xA1 BUS_SPEED;
__sfr __at 0x9D P1ASF;
__sfr __at 0xBA P_SW2;

/*  IE  */
__sbit __at 0xAE ELVD;
__sbit __at 0xAD EADC;

/*  IP  */
__sbit __at 0xBF PPCA;
__sbit __at 0xBE PLVD;
__sbit __at 0xBD PADC;

__sfr __at 0xAF IE2;
__sfr __at 0xB5 IP2;
__sfr __at 0x8F INT_CLKO;

__sfr __at 0xD1 T4T3M;
__sfr __at 0xD1 T3T4M;
__sfr __at 0xD2 T4H;
__sfr __at 0xD3 T4L;
__sfr __at 0xD4 T3H;
__sfr __at 0xD5 T3L;
__sfr __at 0xD6 T2H;
__sfr __at 0xD7 T2L;
__sfr __at 0xAA WKTCL;
__sfr __at 0xAB WKTCH;
__sfr __at 0xC1 WDT_CONTR;

__sfr __at 0x9A S2CON;
__sfr __at 0x9B S2BUF;
__sfr __at 0xAC S3CON;
__sfr __at 0xAD S3BUF;
__sfr __at 0x84 S4CON;
__sfr __at 0x85 S4BUF;
__sfr __at 0xA9 SADDR;
__sfr __at 0xB9 SADEN;

//ADC
__sfr __at 0xBC ADC_CONTR; 
__sfr __at 0xBD ADC_RES;
__sfr __at 0xBE ADC_RESL;

//SPI
__sfr __at 0xCD SPSTAT;
__sfr __at 0xCE SPCTL;
__sfr __at 0xCF SPDAT;

//IAP/ISP
__sfr __at 0xC2 IAP_DATA; 
__sfr __at 0xC3 IAP_ADDRH;
__sfr __at 0xC4 IAP_ADDRL;
__sfr __at 0xC5 IAP_CMD;
__sfr __at 0xC6 IAP_TRIG;
__sfr __at 0xC7 IAP_CONTR; 

//PCA/PWM 
__sfr __at 0xD8 CCON; 
__sbit __at 0xDF CF;
__sbit __at 0xDE CR;
__sbit __at 0xDA CCF2;
__sbit __at 0xD9 CCF1;
__sbit __at 0xD8 CCF0;

__sfr __at 0xD9 CMOD; 
__sfr __at 0xE9 CL; 
__sfr __at 0xF9 CH; 
__sfr __at 0xDA CCAPM0; 
__sfr __at 0xDB CCAPM1;
__sfr __at 0xDC CCAPM2; 
__sfr __at 0xEA CCAP0L; 
__sfr __at 0xEB CCAP1L; 
__sfr __at 0xEC CCAP2L; 
__sfr __at 0xF2 PCA_PWM0; 
__sfr __at 0xF3 PCA_PWM1; 
__sfr __at 0xF4 PCA_PWM2; 
__sfr __at 0xFA CCAP0H; 
__sfr __at 0xFB CCAP1H;
__sfr __at 0xFC CCAP2H; 

__sfr __at 0xE6 CMPCR1;
__sfr __at 0xE7 CMPCR2;

//PWM
__sfr __at 0xf1 PWMCFG; 
__sfr __at 0xf5 PWMCR;
__sfr __at 0xf6 PWMIF;
__sfr __at 0xf7 PWMFDCR;

#define PWMC        (*(unsigned int  volatile xdata *)0xfff0)
#define PWMCH       (*(unsigned char volatile xdata *)0xfff0)
#define PWMCL       (*(unsigned char volatile xdata *)0xfff1)
#define PWMCKS      (*(unsigned char volatile xdata *)0xfff2)
#define PWM2T1      (*(unsigned int  volatile xdata *)0xff00)
#define PWM2T1H     (*(unsigned char volatile xdata *)0xff00)
#define PWM2T1L     (*(unsigned char volatile xdata *)0xff01)
#define PWM2T2      (*(unsigned int  volatile xdata *)0xff02)
#define PWM2T2H     (*(unsigned char volatile xdata *)0xff02)
#define PWM2T2L     (*(unsigned char volatile xdata *)0xff03)
#define PWM2CR      (*(unsigned char volatile xdata *)0xff04)
#define PWM3T1      (*(unsigned int  volatile xdata *)0xff10)
#define PWM3T1H     (*(unsigned char volatile xdata *)0xff10)
#define PWM3T1L     (*(unsigned char volatile xdata *)0xff11)
#define PWM3T2      (*(unsigned int  volatile xdata *)0xff12)
#define PWM3T2H     (*(unsigned char volatile xdata *)0xff12)
#define PWM3T2L     (*(unsigned char volatile xdata *)0xff13)
#define PWM3CR      (*(unsigned char volatile xdata *)0xff14)
#define PWM4T1      (*(unsigned int  volatile xdata *)0xff20)
#define PWM4T1H     (*(unsigned char volatile xdata *)0xff20)
#define PWM4T1L     (*(unsigned char volatile xdata *)0xff21)
#define PWM4T2      (*(unsigned int  volatile xdata *)0xff22)
#define PWM4T2H     (*(unsigned char volatile xdata *)0xff22)
#define PWM4T2L     (*(unsigned char volatile xdata *)0xff23)
#define PWM4CR      (*(unsigned char volatile xdata *)0xff24)
#define PWM5T1      (*(unsigned int  volatile xdata *)0xff30)
#define PWM5T1H     (*(unsigned char volatile xdata *)0xff30)
#define PWM5T1L     (*(unsigned char volatile xdata *)0xff31)
#define PWM5T2      (*(unsigned int  volatile xdata *)0xff32)
#define PWM5T2H     (*(unsigned char volatile xdata *)0xff32)
#define PWM5T2L     (*(unsigned char volatile xdata *)0xff33)
#define PWM5CR      (*(unsigned char volatile xdata *)0xff34)
#define PWM6T1      (*(unsigned int  volatile xdata *)0xff40)
#define PWM6T1H     (*(unsigned char volatile xdata *)0xff40)
#define PWM6T1L     (*(unsigned char volatile xdata *)0xff41)
#define PWM6T2      (*(unsigned int  volatile xdata *)0xff42)
#define PWM6T2H     (*(unsigned char volatile xdata *)0xff42)
#define PWM6T2L     (*(unsigned char volatile xdata *)0xff43)
#define PWM6CR      (*(unsigned char volatile xdata *)0xff44)
#define PWM7T1      (*(unsigned int  volatile xdata *)0xff50)        
#define PWM7T1H     (*(unsigned char volatile xdata *)0xff50)        
#define PWM7T1L     (*(unsigned char volatile xdata *)0xff51)
#define PWM7T2      (*(unsigned int  volatile xdata *)0xff52)
#define PWM7T2H     (*(unsigned char volatile xdata *)0xff52)
#define PWM7T2L     (*(unsigned char volatile xdata *)0xff53)
#define PWM7CR      (*(unsigned char volatile xdata *)0xff54)

#endif

