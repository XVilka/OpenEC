/*-------------------------------------------------------------------------
   kb3700.h - header file for ENE KB3700 Keyboard Controler

   Copyright (C) 2007  Frieder Ferlemann <Frieder.Ferlemann AT web.de>

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!
-------------------------------------------------------------------------*/

/*
 *  See KB3700-ds-01.pdf "KB3700 Keyboard Controller Datasheet"
 *  (Revision 0.1 July 2006)
 *  and http://wiki.laptop.org/go/EC_Register_Settings
 */

#ifndef REG_KB3700_H
#define REG_KB3700_H

#include "compiler.h"

/**
 * 8051 registers first
 */

SFR(P0, 0x80); // Port 1
    SBIT(P0_0, 0x80, 0);
    SBIT(P0_1, 0x80, 1);
    SBIT(P0_2, 0x80, 2);
    SBIT(P0_3, 0x80, 3);
    SBIT(P0_4, 0x80, 4);
    SBIT(P0_5, 0x80, 5);
    SBIT(P0_6, 0x80, 6);
    SBIT(P0_7, 0x80, 7);

SFR(SP,   0x81); // Stack Pointer.
SFR(DPL,  0x82); // Data Pointer Low.
SFR(DPH,  0x83); // Data Pointer High.
SFR(PCON, 0x87); // Power Control.
    #define IDL    0x01 //Idle Mode Enable.
    #define PD     0x02 //Power-Down Mode Enable.
    #define GF0    0x04
    #define GF1    0x08
    #define SMOD   0x80 //Baud Rate Double Bit (UART0)

SFR(TCON, 0x88); // Timer/Counter Control.
    SBIT(IT0, 0x88, 0); // Interrupt 0 type control bit.
    SBIT(IE0, 0x88, 1); // Interrupt 0 flag.
    SBIT(IT1, 0x88, 2); // Interrupt 1 type control bit.
    SBIT(IE1, 0x88, 3); // Interrupt 1 flag.
    SBIT(TR0, 0x88, 4); // Timer 0 run control flag.
    SBIT(TF0, 0x88, 5); // Timer 0 overflow flag.
    SBIT(TR1, 0x88, 6); // Timer 1 run control flag.
    SBIT(TF1, 0x88, 7); // Timer 1 overflow flag.

SFR(TMOD, 0x89); // Timer/Counter Mode Control.
    #define T0_M0  0x01 // Operation mode bit 0 for timer 0.
    #define T0_M1  0x02 // Operation mode bit 1 for timer 0.
    #define T0_GATE 0x08 // External enable for timer 0.
    #define T0_CT  0x04 // Timer or counter select for timer 0.
    #define T1_M0  0x10 // Operation mode bit 0 for timer 1.
    #define T1_M1  0x20 // Operation mode bit 1 for timer 1.
    #define T1_CT  0x40 // Timer or counter select for timer 1.
    #define T1_GATE 0x80 // External enable for timer 1.

SFR(TL0, 0x8a); // Timer 0 LSB.
SFR(TL1, 0x8b); // Timer 1 LSB.
SFR(TH0, 0x8c); // Timer 0 MSB.
SFR(TH1, 0x8d); // Timer 1 MSB.

SFR(P1, 0x90); // Port 1
    SBIT(P1_0, 0x90, 0);
    SBIT(P1_1, 0x90, 1);
    SBIT(P1_2, 0x90, 2);
    SBIT(P1_3, 0x90, 3);
    SBIT(P1_4, 0x90, 4);
    SBIT(P1_5, 0x90, 5);
    SBIT(P1_6, 0x90, 6);
    SBIT(P1_7, 0x90, 7);

SFR(SCON, 0x98);
    SBIT(RI,  0x98, 0); // Receive interrupt flag.
    SBIT(TI,  0x98, 1); // Transmit interrupt flag.
    SBIT(RB8, 0x98, 2); // In Modes 2 and 3, the 9th data bit that was received.
    SBIT(TB8, 0x98, 3); // The 9th data bit that will be transmitted in Modes 2 and 3.
    SBIT(REN, 0x98, 4); // Enables serial reception.
    SBIT(SM2, 0x98, 5); // Serial Port Mode Bit 2.
    SBIT(SM1, 0x98, 6); // Serial Port Mode Bit 1.
    SBIT(SM0, 0x98, 7); // Serial Port Mode Bit 0.

SFR(SBUF, 0x99); // Serial Port UART0 Data Buffer.

