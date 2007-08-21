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

void putchar(unsigned char c)
{
    volatile unsigned int t = 1000; /**< no endless loop for now
                                         (later there's a watchdog so
                                         endless loops are not bad per se) */

    while( !TI && --t )
        ;
    SBUF = c;
}


void uart_init()
{
    GPIOFS00 |=  0x40; /**< switch to alternate output for TX */
    GPIOFS00 &= ~0x80; /**< no alternate output for RX */

    GPIOOE00 |=  0x40; /**< output enable for TX */
    GPIOOE00 &= ~0x80; /**< no output enable for RX */

    GPIOIE00 &= ~0x40; /**< TX is not input */
    GPIOIE00 |=  0x80; /**< RX is input */

    GPIOMISC |=  0x01; /**< GPIO06 is E51_TXD */
    GPIOMISC &= ~0x02; /**< GPIO07 is not E51_CLK */

    SCON  = 0x52;      /**< not unusual */
    SCON2 = 0x9e;      /**< something */
    SCON3 = 0x9f;      /**< dont care for now */

// And now for the desparate part:
// switch on timers that might be used just to get
// uart running with _any_ speed. Remove!
TMOD = 0x11;
TR0 = 1;
TR1 = 1;
}
