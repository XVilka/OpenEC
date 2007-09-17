/*-------------------------------------------------------------------------
   uart.c - handle serial IO on the EC

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

#include <stdbool.h>
#include "kb3700.h"
#include "timer.h"

#define BITBANG (0)
#define BAUDRATE (115200uL)

#if BITBANG && defined(SDCC)

void putchar(unsigned char c)
{
    __asm
        mov     r2,dpl      ; argument
        mov     r3,#0x08    ; number of bits
        mov     dptr,#_GPIOD00

        ;
        clr     ea          ; disable IRQ


        movx    a,@dptr
        anl     a,#0xBF     ; TX_LOW;
        movx    @dptr,a

        mov     r4,#0x03
    STARTBITDELAY$:
        djnz    r4,STARTBITDELAY$
        ;

    BITLOOP$:
        mov     a,r2
        rrc     a           ; bit to send is in Carry now
        mov     r2,a
        ;

        movx    a,@dptr
        mov     _ACC_6,c    ; move carry into bit 6
        movx    @dptr,a
        ;
        mov     r4,#0x02
    BITDELAY$:
        djnz    r4,BITDELAY$
        nop
        ;

        djnz    r3,BITLOOP$
        ;

        mov     r4,#0x02
    BITDELAY2$:
        djnz    r4,BITDELAY2$
        ;

        movx    a,@dptr
        orl     a,#0x40     ; TX_HIGH;
        movx    @dptr,a

        ;
        setb    ea          ; enable IRQ
        ;

        mov     r4,#0x0a
    STOPBITDELAY:
        djnz    r4,STOPBITDELAY
        ;

    __endasm;
}


void uart_init()
{
    GPIOD00  |=  0x40; /**< set TX high */
    GPIOOE00 |=  0x40; /**< output enable for TX */
}

#elif !BITBANG && defined (SDCC)

void putchar(unsigned char c)
{
    while( !TI )
        ;
    TI = 0;
    SBUF = c;
}

//! unsigned? unsigned.
/*! please never poll here. Check RI before calling.

    Ooops, seems WLAN has to be switched ON
    otherwise RX/BAT_L0 is clamped at around 2V... (B1)

    \todo eventually add a resistor between (CN24,3 and CON2,45) so that using
    \todo the debricking adapter can not do harm to the WLAN module
 */
unsigned char getchar()
{
    while( !RI )
        ;
    RI = 0;
    return SBUF;
}

void uart_init()
{
    /* the IO part */
    GPIOFS00 |=  0x40; /**< switch to alternate output for TX */
    GPIOFS00 &= ~0x80; /**< no alternate output for RX */

    GPIOOE00 &= ~0x40; /**< no output enable for TX !!! GPIOFS seems to do it */
    GPIOOE00 &= ~0x80; /**< no output enable for RX */

    GPIOIE00 &= ~0x40; /**< TX is not input */
    GPIOIE00 |=  0x80; /**< RX is input */

    GPIOMISC |=  0x01; /**< GPIO06 is E51_TXD */
    GPIOMISC &= ~0x02; /**< GPIO07 is not E51_CLK */


    /* the UART part */
    SCON  = 0x52;      /**< not unusual */

    /* Would have expected something this: */
    SCON2 = (unsigned char)((SYSCLOCK + BAUDRATE)/ (2 * BAUDRATE) >> 8);
    SCON3 = (unsigned char)((SYSCLOCK + BAUDRATE)/ (2 * BAUDRATE));

    /* But, hmmm, 0x7f gives the desired 115 kBaud here (measured with 
       default PLLCFG).
       Seems the default PLLCFG of 0x70 trimms the PLL too low.
       Set PLLCFG to 0x80 (reset default is 0x70) results in successful
       communication (111.1 kBaud instead of 115.2 kBaud).
       (If baudrate does not match, please either adjust here or
       preferably adjust PLLCFG in _sdcc_external_startup())
     */
//    SCON2 = 0x00;
//    SCON3 = 0x7f;

    /* seems no need to enable a timer:) */

    /* sets TI when character is transmitted,
       another hmmm, I do never see this character. */
    SBUF = '*';
}

#else

/* gcc and others use their putchar() for now */
//void putchar(unsigned char c){}

/* dummy */
void uart_init()
{
}

#endif


void puthex(unsigned char c)
{
    putchar("0123456789abcdef"[ c >> 4 ]);
    putchar("0123456789abcdef"[ c & 0x0f ]);
}

void puthex_u16(unsigned int i)
{
    puthex((unsigned char)(i>>8));
    puthex((unsigned char)i);
}

void putspace()
{
    putchar(' ');
}

void putcrlf()
{
    putchar('\r');
    putchar('\n');
}

unsigned char putstring(unsigned char __code *p)
{
    unsigned char c;
    unsigned char len = 0;

    while( (c = *p++) )
    {
        putchar(c);
        len++;
    }
    return len;
}

