/*-------------------------------------------------------------------------
   watchdog.c - handle watchdog of the EC

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
#include "reset.h"
#include "states.h"
#include "timer.h"
#include "watchdog.h"

//! This is periodically reset. Subsystems should set their bit if they feel fine.
/*! may live in __data memory.
    (Because instructions like
        "orl _watchdog_all_up_and_well,#0x80"
    are atomic)
 */
unsigned char __data watchdog_all_up_and_well;


//! We have to either disable or handle the watchdog
/*! Does it make sense to handle it? Probably yes. 
    Multiplying the number of XOs with the number of MIPS
    with the number of seconds during XO lifetime
    will give an impressive number.
 */
void watchdog_init(void)
{
    /* reset pending flags */
    WDTPF = 0x03;

    /* disable watchdog for now */
    //WDTCFG = 0x48;

    /* enable IRQ and reset WDT */
    WDTCFG = 0x03;

    /* time in units of 64 ms until IRQ occurs. */
    WDTCNT = 50;

    /* allow watchdog interrupt */
    P0IE |= 0x01;
}



//! hopefully unused
/*! This interrupt is _not_ used to calm the watchdog again.
    Instead, if the IRQ was triggered, it reboots gracefully.
 */
void watchdog_interrupt(void) __interrupt(0x08)
{
    #define WATCHDOG_INTERRUPT_STACK_FOOTPRINT (7)

    EA = 0;

    /* note that we came here and why we came here. */
    STATES_UPDATE(watchdog, watchdog_all_up_and_well | WATCHDOG_IRQ_OCCURED);

    /* note when we came here.
       tick is consistent as WDT IRQ does not interrupt Timer IRQ.
     */
    states.timestamp = tick;

    /* note where we came from. Reads the return address from stack.
     */
    {
        unsigned char p = SP - WATCHDOG_INTERRUPT_STACK_FOOTPRINT - 1;
        
        /* note, this information survives a reboot! 
           \see states.c and \see old_states for details.
           This means that in case of a watchdog reboot
           the program counter where the watchdog event 
           happened is known. And the corresponding line of
           the C-source can be found by looking into 
           the .rst (relocated lst) files. */
        states.watchdog_programcounter = *(unsigned int __idata *) p;
    }

    /* reset pending flag? */
    P0IF &= ~0x01;

    /* disabling WDT interrupt, bad enough we came here once */
    P0IE &= ~0x01;

    /* tell the outside world (LED, Host) */

#if defined( SDCC )
    /* force reboot */
    __asm
        ; sending message via UART
        mov   r2, #0x00
        djnz  r2, .-1   ; delay for an eventually pending char
        clr   TI        ; many things may have gone wrong, not polling for TI
        mov   SBUF, #'W ; dummy comment with trailing '
        djnz  r2, .-1   ; delay
        clr   TI
        mov   SBUF, #'D ; dummy comment with trailing '
        djnz  r2, .-1   ; delay
        clr   TI
        mov   SBUF, #'T ; dummy comment with trailing '

        lcall _reboot   ; note, this reboot also resets the GPIO module
    __endasm;
#endif
}
