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

#ifndef REG_KB3700_H
#define REG_KB3700_H

#include "compiler.h"

/* 
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


/*
 *  now the kb3700 specific directly addressable registers
 */

SFR(P0IE,  0x80); // P0 Interrupt Enable register
SFR(PCON2, 0x86); // Processor Control register 2
SFR(P1IE,  0x90); // P1 Interrupt Enable register
SFR(SCON2, 0x9a); // Serial Port Control register 2
SFR(SCON3, 0x9b); // Serial Port Control register 3
SFR(P3IE,  0xb0); // P3 Interrupt Enable register

SFR(P0IF,  0xd8); // P0 Interrupt Flag register
SFR(P1IF,  0xe8); // P1 Interrupt Flag register
SFR(P3IF,  0xf8); // P3 Interrupt Flag register


/*
 *  now the kb3700 specific xdata addressable registers
 */

SFRX(GPTCFG,  0xfe50);
SFRX(GPTPF,   0xfe51);
SFRX(GPT0,    0xfe53);
SFRX(GPT1,    0xfe55);
SFRX(GPT2H,   0xfe56); /* oops, the endianness used here is suboptimal for 8051 :( */
SFRX(GPT2L,   0xfe57);
SFRX(GPT3H,   0xfe58);
SFRX(GPT3L,   0xfe59);

SFRX(CLKCFG,  0xff0d);


/* .. */

#endif
