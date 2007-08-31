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
#include "build.h"
#include "matrix_3x3.h"
#include "power.h"
#include "port_0x6c.h"
#include "sfr_dump.h"
#include "states.h"
#include "timer.h"
#include "uart.h"
#include "unused_irq.h"
#include "watchdog.h"


#define LED_CHG_G_OFF do{GPIOED0 |=  0x08;}while(0)
#define LED_CHG_G_ON  do{GPIOED0 &= ~0x08;}while(0)
#define LED_CHG_R_OFF do{GPIOD08 |=  0x04;}while(0)
#define LED_CHG_R_ON  do{GPIOD08 &= ~0x04;}while(0)


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
bool may_sleep = 0;


//! pre-C stuff
/*! Code that is executed before C-startup may be placed here.
    Do not rely on initialized variables here. 
    Hmm, basically only the stack pointer has been set up. 
    This is really very early stuff and is called from here:
    http://sdcc.svn.sourceforge.net/viewvc/sdcc/trunk/sdcc/device/lib/mcs51/crtstart.asm?view=markup
 */
unsigned char _sdcc_external_startup(void)
{
    // watchdog?
    // ports to HiZ?
    // kick PLL?

    PCON2 |= 0x11;      /**< Enable external space write. Enable idle loop no fetching instr. */
    XBICFG = 0x64;      /**< as dumped by ec-dump.fth */
    XBICS |= 0x30;      /**< bit 5 as dumped by ec-dump.fth, Enable Reset 8051 and XBI Segment Setting */
    SPICFG |= 0x04;     /**< bit 2 as dumped by ec-dump.fth */
    WDTCFG = 0x48;      /**< disable watchdog for now */
    LPCFWH = 0xa0;      /**< as dumped by ec-dump.fth */
    LPCCFG = 0xfd;      /**< as dumped by ec-dump.fth */
    CLKCFG = 0x94;      /**< as dumped by ec-dump.fth */

    // LPC_2EF is marked as internal use only but deviates from reset value

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
    /* LED CHG G# */
    GPIOEOE0 |= 0x08;

    /* LED CHG R# */
    GPIOOE08 |= 0x04;

    /* LED PWR# */
    GPIOOE08 |= 0x02;

    /* LED Light# / EC_EAPD??? */
//    GPIOOE08 |= 0x01;

    /* for now?  */
    LED_CHG_G_ON;
    LED_CHG_R_ON;
    LED_PWR_ON;
}

//! off-load blinking (and off-after-a-while) stuff to non IRQ
void handle_leds(void)
{
    switch ((unsigned char)second & 0x03 )
    {
        case 0: LED_CHG_R_ON;
                break;
        case 1: LED_CHG_G_ON;
                break;
        case 2: LED_CHG_R_OFF;
                break;
        case 3: LED_CHG_G_OFF;
                break;
    }
}

//! Entering low power mode
/*! IRQs should wake us again
 */
#define SLEEP() do \
{ \
    PCON |= 0x01; \
    /* maybe: PMUCFG = 0x53; */ \
} while(0)


//! output a char every second
void debug_uart_ping(void)
{
    static unsigned char __xdata my_s;

    if( my_s != (unsigned char)second )
    {
        my_s = (unsigned char)second;

        SBUF = '*';   // For now simply write without caring for TI...
    }
}

void handle_debug(void)
{
    static unsigned char __pdata my_game_key_status[2];

    /* does current state of this bit differ from the state that was last seen */
    if( (cursors.game_key_status[0] ^ my_game_key_status[0]) & 0x02 )
    {
        /* track that bit */
        my_game_key_status[0] ^= 0x02;
        if( my_game_key_status[0] & 0x02)
        {
            dump_mcs51_sfr();
            dump_xdata_sfr();
        }
    }
}

void startup_message(void)
{
    putstring("\r\nHello World!\r\n");
    putstring(name_string);
    putspace();
    putstring(version_string);
    /* Silicon Revision */
    putstring(" Mask(");
    puthex(ECHV);
    /* Stack pointer */
    putstring(") SP(");
    puthex(SP);
    putstring(") ");
    putstring(date_string);
    putspace();
    putstring(time_string);
}


//! You expected it: This routine is expected never to exit
void main (void)
{
    port_init();
    watchdog_init();
    timer_gpt3_init();
    cursors_init();
    uart_init();

    startup_message();

    dump_mcs51_sfr();
    dump_xdata_sfr();

    print_states_ruler();
    print_states();
    save_old_states();
    states.number = 0;

    power_init();
    LED_CHG_G_OFF;
    LED_CHG_R_OFF;
    LED_PWR_OFF;

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
//        busy |= handle_battery();
        handle_leds();

//        debug_uart_ping();

        watchdog_all_up_and_well |= WATCHDOG_MAIN_LOOP_IS_FINE;

        print_states();

        handle_power();

        handle_debug();

        if( !busy && may_sleep )
            SLEEP();
    }
}