SFR(P2, 0xa0); // Port 2
    SBIT(P2_0, 0xa0, 0);
    SBIT(P2_1, 0xa0, 1);
    SBIT(P2_2, 0xa0, 2);
    SBIT(P2_3, 0xa0, 3);
    SBIT(P2_4, 0xa0, 4);
    SBIT(P2_5, 0xa0, 5);
    SBIT(P2_6, 0xa0, 6);
    SBIT(P2_7, 0xa0, 7);

SFR(IE, 0xa8); // Interrupt Enable Register.
    SBIT(EX0, 0xa8, 0); // Enable External Interrupt INT0.
    SBIT(ET0, 0xa8, 1); // Enable Timer 0 Interrupt.
    SBIT(EX1, 0xa8, 2); // Enable External Interrupt INT1.
    SBIT(ET1, 0xa8, 3); // Enable Timer 1 Interrupt.
    SBIT(ES,  0xa8, 4); // Enable UART0 Interrupt.
    SBIT(EA,  0xa8, 7); // Global disable bit.

SFR(P3, 0xb0); // Port 3
    SBIT(P3_0, 0xb0, 0);
    SBIT(P3_1, 0xb0, 1);
    SBIT(P3_2, 0xb0, 2);
    SBIT(P3_3, 0xb0, 3);
    SBIT(P3_4, 0xb0, 4);
    SBIT(P3_5, 0xb0, 5);
    SBIT(P3_6, 0xb0, 6);
    SBIT(P3_7, 0xb0, 7);

SFR(IP, 0xb8);  // Interrupt Priority Register.
    SBIT(PX0, 0xb8, 0); // External Interrupt INT0 priority level.
    SBIT(PT0, 0xb8, 1); // Timer 0 Interrupt priority level.
    SBIT(PX1, 0xb8, 2); // External Interrupt INT1 priority level.
    SBIT(PT1, 0xb8, 3); // Timer 1 Interrupt priority level.
    SBIT(PS0, 0xb8, 4); // UART0 Interrupt priority level.
    SBIT(PT2, 0xb8, 5); // Timer 2 Interrupt priority level.

SFR(ACC, 0xe0); // Accumulator.
    SBIT(ACC_0, 0xe0, 0); // Accumulator bit 0.
    SBIT(ACC_1, 0xe0, 1); // Accumulator bit 1.
    SBIT(ACC_2, 0xe0, 2); // Accumulator bit 2.
    SBIT(ACC_3, 0xe0, 3); // Accumulator bit 3.
    SBIT(ACC_4, 0xe0, 4); // Accumulator bit 4.
    SBIT(ACC_5, 0xe0, 5); // Accumulator bit 5.
    SBIT(ACC_6, 0xe0, 6); // Accumulator bit 6.
    SBIT(ACC_7, 0xe0, 7); // Accumulator bit 7.

SFR(B, 0xf0); // B Register
    SBIT(B_0, 0xf0, 0); // Register B bit 0.
    SBIT(B_1, 0xf0, 1); // Register B bit 1.
    SBIT(B_2, 0xf0, 2); // Register B bit 2.
    SBIT(B_3, 0xf0, 3); // Register B bit 3.
    SBIT(B_4, 0xf0, 4); // Register B bit 4.
    SBIT(B_5, 0xf0, 5); // Register B bit 5.
    SBIT(B_6, 0xf0, 6); // Register B bit 6.
    SBIT(B_7, 0xf0, 7); // Register B bit 7.




/**
 * now the kb3700 specific directly addressable registers
 */

SFR(P0IE,  0x80); // P0 Interrupt Enable register
SFR(PCON2, 0x86); // Processor Control register 2
SFR(P1IE,  0x90); // P1 Interrupt Enable register
SFR(SCON2, 0x9a); // Serial Port Control register 2, hi byte
SFR(SCON3, 0x9b); // Serial Port Control register 3, low byte
SFR(P3IE,  0xb0); // P3 Interrupt Enable register

SFR(P0IF,  0xd8); // P0 Interrupt Flag register
SFR(P1IF,  0xe8); // P1 Interrupt Flag register
SFR(P3IF,  0xf8); // P3 Interrupt Flag register


/**
 * now the kb3700 specific xdata addressable registers
 */

/* General Purpose IO (include ADC, DAC) */

