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
#include "uart.h"
#include "watchdog.h"

/* see also: http://www.ibutton.com/
 */

//! data from battery sensor
/*! adapt to Maxim/Dallas DS2756 */
struct data_ds2756_type __xdata data_ds2756;

static unsigned char __xdata debug_ds2756_printed;

//! Handling the ds2756 is probably among the more tricky parts
/*! It:
     - has hard realtime requirements.
       (too long to nicely busy wait, too short to really tolerate other IRQ)
     - updates its internal registers asynchronously (in intervals
       of 4/88/220/2800 milliseconds)
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
    static unsigned char __xdata my_timer = 0xff;

    unsigned char state = state_expensive;

    switch (state)
    {
        case 0:
            data_ds2756.error.no_device_flag_is_invalid = 1;

            /* we do not know for sure but this is a good default */
            data_ds2756.error.no_device = 1;

            /* skipping wait - try soon */
            state = 2;
            break;

        case 1:
            /* battery not or not yet found */

            /* here? */
            watchdog_all_up_and_well |= WATCHDOG_ONE_WIRE_IS_FINE;

            /* only start a new try every xx seconds */
            if (0xfc & ((unsigned char)second ^ my_timer) )
            {
                state++;
            }
            break;

        case 2:
            my_timer = (unsigned char)second;
            debug_ds2756_printed = 0;
            ow_transfer_buf[0] = 0x33;
            ow_transfer_init( 1, 8 );
            state++;
            break;

        case 3:
            if(!ow_busy())
            {
                if( !data_ds2756.error.no_device )
                {
                    unsigned char i;

                    /* copy serial number */
                    for( i = 0; i < sizeof data_ds2756.serial_number; i++ )
                    {
                         data_ds2756.serial_number[i] = ow_transfer_buf[i+1];
                    }
                    state++;
                }
                else
                {
                    /* try again once in a while */
                    state = 1;
                }
                data_ds2756.error.no_device_flag_is_invalid = 0;
            }
            break;

        case 4: /* do a CRC check of the ID */
            {
                unsigned char crc = 0;
/*
                unsigned char i;
                unsigned char __code mask[] = { 0x01,0x02,0x04,0x08, 0x10,0x20,0x40,0x80 };

                for( i = 0; i < 56; i++ )
                {
                    if( ow_transfer_buf[i/8] & mask[i&0x07] )
                        ...
                }

*/

                if( 1 /* crc == ow_transfer_buf[7] */ )
                {
                    data_ds2756.error.crc_fail = 0;
                    data_ds2756.serial_number_valid = 1;

                    /* check One-wire device ID */
                    if( ow_transfer_buf[0] == 0x35 )
                    {
                        state++;
                    }
                    else
                    {
                        state = 20;
                    }
                }
                else
                {
                    data_ds2756.error.crc_fail = 1;
                    /* try again once in a while */
                    state = 1;
                }
            }
            break;

        case 5:
            /* now ready to process commands from either
               battery.c or host command */

            /* host can easily issue commands more quickly
               than we can serve them and thus starve the
               battery state-machine.
               So handle commands from battery first
               (or see that at least some get handled) */

            /* here: ? */
            watchdog_all_up_and_well |= WATCHDOG_ONE_WIRE_IS_FINE;

            if( data_ds2756.host_transfer.request_new )
            {
                state = 10;
            }
            else
            {
                // for now!! :
                if (0xfe & ((unsigned char)second ^ my_timer) )
                {
                    my_timer = (unsigned char)second;
                    state++;
                }
            }

            break;

        case 6:
            ow_transfer_buf[0] = 0xcc;  /* skip net address */
            ow_transfer_buf[1] = 0x69;  /* read */
            ow_transfer_buf[2] = 0x0c;  /* address */
            ow_transfer_init( 3, 6 );

            state++;

            break;

        case 7:

            if(!ow_busy())
            {
                if( !data_ds2756.error.no_device )
                {
                    unsigned int u;

                    u = *(unsigned int *)&ow_transfer_buf[0];
                    data_ds2756.voltage_raw = (u>>8) | (u<<8);

                    u = *(unsigned int *)&ow_transfer_buf[2];
                    data_ds2756.current_raw = (u>>8) | (u<<8);

                    u = *(unsigned int *)&ow_transfer_buf[4];
                    data_ds2756.charge_raw = (u>>8) | (u<<8);

                    state++;
                }
                else
                    state = 1;
            }
            break;

        case 8:
            ow_transfer_buf[0] = 0xcc;  /* skip net address */
            ow_transfer_buf[1] = 0x69;  /* read */
            ow_transfer_buf[2] = 0x18;  /* address */
            ow_transfer_init( 3, 4 );

            state++;

            break;

        case 9:

            if(!ow_busy())
            {
                if( !data_ds2756.error.no_device )
                {
                    unsigned int u;

                    u = *(unsigned int *)&ow_transfer_buf[0];
                    data_ds2756.temp_raw = (u>>8) | (u<<8);
                    u = *(unsigned int *)&ow_transfer_buf[2];
                    data_ds2756.avg_current_raw = (u>>8) | (u<<8);

                    if( !debug_ds2756_printed )
                    {
                        debug_ds2756_printed = 1;
                        dump_ds2756();
                    }
                    state = 5;
                }
                else
                    state = 1;
            }
            break;

        case 10:
            {
                unsigned char i;
                unsigned char t = data_ds2756.host_transfer.TX_len;
                for( i = 0; i < t; i++)
                    ow_transfer_buf[i] = data_ds2756.host_transfer.buf[i];

                data_ds2756.host_transfer.request_completed = 0;
                ow_transfer_init( t, data_ds2756.host_transfer.RX_len );
            }
            state++;
            break;

        case 11:

            if(!ow_busy())
            {
                unsigned char i;
                unsigned char r = data_ds2756.host_transfer.RX_len;
                for( i = 0; i < r; i++)
                    data_ds2756.host_transfer.buf[i] = ow_transfer_buf[i];

                data_ds2756.host_transfer.request_new = 0;
                data_ds2756.host_transfer.request_completed = 1;

                state = 5;
            }
            break;

        case 20:
            /* unknown device */
            /* cannot handle it but should maybe we should allow host to read registers? */

            /* for now: */
            state = 1;
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


