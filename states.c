/*-------------------------------------------------------------------------
   states.c - skeleton for the Embedded Controler of the OLPC project

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

#include "states.h"

//! this variable is meant to be used for debugging purposes only
/*! the state machines work on their private state variables (which
    are thus protected from manipulation).
    It's kind of a luxury variable and it might go away or be not
    handled in the release version.  

    If the watchdog bites or external reset is applied this gives 
    a high level view about the state the EC was in.

    Note: this variable is located at the end of the memory area.
    This area is intended not to be cleared on startup. 
    Instead on startup this area is copied into another area
    so that the information stored here survives a reset and the
    state is still available when the EC has booted again.

    Currently the equivalent of memcpy(0x0780, 0x07c0, 0x40) is
    planned on startup so this struct will be mirrored there.
 */
struct states_type __xdata __at (0x07c0) states;
struct states_type __xdata __at (0x0780) old_states;

//! save a copy of state machine states (and other info)
/*! This is meant to be called once after boot. */
void save_old_states( void )
{
#if defined SDCC
    __asm
        push    P2                          ; save
        ;
        mov     dptr, #_states              ; src
        mov     P2, #(_old_states >> 8)     ; dst, now no IRQ using pdata please!
        mov     r0, #(_old_states & 0xff)   ; dst
        mov     r1, #0x40                   ; len
        ;
    memcpy_loop:
        movx    a, @dptr
        inc     dptr
        movx    @r0, a
        inc     r0
        djnz    r1, memcpy_loop
        ;
        pop     P2                          ; restore
        ;
    __endasm;
#endif
}
