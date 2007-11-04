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

     \subsection Compiler Compiler
      - sdcc 2.7.0 or later (needed), http://sdcc.sf.net
        (Compiler which generates the binary for the EC)
      - gcc (optional)
        (Compiler, the source is also compilable with GCC)

     \subsection Documentation Source Documentation
     - doxygen (optional), http://www.doxygen.org
       (Source documentation tool)
     - LaTeX (optional),
       (needed if a pdf version of the documentation should be generated.
       (a html version can be generated without LaTeX))

     \subsection Srecord Srecord
     - srecord (needed), http://srecord.sf.net
       (Handling of hex (etc.) files)

     \subsection Make Make
     - make (needed)
       (GNU make, should be there anyway.)

     \subsection Disassembler Disassembler
     - d52 (optional) http://www.8052.com/users/disasm/
       (Disassembler, generates disassembled file openec.d52)

     \subsection DownloadSoftware Download software
     - spiflash.dic (needed)  http://lists.laptop.org/pipermail/openec/2007-August/000061.html
       (Forth software to download the EC firmware to the target)

     \subsection Hardware Hardware adapter
     - serial adapter (needed) http://wiki.laptop.org/go/Image:Serial_adapter.jpg
       (Hardware adapter to download to target. Connects to CN24 and an RS232 null-modem cable
       (an additional oscillator is needed (66MHz(?) to CN24,Pin5)))
       Additional instructions are at:
       http://wiki.laptop.org/go/SPI_FLASH_Recovery
       (the instructions there address recovery of the complete flash)
     - Recovery Mode jumper block (see above)

     \subsection TerminalSoftware Terminal software
     - minicom (recommended)
       (parameters 115kBaud, 8N1, no handshake - other terminal programs can be used too)

     \subsection HardwareRev XO Hardware Revision
     - XO B1..By (needed)
      (XO Hardware Revision this code targets.)

   \section Related_Docu Related documentation
   http://wiki.laptop.org/go/Category.EC

   Publically available (but outdated) schematic:
   http://dev.laptop.org/attachment/ticket/477/SCHEMATIC1%20_%2012%20--%20EC%20KB3700.pdf

   Collection of Datasheets:
   ToBeDone

   \section Mailing_List Mailing list
   http://lists.laptop.org/pipermail/openec/

   \section Status Status
   very preliminary, <b>might DAMAGE HARDWARE! (no kidding)</b>
 */

#include <stdbool.h>
#include "kb3700.h"
#include "adc.h"
#include "battery.h"
#include "build.h"
#include "charge_sched.h"
#include "one_wire.h"
#include "ds2756.h"
#include "idle.h"
#include "led.h"
#include "matrix_3x3.h"
#include "manufacturing.h"
#include "monitor.h"
#include "one_wire.h"
#include "power.h"
#include "port_0x6c.h"
#include "sfr_dump.h"
#include "states.h"
#include "timer.h"
#include "uart.h"
#include "unused_irq.h"
#include "watchdog.h"

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
#if 1
    CLKCFG = 0x94;      /**< as dumped by ec-dump.fth */
#else
    CLKCFG = 0xd4;      /**< WARNING: setting bit 6 here is out of specification for
                             the Spansion S25FL008A. It is unstable here yet it worked long enough
                             so I could see that it results in a speed increase of about 46%
                             (unrepresentative benchmark).
                             If the XO waits on the EC the increased speed can translate
                             into powersavings */
#endif
    SPICFG = 0x00;      /**< bit 2 as or not as dumped by ec-dump.fth. Not sure how this bit should be set.
                             It relates to FAST_READ of the SPI flash. The clock frequency I'm seeing
                             at pin 6 of the flash (30MHz) would not require the FAST_READ mode.
                             So likely the dummy byte which is additionally transmitted for FAST_READ
                             can be avoided (40# instead of 48# for the first byte if address had to be set)
                             CLKCFG Bit6 would allow to select full/half speed (of internal clock
                             (66MHz+-25%)). 

                             48# at 66MHz are 727ns, 40# at 33MHz are 1212ns

                             */
    WDTCFG = 0x48;      /**< disable watchdog for now */
    LPCFWH = 0xa0;      /**< as dumped by ec-dump.fth */
    LPCCFG = 0xfd;      /**< as dumped by ec-dump.fth */

    PLLCFG = 0x95;      /**< 0x70 is reset default but results in an initial 
                             clock of about 29.3 MHz here. (this is too far off
                             from the expected 32 MHz for the serial 
                             communication too work)
                             0x80 gives about 30.9 MHz here.
                             0x95 is what ec-dump.fth shows */

    GPIOIE08 |= 0x20;   /**< enable external Reset input */
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

    /* all LED on on power up (for a short while) */
    LED_CHG_G_ON();
    LED_CHG_R_ON();
    LED_PWR_ON();
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
            dump_mcs51();
            dump_xdata_sfr();
        }
        cursors.keycode_updated = 0; // hack
    }

    /* does current state of this bit differ from the state that was last seen */
    if( (cursors.game_key_status[0] ^ my_game_key_status[0]) & 0x04 )
    {
        /* track that bit */
        my_game_key_status[0] ^= 0x04;
        if( my_game_key_status[0] & 0x04)
        {
            dump_gpio();
        }
        cursors.keycode_updated = 0; // hack
    }

}


void startup_message(void)
{
    putcrlf();
#ifdef __GNUC__
    putstring("GCC " __VERSION__ "\r\n");
#endif
    putstring(name_string);
    putspace();
    putstring(version_string);
    /* Silicon Revision */
    putstring(" Mask(");
    puthex(ECHV);
    /* ADC reading codes Board ID */
    putstring(") ID(");
    putstring(board_id_to_string());
    /* Stack pointer */
    putstring(") SP(");
    puthex(SP);
    putstring(") ");
    putstring(date_string);
    putspace();
    putstring(time_string);

    /* now for some manufacturing data */
    putcrlf();
    if( manufacturing_find_tag("T#") )
        manufacturing_print_tag();
    putspace();
    if( manufacturing_find_tag("SN") || manufacturing_find_tag("S#") )
        manufacturing_print_tag();
}


//! You expected it: This routine is expected never to exit
void main (void)
{
    port_init();
    watchdog_init();
    timer_gpt3_init();
    adc_init();
    cursors_init();
    power_init();

    uart_init();

    /* enable interrupts. */
    EA = 1;

    get_board_id();
    startup_message();

    dump_xdata_sfr();
    gpio_check_IO_direction();
    dump_gpio();
    dump_mcs51();

tx_drain();  // oops, UART routines seem not yet clean

    print_states_ruler();
    print_states_enable = 1;
    print_states();
    print_states_enable = 0;

    save_old_states();
    states.number = 0;

    manufacturing_print_all();

    ow_init();
    timer1_init();
    battery_charging_table_init();

    LED_CHG_G_OFF();
    LED_CHG_R_OFF();
    LED_PWR_OFF();

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
        handle_leds();
        handle_power();
        handle_ds2756_requests();
        handle_ds2756_readout();
        busy |= handle_battery_charging_table();

        watchdog_all_up_and_well |= WATCHDOG_MAIN_LOOP_IS_FINE;

        print_states();

        monitor();

        handle_debug();

        sleep_if_allowed();
    }
}