void dump_ds2756()
{
    unsigned char i;

    putstring("\r\nSer:");
    for( i = 0; i < sizeof data_ds2756.serial_number; i++ )
        puthex(data_ds2756.serial_number[i]);

    putstring(" U_raw:");
    puthex(data_ds2756.voltage_raw>>8);
    puthex(data_ds2756.voltage_raw);

    putstring(" I_raw:");
    puthex(data_ds2756.current_raw>>8);
    puthex(data_ds2756.current_raw);

    putstring(" I_avg_raw:");
    puthex(data_ds2756.avg_current_raw>>8);
    puthex(data_ds2756.avg_current_raw);

    putstring(" Q_raw:");
    puthex(data_ds2756.charge_raw>>8);
    puthex(data_ds2756.charge_raw);

    putstring(" T_raw:");
    puthex(data_ds2756.temp_raw>>8);
    puthex(data_ds2756.temp_raw);

    putstring(" E:");
    puthex(data_ds2756.error.c);
}


bool dump_ds2756_all()
{
    unsigned char i,k;

    if( ow_busy() ) // hmmm, racy, we still could take over another transfer...
        return 0;

    for( i = 0; i < 0x8f; i += 8 )
    {
        putstring("\r\nDS2756 ");
        puthex(i);
        putchar(':');

        ow_transfer_buf[0] = 0xcc;  /* skip net address */
        ow_transfer_buf[1] = 0x69;  /* read */
        ow_transfer_buf[2] = i;     /* address */
        ow_transfer_init( 3, 8 );

        while( ow_busy() )
            ;

        for( k = 0; k<8; k++ )
        {
            if( k == 4 )
                putspace();
            putspace();
            if( data_ds2756.error.no_device )
                putstring("--");
            else
                puthex( ow_transfer_buf[k] );
        }
    }

    return 1;
}
