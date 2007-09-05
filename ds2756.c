/*-------------------------------------------------------------------------
   ds2756.c - handle Maxim/Dallas DS2756

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

/* 
   Status: committed "as is".
 */

#include <stdbool.h>
#include "kb3700.h"
#include "ds2756.h"
#include "one_wire.h"
#include "states.h"
#include "timer.h"

/* see also: http://www.ibutton.com/
 */

//! data from battery sensor
/*! adapt to Maxim/Dallas DS2756 */
struct data_ds2756_type __xdata data_ds2756;


//! Handling the ds2756 is probably among the more tricky parts
/*! It:
     - has hard realtime requirements.
       (too long to nicely busy wait, too short to really tolerate other IRQ)
     - updates its internal registers asynchronously (in intervals
       of 4/88/2800 milliseconds)
     - needs to take commands from either host communication or
       from battery state machine
     - is used for measuring 4 different physical quantities
       (voltage, current, capacity, temperature)
     - is picky: if the wrong register is written it might switch
       its low level protocol (from normal to overdrive mode)
       or permanently lock memory regions.
     - it's an external part (so it might or might not be there
       and it might be removed/added/exchanged on the fly)

     So this routine interfaces to:
     - host communication routine (transfer in both directions)
     - battery routine (NiMH/LiFePo4) (transfer in both directions)
     - an "interrupt chain" handling bytewise IO

     This routine does _not_ try to be clever. Appart from establishing
     communication to the ds2756 it does nothing on its own.
     In particular it does no caching, summing, averaging or read ahead.

     In the first phase of development this routine might protect some
     registers from being written but ideally _this_complete_file_
     would not have to be changed if any of the ds2756 registers changes
     its meaning.
*/
bool handle_ds2756(void)
{
    static unsigned char __pdata state_expensive = 0;
    static unsigned char __xdata second_last_presence_detect = 0xff;

    unsigned char state = state_expensive;

    switch (state)
    {
        case 0:
            data_ds2756.error.no_device_flag_is_invalid = 1;

            /* we do not know for sure but this is a good default */
            data_ds2756.error.no_device = 1;

            /* should be high anyway.. */
//            DQ_HIGH();

            /* skipping wait - try soon */
            state = 2;
            break;

        case 1:
            /* only start a new try every xx seconds */
            if ( 0xfc & (second_last_presence_detect ^ (unsigned char)second) )
            {
                state++;
            }
            break;

        case 2:
            second_last_presence_detect = (unsigned char)second;
            ow_reset_and_presence_detect_init();
            state++;
            break;


        case 3:
            if(!ow_busy())
            {
                if( !data_ds2756.error.no_device )
                {
                    /* fine! Reset previous CRC error */
                    data_ds2756.error.crc_fail = 0;

                    state++;
                }
                else
                {
                    /* back to previous state, try again once in a while */
                    state--;
                }
               data_ds2756.error.no_device_flag_is_invalid = 0;
            }
            break;


   //     case 2: get id...

        case 4: /* should be get ID 
                   for now simply output something */
            ow_write_byte_init(0x12);
            state++;
            break;

        case 5:
            if(!ow_busy())
            {
                if( 1 /* not_yet_six_bytes*/ )
                    state--;
                else
                {
                    if (1)
                    {
                        /* ID CRC not ok, retry soon */
                        state = 2;
                    }
                    else
                    {
                        /* CRC ok */
                        data_ds2756.serial_number_valid = 1;
                        state++;
                    }
                }
            }
            break;

        case 6:
            /* now ready to process commands from either
               battery.c or host command */

            /* host can easily issue commands more quickly
               than we can serve them and thus starve the
               battery state-machine.
               So handle commands from battery first
               (or see that at least some get handled) */
            break;

        default:
            /* internal error */
            state = 0;

   }

   /* write back to the static variable (__xdata or __pdata)
    */
   state_expensive = state;

   /* and to debugging area (if enabled) */
   STATES_UPDATE(ds2756, state);

   return 0;
}
