/*-------------------------------------------------------------------------
   matrix_3x3.c - routines to handle 3x3 matrix within the OLPC project

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
#include "states.h"
#include "timer.h"
#include "matrix_3x3.h"

#define UPDATE_GAME_KEY_STATUS 1

//! bit mask for keys within game_key_status
#define KEY_LF_R  0x02
#define KEY_RT_R  0x04
#define KEY_UP_L  0x08

#define KEY_DN_L  0x10
#define KEY_LF_L  0x20
#define KEY_RT_L  0x40

#define KEY_COLOR 0x80
#define KEY_UP_R  0x01  /* interpret as 0x0100 */
#define KEY_DN_R  0x02  /* interpret as 0x0200 */


//! keeps the externally visible data to handle the 3x3 matrix
struct cursors __pdata cursors;

//! keeps the not externally visible data to handle the 3x3 matrix
static struct
{
    /* stores (low byte of) tick the scanning routine was last called
       matrix should not be scanned to quickly for two reasons:
       debouncing, and not using more CPU cycles than needed.
     */
    unsigned char ctick;

    /* column of the matrix
       schematic names them 1..3. This variable counts 0..2 
     */
    unsigned char column;

    /* a mirror of the keyboard matrix.
       Not as it is but rather the state we have reacted upon 
     */
    unsigned char row[3];

    /* used for debouncing.
       lower nibble holds previous reading,
       higher nibble might hold the one before 
     */
    unsigned char row_raw[3];
} __pdata cursors_private;



void cursors_init( void )
{
    /* KEY_OUT_n is not input */
    GPIOIE10 &= ~0xe0;

    /* KEY_OUT_n default is high */
    GPIOD10 |= 0xe0;

    /* KEY_OUT_n open drain enable. Warning KEY_OUT_3 is also used for ISP_CLK
       so if the serial debricking adapter is connected output drivers work
       against each others with a duty cycle of 1/6 */
    GPIOOD10 |= 0xe0;

    /* KEY_OUT_n enable pull up */
    GPIOPU10 |= 0xe0;

    /* KEY_OUT_n is output */
    GPIOOE10 |= 0xe0;

    /* KEY_IN_n is input */
    GPIOEIN0_0xfc64 |= 0x70;  /* avoiding name clash, GPIOEIN0 */
}



#ifdef SDCC
# pragma callee_saves debug_toggle
#endif

//! toggle pin KEY_OUT3 available at ISP header PIN 5
void debug_toggle(void)
{
    #if defined(SDCC) && 1
    __asm
        mov   dptr,#_GPIOD10
        movx  a,@dptr
        push  acc      ; save previous state

        setb  acc.5    ; set to high case it was low before
        movx  @dptr,a  ; and write
        clr   acc.5    ; clear to give a short trigger pulse
        movx  @dptr,a  ; and write
        setb  acc.5
        movx  @dptr,a

        pop   acc      ; restore previous state
        movx  @dptr,a
    __endasm;
    #endif
}


//! Handles input from 3x3 matrix
/*! see also http://wiki.laptop.org/index.php?title=Ec_specification
 *
 * This routine only reacts on a single key change.
 * If more than one key is changed (can you really
 * press keys more quickly than this routine is called?)
 * then the other key changes are handled within the
 * next calls of this routine.
 *
 * copy and paste from the wiki (20070718) :
 *  Key matrix       	 Make code      	 Break code
 *  Bit 9: KEY_DN_R, 	{0xE0,0x66,0x00} 	{0xE0,0xE6,0x00}
 *  Bit 8: KEY_UP_R, 	{0xE0,0x65,0x00} 	{0xE0,0xE5,0x00}
 *  Bit 7: KEY_COLOR/MONO {0x69,0x00,0x00} 	{0xE9,0x00,0x00}
 *  Bit 6: KEY_RT_L, 	{0x68,0x00,0x00} 	{0xE8,0x00,0x00}
 *  Bit 5: KEY_LF_L, 	{0x67,0x00,0x00} 	{0xE7,0x00,0x00}
 *  Bit 4: KEY_DN_L, 	{0x66,0x00,0x00} 	{0xE6,0x00,0x00}
 *  Bit 3: KEY_UP_L, 	{0x65,0x00,0x00} 	{0xE5,0x00,0x00}
 *  Bit 2: KEY_RT_R, 	{0xE0,0x68,0x00} 	{0xE0,0xE8,0x00}
 *  Bit 1: KEY_LR_R 	{0xE0,0x67,0x00} 	{0xE0,0xE7,0x00}
 *
 *  This code is untested and unlikely to contain no bugs.
 *
 *  Resources:
 *  0x01a1 byte code memory (0.6% of code memory),
 *  0      bytes data memory,
 *  0      bytes overlayable data memory,
 *  14     bytes pdata memory,
 *  xx/yy/zz cycles (best/typ/worst case)
 *
 *  \return TRUE if a change in the matrix is detected
 */

