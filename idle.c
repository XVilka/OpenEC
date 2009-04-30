/*-------------------------------------------------------------------------
   idle.c - handle idle mode on the EC

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
#include "kb3700.h"
#include "idle.h"

//! This is set by an interrupt routine or a state machine
/*! Value is reset during each iteration of the main loop.
    \see may_sleep
 */
bool busy;

//! This is kind of "not busy"
/*! It's value is expected to persevere for more than one
    iteration of the main loop.
    Note, "maysleep" might not be checked within the sleep mode
    (only when entering sleep mode - use "busy" if sleep mode
    should be exited). Currently not in use?
    \see busy
 */
bool may_sleep = 1;


#if defined(SDCC)

//! Entering low power mode
/*! IRQs should wake us again, but if they do not tell us
    to stay awake, then fall asleep again.
 */
void sleep_if_allowed( void )
{
    __asm

        jnb     _may_sleep, END$; not checked within the loop

    LOOP$:
        clr     EA              ; disable IRQ to avoid a race condition when checking flags
        jb      _busy, EXIT$
        setb    EA              ; enable IRQ again, next instruction is executed anyway
        orl     PCON, #0x01     ; sleep now, see PMUCFG and CLKCFG
        sjmp    LOOP$

    EXIT$:
        setb    EA

    END$:

    __endasm;
}


#else


//! Entering low power mode
/*! IRQs should wake us again, but if they do not tell us
    to stay awake, then fall asleep again.
 */
void sleep_if_allowed( void )
{
    /* disable IRQ to avoid a race condition when checking flags */
    EA = 0;

    while( !busy && may_sleep )
    {
        /* enable IRQ again, next instruction executed anyway */
        EA = 1;

        /* sleep, see PMUCFG and CLKCFG */
        PCON |= 0x01;

        /* data sheet mentions Ultra Low clock
           at one place (PMUCFG bit 3).
           Eventually/likely this is the 32768 Hz
           clock mentioned for GPT and WDT units.
           Can we use it? */

#if defined(SDCC)
        __asm
            nop
        __endasm;
#endif

        /* disable IRQ again */
        EA = 0;
    }

    /* leave with IRQ enabled */
    EA = 1;
}


#endif
