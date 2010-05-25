/*-------------------------------------------------------------------------
   failsafe.c - fail-safe startup code for the kb3700 on the XO

   Copyright (C) 2007  Frieder Ferlemann <Frieder.Ferlemann AT web.de>

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
   USA

   Linking ABC statically or dynamically with other modules is making
   a combined work based on ABC. Thus, the terms and conditions of the
   GNU General Public License cover the whole combination.

   In addition, as a special exception, the copyright holders of ABC
   give you permission to combine ABC program with free software
   programs or libraries that are released under the GNU LGPL and with 
   code included in the standard release of DEF under the XYZ license
   (or modified versions of such code, with unchanged license).
   You may copy and distribute such a system following the terms of the
   GNU GPL for ABC and the licenses of the other code concerned,
   provided that you include the source code of that other code when
   and as the GNU GPL requires distribution of source code.

   Note that people who make modified versions of ABC are not obligated
   to grant this special exception for their modified versions; it is
   their choice whether to do so. The GNU General Public License gives
   permission to release a modified version without this exception;
   this exception also makes it possible to release a modified version
   which carries forward this exception.

   (This license text is a cut and paste from:
   http://www.fsf.org/licensing/licenses/gpl-faq.html#GPLIncompatibleLibs
   (2007-08-06) with only white-space changes.
   "ABC" denotes failsafe.c, "DEF" and "XYZ" would yet have to be defined)
-------------------------------------------------------------------------*/

/* This (want to be) fail-safe startup code is meant to be used for
   both openec and proprietary EC code within the OLPC project.

   We'll have to find a license that allows for this, prohibits an
   "embrace an extend" takeover from proprietary code, protects its
   authors from being sued, and socially is a good aproach for OLPC.
   Also as there are a few GB of GPL'ed software we cannot afford to
   loose the option to make use of GPL'ed software.

   Last time I looked I was not a native english speaker. I'm pretty
   sure that I'm not a lawyer either and that I'm not familiar with
   licensing within different countries (to be honest not even my own)
   so I am unlikely to be able to invent a proper license. Or even to
   be likely to recognise a proper one.
   Still less likely a license that would work in the long run
   (allows for a solid growth model). What is clear to me is, that I
   want this code to be called directly by hardware reset (as on power
   up or reset input) and that this may not be hidden from the user
   and that the code this code jumps to (jumps in the sense of
   (mcs51) ljmp, ajmp, sjmp and not in the sense of lcall, acall)
   may be proprietary code.
 */


#include "chip.h"                             /* adapt license there too */


#define RUNNING_FROM_BATTERY (0)                /**< fix */
#define ADDR_TO_XBISEGn_SETTING(address) (0x80| (address) / 0x4000)

//! There is a gap withing the table 4.6.2 "SPI Register Descriptions"
/*! The comment for XBICS bit 4 mentions the existence of
    XBI registers A0-A3h(bank select)
 */
SFRX(XBISEG2,   0xfea2);
SFRX(XBISEG3,   0xfea3);


char __code timestamp[] = __FILE__ " " __DATE__ " " __TIME__;


//! bank switching routine
void switch_to_segment_0x10000(void) __naked
{
    __asm

        .area SW_ACSEG (CODE,ABS)
        .org 0x01e3

        ; pushing the address of the reset vector (0x0000) onto stack
        ; (just in case)
        clr     a
        push    acc
        push    acc

        ; remapping Segment 3 (0xc000-0xffff)
        ; from 0x00c000 to 0x01c000
        mov     dptr,#_XBISEG3
        mov     a,#ADDR_TO_XBISEGn_SETTING( 0x01c000 )
        movx    @dptr,a

        ; remapping Segment 2 (0x8000-0xbfff)
        mov     dptr,#_XBISEG2
        mov     a,#ADDR_TO_XBISEGn_SETTING( 0x018000 )
        movx    @dptr,a

        ; remapping Segment 1 (0x4000-0x7fff)
        mov     dptr,#_XBISEG1
        mov     a,#ADDR_TO_XBISEGn_SETTING( 0x014000 )
        movx    @dptr,a

        ; remapping Segment 0 (0x0000-0x3fff)
        ; (the segment this code runs in)
        mov     dptr,#_XBISEG0
        mov     a,#ADDR_TO_XBISEGn_SETTING( 0x010000 )

        .org 0x1ff
        ; fasten seatbelts:
        movx    @dptr,a

        ; execution now continues in a different
        ; bank at address 0x0200.
        ; Expecting ljmp/ajmp 0x0000 there.
        ;

        ; this instruction should never be reached:
        ret

    __endasm;
}


//! this routine might disappear
/*! it tests for some cookies which are
    expected to be at 0x014000, 0x018000, 0x01c000 */