SFRX(GPIOFS00,  0xfc00);        /**< Output Function Selection */
SFRX(GPIOFS08,  0xfc01);
SFRX(GPIOFS10,  0xfc02);
SFRX(GPIOFS18,  0xfc03);

SFRX(GPIOOE00,  0xfc10);        /**< Output Enable */
SFRX(GPIOOE08,  0xfc11);
SFRX(GPIOOE10,  0xfc12);
SFRX(GPIOOE18,  0xfc13);
SFRX(GPIOE0E0,  0xfc14);
SFRX(GPIOE0E8,  0xfc15);

SFRX(GPIOD00,   0xfc20);        /**< Data Output*/
SFRX(GPIOD08,   0xfc21);
SFRX(GPIOD10,   0xfc22);
SFRX(GPIOD18,   0xfc23);
SFRX(GPIOED0,   0xfc24);
SFRX(GPIOED8,   0xfc25);

SFRX(GPIOIN00,  0xfc30);        /**< Input Status */
SFRX(GPIOIN08,  0xfc31);
SFRX(GPIOIN10,  0xfc32);
SFRX(GPIOIN18,  0xfc33);
SFRX(GPIOEIN0,  0xfc34);
SFRX(GPIOEIN8,  0xfc35);
SFRX(GPIADIN,   0xfc36);

SFRX(GPIOPU00,  0xfc40);        /**< Pull Up Enable */
SFRX(GPIOPU08,  0xfc41);
SFRX(GPIOPU10,  0xfc42);
SFRX(GPIOPU18,  0xfc43);
SFRX(GPIOEPU0,  0xfc44);
SFRX(GPIOEPU8,  0xfc45);

SFRX(GPIOOD00,  0xfc50);        /**< Open Drain Enable*/
SFRX(GPIOOD08,  0xfc51);
SFRX(GPIOOD10,  0xfc52);
SFRX(GPIOOD18,  0xfc53);

SFRX(GPIOIE00,  0xfc60);        /**< Input Enable */
SFRX(GPIOIE08,  0xfc61);
SFRX(GPIOIE10,  0xfc62);
SFRX(GPIOIE18,  0xfc63);
//SFRX(GPIOEIN0,  0xfc64);      /**< name clash within table 4.2.1 */
//SFRX(GPIOEIN8,  0xfc65);      /**< name clash within table 4.2.1 */
//SFRX(GPIAD0,    0xfc66);      /**< GPIADIE or GPIAD0? */

SFRX(GPIOMISC,  0xfc70);

/** Keyboard Controler */
SFRX(KBCCB,     0xfc80);
SFRX(KBCCFG,    0xfc81);
SFRX(KBCCIF,    0xfc82);
SFRX(KBCHWEN,   0xfc83);
SFRX(KBCCMD,    0xfc84);
SFRX(KBCDAT,    0xfc85);
SFRX(KBCSTS,    0xfc86);

/** Pulse Width Modulation */
SFRX(PWMCFG,    0xfe00);
SFRX(PWMHIGH0,  0xfe01);
SFRX(PWMCYCL0,  0xfe02);
SFRX(PWMHIGH1,  0xfe03);
SFRX(PWMCYCL1,  0xfe04);
SFRX(PWMCFG2,   0xfe05);
SFRX(PWMCFG3,   0xfe06);
SFRX(PWMCFG4,   0xfe07);
SFRX(PWMHIGH2,  0xfe08);
SFRX(PWMHIGH3,  0xfe09);
SFRX(PWMHIGH4,  0xfe0a);
SFRX(PWMCYC2,   0xfe0b);
SFRX(PWMCYC3,   0xfe0c);
SFRX(PWMCYC4,   0xfe0d);

/** General Purpose Timer */
SFRX(GPTCFG,    0xfe50);
SFRX(GPTPF,     0xfe51);
SFRX(GPT0,      0xfe53);
SFRX(GPT1,      0xfe55);
SFRX(GPT2H,     0xfe56);
SFRX(GPT2L,     0xfe57);
SFRX(GPT3H,     0xfe58);
SFRX(GPT3L,     0xfe59);

/** Watchdog Timer */
SFRX(WDTCFG,    0xfe80);
SFRX(WDTPF,     0xfe81);
SFRX(WDTCNT,    0xfe82);
SFRX(WDT19_12,  0xfe83);
SFRX(WDT11_04,  0xfe84);
SFRX(WDT03_00,  0xfe85);

