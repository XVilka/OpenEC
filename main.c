/*-------------------------------------------------------------------------
   main.c - skeleton for the Embedded Controler of the OLPC project

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
/*!
   \mainpage
   This is GPL'ed source code targetting the Embedded Controler of the
   OLPC (One Laptop Per Child) Project. http://www.laptop.org

   \section Tools Tools used
    - sdcc 2.7.0, http://sdcc.sf.net    (Compiler)
    - doxygen, http://www.doxygen.org   (Source documentation tool)
    - srecord, http://srecord.sf.net    (Handling of hex (etc.) files)
    - make                              (GNU make)
    - gcc                               (Source is also compilable with GCC)
    - xxx                               (Software to download to target)
    - xxx                               (Hardware adapter to download to target)
    - XO B2..By                         (XO Hardware version this code targets)

   \section Related_Docu Related documentation
   http://wiki.laptop.org/go/Category.EC

   Publically available (but outdated) schematic:
   http://dev.laptop.org/attachment/ticket/477/SCHEMATIC1%20_%2012%20--%20EC%20KB3700.pdf

   \section Mailing_List Mailing list
   http://lists.laptop.org/pipermail/openec/

   \section Status Status
   very preliminary, <b>might DAMAGE HARDWARE! (no kidding)</b>
 */

#include <stdbool.h>
#include "kb3700.h"
#include "battery.h"
#include "matrix_3x3.h"
#include "port_0x6c.h"
#include "states.h"
#include "timer.h"
#include "unused_irq.h"
#include "watchdog.h"


//! This is set by an interrupt routine or a state machine
/*! Value is reset during each iteration of the main loop.
    \see may_sleep
 */ 
bool busy;


//! This is kind of "not busy" 
/*! It's value is expected to persevere for more than one 
    iteration of the main loop. Currently not in use?
    \see busy
 */
bool may_sleep = 1;


//! pre-C stuff
/*! Code that is executed before C-startup may be placed here.
    Do not rely on initialized variables here. 
    Hmm, basically only the stack pointer has been set up. 
    This is really very early stuff and is called from here:
    http://sdcc.svn.sourceforge.net/viewvc/sdcc/trunk/sdcc/device/lib/mcs51/crtstart.asm?view=markup
 */ 
unsigned char external_startup(void)
{
    // watchdog?
    // ports to HiZ?
    // kick PLL?    
    return 0;
}


//! State machine that completely handles keyboard input
/*! feeds input characters into a short input event buffer.
 */
bool handle_keyboard_in(void)
{
    return 0;
}

//! Communication from/to the main processor 
/*! Unfortunately the EC is not completely self contained. 
    Someone with much more CPU power might have a request. 
    \return - not zero if there is work to be done.
 */
bool handle_command(void)
{
    return 0;
}


//! There is an outer world
/*! Initialize ports to pull-up, push/pull and input.
    Do so to a enter a reasonable state without 
    expecting the main loop to be functional yet.

    GPIO module is not reset by watchdog resets.
 */ 
void port_init(void)
{
}

//! off-load blinking (and off-after-a-while) stuff to non IRQ
void handle_leds(void)
{
}

void uart0_init(void)
{
}

//! Entering low power mode
/*! IRQs should wake us again
 */
#define SLEEP() do \
{ \
    PCON |= 0x01; \
    /* maybe: PMUCFG = 0x53; */ \
} while(0)


//! You expected it: This routine is expected never to exit
void main (void)
{
    port_init();
    save_old_states();
    watchdog_init();
    timer_gpt3_init();
    uart0_init();

    /* enable interrupts. */
    EA = 1;

    /* The main loop contains several state machines handling
       various subsystems on the EC.
       The idea is to have each subroutine return as quickly
       as it can (no busy wait unless for a _very_ short delay).
       Also the subroutines should be pretty much self contained.

       Unless it is noted _here_ no state machine should need
       more than xxx microseconds before returning control.
       When a state machine returns control it is expected to
       have the variables it is working on in a consistent
       state. Period (just in case it was missed:^)

       If it helps: you may want to think of the main loop
       as a round-robin cooperative scheduler without the
       overhead this usually implies. This works well if, well,
       if _all_ routines within the main loop cooperate well.
     */
    while(1)
    {
        STATES_TIMESTAMP();

        busy = handle_command();
        busy |= handle_cursors();
        busy |= handle_battery();
        handle_leds();

        if( !busy && may_sleep )
            SLEEP();
    }
}