void test_remapping(void)
{
   volatile unsigned char __xdata __at(0xfbff) num_fail; /**< topmost memory location */
   unsigned char tmp[4];

   #define COOKIE_0 (0x02) /* not a typo */
   #define COOKIE_1 (0x01)
   #define COOKIE_2 (0x02)
   #define COOKIE_3 (0x03)

   num_fail = 0;

   tmp[0] = *(unsigned char __xdata *)0x0000;
   tmp[1] = *(unsigned char __xdata *)0x4000;
   tmp[2] = *(unsigned char __xdata *)0x8000;
   tmp[3] = *(unsigned char __xdata *)0xc000;

   // does XBISEG1 work as expected?
   // mapping different banks to 0x4000..0x7fff
   XBISEG1 = ADDR_TO_XBISEGn_SETTING( 0x010000 );
   if( *(volatile unsigned char __xdata *)0x4000 != COOKIE_0 )
       num_fail++;
   XBISEG1 = ADDR_TO_XBISEGn_SETTING( 0x014000 );
   if( *(volatile unsigned char __xdata *)0x4000 != COOKIE_1 )
       num_fail++;
   XBISEG1 = ADDR_TO_XBISEGn_SETTING( 0x018000 );
   if( *(volatile unsigned char __xdata *)0x4000 != COOKIE_2 )
       num_fail++;
   XBISEG1 = ADDR_TO_XBISEGn_SETTING( 0x01c000 );
   if( *(volatile unsigned char __xdata *)0x4000 != COOKIE_3 )
       num_fail++;
   XBISEG1 &= ~0x80;
   if( *(volatile unsigned char __xdata *)0x4000 != tmp[1] )
       num_fail++;


   // do XBISEG2 and XBISEG3 work as expected?
   // swapping them to ensure it is working
   XBISEG3 = ADDR_TO_XBISEGn_SETTING( 0x018000 );
   if( *(volatile unsigned char __xdata *)0xc000 != 0x02 )
       num_fail++;

   XBISEG2 = ADDR_TO_XBISEGn_SETTING( 0x01c000 );
   if( *(volatile unsigned char __xdata *)0x8000 != 0x03 )
       num_fail++;

   // mapping the banks where we want to have them later
   XBISEG3 = ADDR_TO_XBISEGn_SETTING( 0x01c000 );
   if( *(volatile unsigned char __xdata *)0xc000 != 0x03 )
       num_fail++;

   XBISEG2 = ADDR_TO_XBISEGn_SETTING( 0x018000 );
   if( *(volatile unsigned char __xdata *)0x8000 != 0x02 )
       num_fail++;


   /* switching off remapping again */
   XBISEG3 &= ~0x80;
   if( *(volatile unsigned char __xdata *)0xc000 != tmp[3] )
       num_fail++;

   XBISEG2 &= ~0x80;
   if( *(volatile unsigned char __xdata *)0x8000 != tmp[2] )
       num_fail++;

   XBISEG1 &= ~0x80;
   if( *(volatile unsigned char __xdata *)0x4000 != tmp[1] )
       num_fail++;

}


//! setting up GPIO so we can detect key presses
void matrix_3x3_init()
{
    /* KEY_IN_1 .. KEY_IN_3 are inputs */
    GPIOOE00 &= ~0x70;

    /* set KEY_OUT_1 KEY_OUT_2, KEY_OUT_3 Open Drain Enable? */
    GPIOOD10 |= 0x70;

    /* let KEY_OUT_1 .. KEY_OUT_3 be high */
    GPIOD10 |= 0x70;

    /* set KEY_OUT_1 to output */
    GPIOOE10 |= 0x40;
}


//! setting up GPIO so we can detect key presses
void matrix_3x3_select_column()
{
    /* drive KEY_OUT_1 low. Eventually not a good idea
       to use this column of the matrix because its
       full name is KEY_OUT_1/ISP_EN# 
       Which one to use best? */
    GPIOD10 &= ~0x40;

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
       lines to be discharged.
     */
    {
        volatile char counter = 10;
        while( --counter )
            ;
    }
}



//! switch on DCON (Display CONtroler)
void dcon_enable()
{
    // GPIOED
}

void wlan_enable()
{
    // GPIO01
}

void main_on()
{
    // GPIO1A
}


void main (void)
{
    /* makes no sense to run recovery mode on battery */
    if( !RUNNING_FROM_BATTERY )
    {

        test_remapping();

        matrix_3x3_init();
        matrix_3x3_select_column();

        /* now checking for if KEY_RT_L, KEY_LF_L and KEY_DN_L
           are simultanuously pressed. */
        if( 0x00 == (GPIOIN10 & 0x70))
        {

            /* switch all IO ports to failsafe state */

            /* switching on enough to allow the XO to boot */
            main_on();
            dcon_enable();
            wlan_enable(); /**< sure? */

            /* switch LED */

            /* going into an (endless!?) loop waiting
               for the XO giving us a reset.
               We probably should never exit by our own
               as a flash write might be in progress.
             */
            while( 1 )
            {
                /* reset watchdog pending flags */
                WDTPF = 0x03;

                /* signal to the host and user that
                   we are here and are alive */

                /* if the XO does not forbid us
                   within xx seconds then eventually wait
                   for input data (UART?, DQ?) and write
                   that to flash?
                   Probably not without checking:
                   http://wiki.laptop.org/go/OLPC_Bitfrost
                 */
            }

            /* switch LED */
        };
    }

    switch_to_segment_0x10000();
}
