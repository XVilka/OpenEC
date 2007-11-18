/*-------------------------------------------------------------------------
   adc.c - handle the ADC on the EC

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
#include "uart.h"

unsigned char __xdata board_id;
unsigned char __xdata adc_cache[4]; /* 3 */

static __bit adc_interrupt_flag_remove_me;


//! store the result and select next channel
/*! Wait for someone else to start ADC conversion.
    Actually temperature sensor and board ID will
    be quite boring.
    If we read out the temperature sensor say ten
    times quicker than its time constant then this
    likely is more than enough.

    How many uJ does a conversion take?^)
 */
void adc_interrupt(void) __interrupt(0x1f)
{
    unsigned char t,d;

    d = ADCDAT;
    t = ADCTRL;
    adc_cache[(t>>2) & 0x03] = d;
    t = t + 0x04;               /**< switch to next channel */
    if( t >= (3 * 0x04) )       /**< allow 3 channels */
        t = 0;
    ADCTRL = t;

   /* Reset pending flag */
    P3IF &= ~0x80;

    adc_interrupt_flag_remove_me = 1;
}


void adc_init(void)
{
    /* enable selected channels */
    ADDAEN = 0x07;

    /* Reset pending flag */
    P3IF &= ~0x80;

    /* enable IRQ */
    P3IE |= 0x80;

    /* start conversion on ADC1 */
    ADCTRL = 0x05;
}



//--------8<------------------------------------------------------------------
// The stuff below should probably go into a separate file.
// Currently it does not really hurt here, but it is no clean
// cut between low level and medium level code.
// On the other hand having many files is not nice either.
// Should there be board_id.c and board_id.h ?

void get_board_id(void)
{
    unsigned int i;
    unsigned char adc;
    unsigned char __code board_id_table[16] =
    {
        0xff,
        0xb3,  0xff,
        0xb4,  0xff,
        0x00,  0xff,
        0x01,  0xff,
        0xb2,  0xff,
        0x02,  0xff,
        0x03,  0xff,
        0x04,
    };

    /* from http://wiki.laptop.org/go/Ec_specification 20070901
        00-07h  B3
        08-17h          guard band
        18-27h  B4
        28-37h          guard band
        38-47h  N
        48-57h          guard band
        58-67h  N + 1
        68-77h          guard band
        78-87h  B2
        88-97h          guard band
        98-A7h  N + 2
        A8-B7h          guard band
        B8-C7h	N + 3
        C8-D7h          guard band
        D8-E7h  N + 4
        E8-F7h          guard band
        F8-FFh  N + 5
    */

    for( i = 0; i != 0x3fff; i++ )
    {
        if( adc_interrupt_flag_remove_me )
            break;
    }

    adc = adc_cache[1];

    if( adc >= 0xf8 )
        board_id = 0x05;
    else if( adc >= 0xe8 )
        board_id = 0xff;
    else
        board_id = board_id_table[ (unsigned char)(adc + 0x08) / 0x10 ];

    /* putspace(); puthex_u16(i); */
}


//! assumes board_id is properly set
unsigned char __code * board_id_to_string(void)
{
   switch( board_id )
   {
      case 0xb1: return "B1";
      case 0xb2: return "B2";
      case 0xb3: return "B3";
      default: return "??";
   }
}