bool handle_cursors(void)
{
    unsigned char column;
    unsigned char in;
    unsigned char in_debounced;

    unsigned int keycode;
    unsigned char row;

    //! keycodes
    const unsigned int __code keyin1_make[3]  = {0x6800, 0x6500, 0xe066};
    const unsigned int __code keyin1_break[3] = {0xe800, 0xe500, 0xe0e6};
    const unsigned int __code keyin2_make[3]  = {0x6700, 0xe068, 0xe065};
    const unsigned int __code keyin2_break[3] = {0xe700, 0xe0e8, 0xe0e5};
    const unsigned int __code keyin3_make[3]  = {0x6600, 0xe067, 0x6900};
    const unsigned int __code keyin3_break[3] = {0xe600, 0xe0e7, 0xe900};

    //! columns 0,1,2 are on bit 6,7,5
    const unsigned char __code column_GPIOD10[3] = {~0x40,~0x80,~0x20};

    /* do not want to be called twice per tick */
    if( cursors_private.ctick == (unsigned char)tick )
        return 0;
    cursors_private.ctick = (unsigned char)tick;

    /* previous keycode still not transmitted to host? */
    if( cursors.keycode_updated )
    {
        /* Tempting: Try to send now. I think this should
           not be done here, because it would be a hack 
           (mixing two completely different functionalities). 
         */
        return 0;
    }

    /* read column number 0..2 into local variable */
    column = cursors_private.column;

    /* reading input, inverting, ignore Power_Button
       Note: this read would not need be valid if done
       immediately after selecting a new scan column.
       If previously held low the capacity of the scan
       lines (xxx pF?) needs to be charged via a
       10k pullup resistor so a time constant in the 
       order of microseconds is expected.
       (Will not be critical here but nevertheless:
       can someone send an oscillocope screenshot
       and/or measure capacity of KEY_IN_1..3?)
     */
    in = ((unsigned char)~GPIOEIN0 >> 4) & 0x07;

    /* do some debouncing.
       The way it is done here ensures that a "Make" transition
       is noticed immediately, whereas a "Break" condition needs
       three consequtive consistent readings to be noticed.
       Can the maximum bouncing time of the switches be specified?
     */
    in_debounced = in | 
                   cursors_private.row_raw[column] | 
                   ((cursors_private.row_raw[column]>>4)|cursors_private.row_raw[column]<<4);
    in_debounced &= 0x07;

    /* shift previous raw value into high nibble,
       low nibble gets the current value 
     */
    cursors_private.row_raw[column] = (cursors_private.row_raw[column] << 4) | in;

    /* row is used often, have a local copy */
    row = cursors_private.row[column];

    /* does it differ from what has been handled so far? */
    if (row ^ in_debounced)
    {
        /* find out which key has changed.
           Deliberately handling _only_ the first
           key where a change is detected. */
        if( (row ^ in_debounced) & 0x01 )
        {
            /* track the value */
            row ^= 0x01;
            /* and prepare code to be sent */
            if(row & 0x01)
                keycode = keyin1_make[column];
            else
                keycode = keyin1_break[column];

        }
        else if( (row ^ in_debounced) & 0x02 )
        {
            row ^= 0x02;
            if(row & 0x02)
                keycode = keyin2_make[column];
            else
                keycode = keyin2_break[column];
        }
        else /* if is neither needed nor wanted */
        {
            row ^= 0x04;
            if(row & 0x04)
                keycode = keyin3_make[column];
            else
                keycode = keyin3_break[column];
        }

        /* & 0x07 is paranoia. */   
        row &= 0x07;
        /* write back from register to struct */
        cursors_private.row[column] = row;

        /* pass data to struct for later transmission */
        cursors.keycode[0] = keycode>>8;
        cursors.keycode[1] = keycode;
        cursors.keycode[2] = 0x00; /* should be zero anyway, paranoia */


#if UPDATE_GAME_KEY_STATUS

        /* updating game_key_status.
           (sending keycodes _and_ handling game_key_status) */
        {
            if( column == 0 )
            {
                /* the bit positions as transmitted by port 0x6c command 0x1d
                   seem not a good match for the hardware. Doing a table lookup.
                   Expect some bugs here:
                 */
                const unsigned char __code mask[8] =
                {
                                                 0,
                                          KEY_RT_L,
                               KEY_LF_L           ,
                               KEY_LF_L | KEY_RT_L,
                    KEY_DN_L                      ,
                    KEY_DN_L |            KEY_RT_L,
                    KEY_DN_L | KEY_LF_L           ,
                    KEY_DN_L | KEY_LF_L | KEY_RT_L
                };

                /* using a temporary variable for the bit operations
                   so no intermediate values for cursors.game_key_status
                   can be seen.
                 */
                unsigned char tmp;

                tmp = cursors.game_key_status[0];
                tmp &= ~(KEY_RT_L | KEY_LF_L | KEY_DN_L);
                tmp |= mask[row];
                cursors.game_key_status[0] = tmp;
            }
            else if( column == 1)
            {
                const unsigned char __code mask[8] =
                {
                                                 0,
                                          KEY_UP_L,
                               KEY_RT_R           ,
                               KEY_RT_R | KEY_UP_L,
                    KEY_LF_R                      ,
                    KEY_LF_R |            KEY_UP_L,
                    KEY_LF_R | KEY_RT_R           ,
                    KEY_LF_R | KEY_RT_R | KEY_UP_L
                };
                unsigned char tmp;

                tmp = cursors.game_key_status[0];
                tmp &= ~(KEY_UP_L | KEY_RT_R | KEY_LF_R);
                tmp |= mask[row];
                cursors.game_key_status[0] = tmp;
            }
            else
            {
                const unsigned char __code mask[4] =
                {
                                      0,
                               KEY_DN_R,
                    KEY_UP_R           ,
                    KEY_UP_R | KEY_DN_R,
                };
                unsigned char tmp;

                /* setting bit 7 accordingly */
                tmp = cursors.game_key_status[0];
                if(row&0x04)
                    tmp |= KEY_COLOR;
                else
                    tmp &= ~KEY_COLOR;
                cursors.game_key_status[0] = tmp;

                /* setting bit 8 and bit 9 accordingly
                  (and clearing bit 10..15) */
                cursors.game_key_status[1] = mask[row];
            }
        }

#endif

        /* News! Please transmit to host */
        cursors.keycode_updated = 1;
    }

    /* advance to next column */
    column++;
    if(column == 3)
       column = 0;

    /* write local variable into struct */
    cursors_private.column = column;

    /* and to debugging area (if enabled) */
    STATES_UPDATE(matrix_3x3, column | (cursors.keycode_updated?0x80:0x00));

    /* remove? */
//    debug_toggle();

    /* non atomic access. Unless we protect this, no IRQ might
       write to GPIOD10 */
    GPIOD10 = (GPIOD10 | 0xe0) & column_GPIOD10[column];

    return cursors.keycode_updated;

}

