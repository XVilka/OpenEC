/*-------------------------------------------------------------------------
   timer.c - timer routines for the Embedded Controler of the OLPC project

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


//! number of timer IRQs per second
#define HZ (100)

//! incremented by IRQ
/*! It's a 16 bit variable and the 8051 is a 8 bit CPU,
    so when reading this variable be sure that hi and lo
    byte are consistent ("atomic access").
    \see get_tick()
 */
volatile unsigned int __pdata tick;
volatile unsigned char __pdata tick_next_s;

//! might as well count seconds since 01.01.1970
/*! please no translation from/to YYYY MM DD on the EC! 
    (If subsecond resolution should be needed this
    could be achieved by a clever mix of 
    tick_next_s, tick, HZ, + and - operators)
 */
volatile unsigned long __pdata second;

 
 //! There is no embedded device without a timer, is there?
 /*! different speed if powered down?
  */
void timer_init(void)
{
  /* which one to use? */
}

//! every xx ms (HZ times per second)
/*! It is very tempting to insert stuff here:)
    Yet the price is high: Higher interrupt latency
    and interrupt jitter for other IRQ routines.

    And we cannot do "serious work" within IRQ as
    the code the compiler generates is not
    reentrant (unless you tell the compiler).
    On 8051 architecture reentrancy usually means
    high overhead.
    Also 16 bit data within a struct handled by
    the state machines might be in an intermediate
    state (or the variables are not consistent within
    themselves (f.e. temperature was read but
    flag for overtemperature not yet updated)).

    Please do not call subroutines here (also
    make sure that f.e. no 16 bit multiply
    silently slips in).
    In general: off load work to the state-machines
    whereever possible and rely on the main loop
    spinning around quickly enough.
 */
void timer_IRQ(void) __interrupt(1)
{
    tick++;

    if( (unsigned char)tick == tick_next_s )
    {
        tick_next_s += HZ;
        second++;
    }
}


//! savely gets the timer tick
int get_tick(void)
{
    unsigned int t;

    EA = 0; /* should not touch EA, instead mask the timer IRQ */
    t = tick;
    EA = 1;

    return t;
}
