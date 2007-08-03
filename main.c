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
    - make                              (GNU make, should be there anyway)
    - gcc                               (Source is also compilable with GCC)
    - xxx                               (Software to download to target)
    - xxx                               (Hardware adapter to download to target)
    - XO B2..By                         (XO Hardware version this code targets)

   \section Related_Docu Related documentation
   http://wiki.laptop.org/go/Category.EC

   Publically available (but outdated) schematic:
   http://dev.laptop.org/attachment/ticket/477/SCHEMATIC1%20_%2012%20--%20EC%20KB3700.pdf

   Collection of Datasheets:

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
unsigned char _sdcc_external_startup(void)
{
    /* the code here is meant be able to put the EC into a safe
       recovery mode that allows linux/BIOS to reflash EC code
       with an EC image that is known to work.

       We should take extra extra care that this works as intended.
     */

    // watchdog?
    // ports to HiZ?
    // kick PLL?


    /* The recovery mode does not want to be running on battery */
    if( 0 /* running from battery */ )
        return 0;

    /*
       setting up GPIO so we can detect key presses
     */

    /* set KEY_OUT_1 KEY_OUT_2, KEY_OUT_3 Open Drain Enable? */
    GPIOOD10 |= 0x70;

    /* set KEY_OUT_1 to output */
    GPIOOE10 |= 0x40;

    /* make sure KEY_IN_1 .. KEY_IN_3 are inputs */
    GPIOOE00 &= ~0x70;

    /* drive KEY_OUT_1 low. Eventually not a good idea
       to use this column of the matrix because its
       full name is KEY_OUT_1/ISP_EN# 
       Which one to use best? */
    GPIOD10 &= ~0x40;

    /* let KEY_OUT_2, KEY_OUT_3 be high */
    GPIOD10 |= (0x80 | 0x20);

    /* enable input for KEY_IN_1 .. KEY_IN_3 */
    /* Seems we have a problem here:
       http://wiki.laptop.org/go/EC_Register_Settings#OFW_Dump
       lists 0x99 as the value for GPIOIE00
       I would have expected bits 6,5,4 to be set.
       Which means that the register probably does not
       do what it is expected to.
       Also there seems to be a name clash in table 4.2.1:
       GPIOEIN0 is used for 0xfc64 and 0xfc34.
     */
    // GPIOIE00 |= 0x70;


    /* short delay to allow capacity of keyboard scan
       lines to be discharged. Probably not needed.
     */
    {
        volatile char counter = 10;
        do
           {}
        while(--counter);
    }


    /* now checking for if KEY_RT_L, KEY_LF_L and KEY_DN_L
       are simultanuously pressed. */
    if( 0x00 == (GPIOIN10 & 0x70))
    {
        /* switch on DCON, WLAN, POWER, LED */

        /* switch all IO ports to failsafe state */

        /* going into an (endless!?) loop waiting
           for the XO giving us a reset? */
        while (1  /* && !running from battery */)
        {
            /* reset watchdog pending flags */
            WDTPF = 0x03;

            /* signal to the host and user that
               we are here and are alive */
        }

        /* switch LED */
    };

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

