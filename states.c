/*-------------------------------------------------------------------------
   states.c - States machines

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
#include "states.h"
#include "matrix_3x3.h"
#include "uart.h"

#define DEBUG_MATRIX_3x3 (1)

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

    Currently the equivalent of:
    memcpy(0xfb80, 0xfbc0, 0x40);
    memset(0xfbc0, 0x00, 0x40);
    is done on startup so this struct will be mirrored there.
 */
struct states_type __xdata __at (0xfbc0) states;
struct states_type __xdata __at (0xfb80) old_states;

unsigned char __pdata print_states_enable;

#define BACKFILL (0xee)

#if defined( SDCC )

//! save a copy of state machine states (and other info)
/*! This is meant to be called once after boot. */
void save_old_states( void )
{
    __asm
        push    P2                          ; save
        ;
        mov     dptr, #_states              ; src
        mov     P2, #(_old_states >> 8)     ; dst, now no IRQ using pdata please!
        mov     r0, #(_old_states & 0xff)   ; dst
        mov     r1, #0x40                   ; len
        mov     r2, #BACKFILL               ; zero (or cookie)
        ;
    mem_copy_clear_loop:                    ; do
        movx    a, @dptr                    ;   tmp = *src;
        xch     a,r2
        movx    @dptr,a                     ;   *src = BACKFILL;
        xch     a,r2
        inc     dptr                        ;   src++;
        movx    @r0, a                      ;   *dst = tmp;
        inc     r0                          ;   dst++;
        djnz    r1, mem_copy_clear_loop     ; while(--len);
        ;
        pop     P2                          ; restore
        ;
    __endasm;
}

#else

#include <string.h>

//! save a copy of state machine states (and other info)
/*! This is meant to be called once after boot.
    The high level version.
 */
void save_old_states( void )
{
    memcpy(&old_states, &states, sizeof states);
    memset(&states, BACKFILL, sizeof states);
}

#endif

void print_states_ruler (void)
{
     putstring("\r\ntime numb co ma po ba ds wa");
}

void print_states (void)
{
     unsigned char i;

     if( !print_states_enable )
         return;

     putstring("\r\n");
     puthex_u16(states.timestamp);
     putspace();
     puthex_u16(states.number);
     putspace();
     puthex(states.command);
     putspace();
     puthex(states.matrix_3x3);
     putspace();
     puthex(states.power);
     putspace();
     puthex(states.battery);
     putspace();
     puthex(states.ds2756);
     putspace();
     puthex(states.watchdog);
     putspace();

#if DEBUG_MATRIX_3x3
     puthex(GPIOEIN0);
     putspace();
     for(i=0; i<5; i++)
     {
         puthex(((unsigned char __pdata *)&cursors)[i]);
         if( i==2 )
             putspace();
     }
     if( cursors.keycode_updated )
     {
         putchar('*');
         cursors.keycode_updated = 0;
     }
#endif

ow_dump();

}
