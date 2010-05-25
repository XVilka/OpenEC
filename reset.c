/*-------------------------------------------------------------------------
   reset.c - reset the EC

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

   As a special exception, you may use this file as part of a free software
   library for the XO of the One Laptop per Child project without restriction.
   Specifically, if other files instantiate
   templates or use macros or inline functions from this file, or you compile
   this file and link it with other files to produce an executable, this
   file does not by itself cause the resulting executable to be covered by
   the GNU General Public License.  This exception does not however
   invalidate any other reasons why the executable file might be covered by
   the GNU General Public License.
-------------------------------------------------------------------------*/

#include <stdbool.h>
#include "chip.h"

#if defined(SDCC)

//! Do a full reset via external reset pin
/*! if this routine returns someone might be tampering the XO and clamps the line?

    \image latex ec_reset.png "EC_RST# (CH1) and TX (CH3) during EC reboot" width=0.8\textwidth
    \image html  ec_reset.png "EC_RST# (CH1) and TX (CH3) during EC reboot"

    The figure shows the EC_RST# pin and the TX pin on the serial adapter during reboot on a B1.
    The EC_RST# pin is switched low for 10ms then rises again. The fall time 200us
    is not consistant with the specified output driver characteristics (page 8/9 of datasheet)
    and an 1uF external condensator.
    The lower trace shows the TX of the kb3700 UART, first the characters of the reboot
    message can be seen then later the bootup message of OpenEC (solid black block).
 */
void reboot(void)
{
    static const __code unsigned char msg[]= {'r','e','b','o','o','t'};

    EA = 0;

    /* disable external Reset input ECRST# */
    GPIOIE08 &= ~0x20;
    /* data to low */
    GPIOD08  &= ~0x20;
    /* enable output */
    GPIOOE08 |= 0x20;

    /* allow output to settle.
       Output is connected to 1uF condensator and specified
       for 2..4 mA source/sink current.
       Needs 1.65ms nominally (and about 200us measured here)
       Wait a little longer and while at it output a message */

    __asm
        mov     dptr, #_reboot_msg_1_1
        mov     r6, #0x00
        mov     r7, #0x00

    00001$:                     ; inner delay loop
        djnz    r6, 00001$

        cjne    r7, #6, .+1     ; if( r7 < sizeof msg ) { TI = 0; SBUF = msg[r7]; }
        jnc     00002$
        mov     a, r7
        movc    a, @a+dptr      ; loading message
        clr     TI
        mov     SBUF, a         ; oops, clearing TI seems needed

    00002$:
        inc     r7
        cjne    r7, #18, 00001$ ; outer loop  (18 results in about 10ms)

        mov     dptr, #_GPIOIE08 ; enable external reset input again
        movx    a, @dptr
        orl     a, #0x20
        movx    @dptr, a
        ;
        nop
    __endasm;

    /* Code below should never be executed - or is someone clamping the line???
       What to do then?
       Continue as if nothing happened? Let the calling function decide.
     */

    /* disable ECRST# as output */
    GPIOOE08 &= ~0x20;
    EA = 1;

}

#else

void reboot(void)
{}

#endif