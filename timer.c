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
#include "adc.h"
#include "power.h"
#include "watchdog.h"
#include "timer.h"


//! incremented by IRQ
/*! It's a 16 bit variable and the 8051 is a 8 bit CPU,
    so when reading this variable be sure that hi and lo
    byte are consistent ("atomic access").
    \see get_tick()
 */
volatile unsigned int __pdata tick;
static volatile unsigned char __pdata tick_next_s;

//! might as well count seconds since 01.01.1970
/*! please no translation from/to YYYY MM DD on the EC! 
    (If subsecond resolution should be needed this
    could be achieved by a clever mix of 
    tick_next_s, tick, HZ, + and - operators)
 */
volatile unsigned long __pdata second;


//! There is no embedded device without a timer, is there?
/*! different speed if powered down?
    Currently using the 8-bit timer with lowermost priority
    for the timer tick IRQ

    \bug HZ==100 and GPTCLOCK==32768u result in an
    \bug unexpected intervall of 10.70 ms here.
 */
void timer_gpt3_init(void)
{
    GPT3H = GPTCLOCK / HZ >> 8;
    GPT3L = GPTCLOCK / HZ & 0xff;

    /* IRQ enable & enable. Is it true that one bit is
       used with dual purpose? */
    GPTCFG |= 0x08;     /**< enable GPTn counting */

    /* start */
    GPTPF |= 0x80;

    /* set IRQ mask */
    P1IE |= 0x80;

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

    See section  "Common interrupt pitfall" in
    http://sdcc.sf.net/doc/sdccman.pdf for a(n incomplete:)
    list of what do avoid within IRQ.

    Please do not call subroutines here (also
    make sure that f.e. no 16 bit multiply
    silently slips in).
    In general: off load work to the state-machines
    whereever possible and rely on the main loop
    spinning around quickly enough.
 */
void timer_gpt3_interrupt(void) __interrupt(0x17)
{
    /* reset IRQ pending flag 
       is this the way to reset it?
       or rather GPTPF = 0x08; */
    GPTPF |= 0x08;

    tick++;

#if (HZ > 255)
# warning code expects HZ to fit in a byte
#endif

    if( (unsigned char)tick == tick_next_s )
    {
        tick_next_s += HZ;
        second++;

//        WDTCFG |= 0x01;
//        WDTPF = 0x03;

        /* handle watchdog. Here? */
        if( watchdog_all_up_and_well == (WATCHDOG_MAIN_LOOP_IS_FINE |
                                         WATCHDOG_PS2_IS_FINE |
                                         WATCHDOG_BATTERY_CODE_IS_FINE) )
        {
            /* reset pending flags */
            WDTPF = 0x03;
            /* reset this so subsystems have to report again */
            watchdog_all_up_and_well = 0x00;
        }

        ADC_START_CONVERSION;
    }

    busy = 1;
}


//! safely gets the timer tick
int get_tick(void)
{
    unsigned int t;

    /* mask the IRQ that changes tick */
    P1IE &= ~0x80;

    t = tick;

    /* reenable the IRQ. It was enabled was it? */
    P1IE |= 0x80;

    return t;
}

//! safely gets the time
unsigned long get_time(void)
{
    unsigned long t;

    /* mask the IRQ that changes tick */
    P1IE &= ~0x80;

    t = second;

    /* need subsecond resolution? */
    //subsecond = tick_next_s - tick;

    /* reenable the IRQ. It was enabled wasn't it? */
    P1IE |= 0x80;

    return t;
}

//! safely gets the time
unsigned long get_time_ms(void)
{
    unsigned long t;
    unsigned int subsecond;

    /* mask the IRQ that changes tick */
    P1IE &= ~0x80;

    t = second;

    /* need subsecond resolution? */
    subsecond = tick_next_s - tick;

    /* reenable the IRQ. It was enabled wasn't it? */
    P1IE |= 0x80;

    t *= 1000;
    t += subsecond * (1000/HZ);

    return t;
}


//! sets the time
/*! calling this might upset functions using the variable second

    \warning Listed in order of my personal preference:
    \warning a) should this clear the wake-up timer? or
    \warning b) should the wake-up timer be corrected accordingly? or
    \warning c) should we rely on the host doing it himself?
 */
void set_time(unsigned long s)
{
    /* mask the IRQ that changes tick */
    P1IE &= ~0x80;

    second = s;

    /* sync */
    tick_next_s = tick + HZ;

    /* reset timer and an eventually pending IRQ flag */
    GPTPF = 0x88;

    /* reenable the IRQ. It was enabled wasn't it? */
    P1IE |= 0x80;
}