/** LPC Low Pin Count */
SFRX(LPCSTAT,   0xfe90);
SFRX(LPCSIRQ,   0xfe91);
SFRX(LPCBAH,    0xfe92);
SFRX(LPCBAL,    0xfe93);
SFRX(LPCFWH,    0xfe94);
SFRX(LPCCFG,    0xfe95);
SFRX(LPCXBAH,   0xfe96);
SFRX(LPCXBAL,   0xfe97);
SFRX(LPCEBAH,   0xfe98);
SFRX(LPCEBAL,   0xfe99);
SFRX(LPC_2EF,   0xfe9a);
SFRX(LPC_RSV_fe9b,      0xfe9b);
SFRX(LPC_2F_DATA,       0xfe9c);
SFRX(LPC68CFG,  0xfe9d);        /**< LPC 0x68/0x6c IO Configuration */
SFRX(LPC68CSR,  0xfe9e);        /**< LPC 0x68 Command Status Register */
SFRX(LPC68DAT,  0xfe9f);        /**< LPC 0x6c IO Data Register */

/** X-Bus SPI/ISP Interface*/
SFRX(XBISEG0,   0xfea0);
SFRX(XBISEG1,   0xfea1);
SFRX(XBIXIOEN,  0xfea4);
SFRX(XBICFG,    0xfea5);
SFRX(XBICS,     0xfea6);
SFRX(XBIWE,     0xfea7);
SFRX(SPIA0,     0xfea8);
SFRX(SPIA1,     0xfea9);
SFRX(SPIA2,     0xfeaa);
SFRX(SPIDAT,    0xfeab);
SFRX(SPICMD,    0xfeac);
SFRX(SPICFG,    0xfead);
SFRX(SPIDATR,   0xfeae);
SFRX(SPICFG2,   0xfeaf);

/** PS2 */
SFRX(PS2CFG,    0xfee0);
SFRX(PS2PF,     0xfee1);
SFRX(PS2CTRL,   0xfee2);
SFRX(PS2DATA,   0xfee3);
SFRX(PS2CFG2,   0xfee4);
SFRX(PS2PINS,   0xfee5);
SFRX(PS2PINO,   0xfee6);

/** Embedded Controler (hardware EC Space) */
SFRX(ECHV,      0xff00);
SFRX(ECFV,      0xff01);
SFRX(ECHA,      0xff02);
SFRX(ESCICFG,   0xff03);
SFRX(ECCFG,     0xff04);
SFRX(SCIE0,     0xff05);
SFRX(SCIE1,     0xff06);
SFRX(SCIE2,     0xff07);
SFRX(SCIF0,     0xff08);
SFRX(SCID,      0xff0b);
SFRX(PMUCFG,    0xff0c);
SFRX(CLKCFG,    0xff0d);
SFRX(EXTIO ,    0xff0e);
SFRX(PLLCFG,    0xff0f);
//;
SFRX(RSV_0xff11,0xff11);
SFRX(CLKCFG2,   0xff12);
SFRX(PLLCFG2,   0xff13);
SFRX(PXCFG,     0xff14);
SFRX(ADDAEN,    0xff15);
SFRX(PLLFRH,    0xff16);
SFRX(PLLFRL,    0xff17);
SFRX(ADCTRL,    0xff18);
SFRX(ADCDAT,    0xff19);
SFRX(ECIF,      0xff1a);
SFRX(ECDAT,     0xff1b);
SFRX(ECCMD,     0xff1c);
SFRX(ECSTS,     0xff1d);
SFRX(PLLVAL_A,  0xff1e);
SFRX(PLLVAL_B,  0xff1f);

/** General Purpose Wake-up (hardware EC Space) */
SFRX(GPWUEN00,  0xff30);
SFRX(GPWUEN08,  0xff31);
SFRX(GPWUEN10,  0xff32);
SFRX(GPWUEN18,  0xff33);

SFRX(GPWUPF00,  0xff40);
SFRX(GPWUPF08,  0xff41);
SFRX(GPWUPF10,  0xff42);
SFRX(GPWUPF18,  0xff43);

SFRX(GPWUPS00,  0xff50);
SFRX(GPWUPS08,  0xff51);
SFRX(GPWUPS10,  0xff52);
SFRX(GPWUPS18,  0xff53);

SFRX(GPWUEL00,  0xff60);
SFRX(GPWUEL08,  0xff61);
SFRX(GPWUEL10,  0xff62);
SFRX(GPWUEL18,  0xff63);

#endif
