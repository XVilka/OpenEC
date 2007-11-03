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
#include "battery.h"
#include "charge_sched.h"
#include "ds2756.h"
#include "led.h"
#include "one_wire.h"
#include "states.h"
#include "timer.h"
#include "uart.h"
#include "watchdog.h"

/* see also: http://www.ibutton.com/
 */

#define DEBUG 1
#if DEBUG && defined(SDCC)
# include <stdio.h>
# define PRINTF(...) printf_tiny(__VA_ARGS__)     /* printf is just here for debugging */
#else
# define PRINTF(...)
#endif


//! data from battery sensor
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
bool handle_ds2756_requests(void)
{
    static unsigned char __pdata state_expensive = 0;
    static unsigned char __xdata my_timer = 0xff;

    unsigned char state = state_expensive;

    switch (state)
    {
        case 0:
            /* here? */
            watchdog_all_up_and_well |= WATCHDOG_ONE_WIRE_IS_FINE;

            data_ds2756.error.no_device_flag_is_invalid = 1;

            /* we do not know for sure but this is a good default */
            data_ds2756.error.no_device = 1;

            /* skipping wait - try soon */
            state = 2;
            break;

        case 1: /* battery not or not yet found */

            /* here? */
            watchdog_all_up_and_well |= WATCHDOG_ONE_WIRE_IS_FINE;

            /* only start a new try every xx seconds */
            if ( (unsigned char)((unsigned char)second - my_timer) > 1 )
            {
                state++;
            }
            break;

        case 2:
            my_timer = (unsigned char)second;
            debug_ds2756_printed = 0;

            /* (re-)init hardware */
            ow_init();
            /* (re-)init timer */
            timer1_init();

            if( (data_ds2756.long_time_error.c ^ data_ds2756.error.c) & data_ds2756.error.c)
            {
                putstring("\r\nNew DS2756 E:");
                puthex(data_ds2756.error.c);
            }

            data_ds2756.long_time_error.c |= data_ds2756.error.c;
            data_ds2756.error.c = 0x00; /* clear all errors */
            data_ds2756.error.no_device = 1;
            data_ds2756.error.no_device_flag_is_invalid = 1;

            ow_transfer_buf[0] = 0x33;
            ow_transfer_init( 1, 8 );

            debug_ds2756_printed = 0;

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
                        BATT_LED_OFF();
                        BATT_LED_JINGLE_1s();
                    }
                    else
                    {
                        state = 15;
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

            if( data_ds2756.batt_transfer.request_new )
            {
                state = 8;
                break;
            }

            if( data_ds2756.host_transfer.request_new )
            {
                state = 6;
            }
            else
            {
                break;
            }
            /* intentionally no break here */


        case 6:
            {
                unsigned char i;
                unsigned char t = data_ds2756.host_transfer.TX_len;
                for( i = 0; i < t; i++)
                    ow_transfer_buf[i] = data_ds2756.host_transfer.buf[i];

                data_ds2756.host_transfer.request_completed = 0;
                data_ds2756.host_transfer.request_new = 0;

                ow_transfer_init( t, data_ds2756.host_transfer.RX_len );
            }
            state++;
            break;

        case 7:

            if(!ow_busy())
            {
                unsigned char i;
                unsigned char r = data_ds2756.host_transfer.RX_len;
                for( i = 0; i < r; i++)
                    data_ds2756.host_transfer.buf[i] = ow_transfer_buf[i];

                data_ds2756.host_transfer.error.c = data_ds2756.error.c;
                data_ds2756.host_transfer.request_completed = 1;

                if( data_ds2756.error.c )
                {
                    state = 10;
                }
                else
                    state = 5;
            }
            break;

        case 8:
            {
                unsigned char i;
                unsigned char t = data_ds2756.batt_transfer.TX_len;
                for( i = 0; i < t; i++)
                    ow_transfer_buf[i] = data_ds2756.batt_transfer.buf[i];

                data_ds2756.batt_transfer.request_completed = 0;
                data_ds2756.batt_transfer.request_new = 0;

                ow_transfer_init( t, data_ds2756.batt_transfer.RX_len );
            }
            state++;
            break;

        case 9:

            if(!ow_busy())
            {
                unsigned char i;
                unsigned char r = data_ds2756.batt_transfer.RX_len;
                for( i = 0; i < r; i++)
                    data_ds2756.batt_transfer.buf[i] = ow_transfer_buf[i];

                data_ds2756.batt_transfer.error.c = data_ds2756.error.c;
                data_ds2756.batt_transfer.request_completed = 1;

                if( data_ds2756.error.c )
                {
                    state = 10;
                }
                else
                    state = 5;
            }
            break;

        case 10:
            batt_led_jingle = HZ;
            BATT_LED_RED_BLINKING();
            my_timer = (unsigned char)second;
            state = 1;
            break;


        case 15:
            /* unknown device */
            /* cannot handle it but should maybe we should allow host to read registers? */
            batt_led_jingle = HZ;
            BATT_LED_ORANGE_BLINKING();
            my_timer = (unsigned char)second;

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


//! this one should be more clever
void set_batt_led_colour()
{
    unsigned int u = compare_is.U_mV;

    #define GUESSED_BATTERY_IMPEDANCE_mOHM 180
    u -= (long)compare_is.I_mA * GUESSED_BATTERY_IMPEDANCE_mOHM / 1000;

    if( data_ds2756.error.no_device )
    {
        BATT_LED_OFF();
    }
    else if( u < 5 * 1100)
    {
        BATT_LED_RED_BLINKING();
    }
    else if( u < 5 * 1150)
    {
        BATT_LED_RED();
    }
    else if( u < 5 * 1200)
    {
        BATT_LED_ORANGE();
    }
    else if( u < 5 * 1400)
    {
        BATT_LED_GREEN();
    }
    else
    {
        BATT_LED_GREEN_BLINKING();
    }
}


//! initiates readout of battery data
/*!
 */
bool handle_ds2756_readout(void)
{
    static unsigned char __pdata state;
    static unsigned char __xdata my_timer = HZ;
    static unsigned char __xdata buf[7];

    switch( state )
    {
        case 0:
            if( data_ds2756.serial_number_valid )
                 state = 1;
            break;

        case 1:
            if( (signed char)(my_timer - (unsigned char)tick) < 0 )
            {
                my_timer += HZ; /* next reading one second later? */

                buf[0] = 0xcc;  /* skip net address */
                buf[1] = 0x69;  /* read */
                buf[2] = 0x0c;  /* address */

                data_ds2756.batt_transfer.buf = buf;
                data_ds2756.batt_transfer.RX_len = 6;
                data_ds2756.batt_transfer.TX_len = 3;
                data_ds2756.batt_transfer.request_completed = 0;
                data_ds2756.batt_transfer.request_new = 1;
                data_ds2756.batt_transfer.error.c = 0x00;

                state = 2;
            }
            break;

        case 2:
            if( data_ds2756.batt_transfer.request_completed )
            {
                data_ds2756.batt_transfer.request_new = 0;
                if( !data_ds2756.batt_transfer.error.c )
                {
                    unsigned int u;

                    u = *(unsigned int *)&buf[0];
                    data_ds2756.voltage_raw = (u>>8) | (u<<8);

                    u = *(unsigned int *)&buf[2];
                    data_ds2756.current_raw = (u>>8) | (u<<8);

                    u = *(unsigned int *)&buf[4];
                    data_ds2756.charge_raw = (u>>8) | (u<<8);

                    state = 3;
                }
                else
                    state = 0;

            }
            break;

        case 3:

            buf[0] = 0xcc;  /* skip net address */
            buf[1] = 0x69;  /* read */
            buf[2] = 0x18;  /* address */

            data_ds2756.batt_transfer.buf = buf;
            data_ds2756.batt_transfer.RX_len = 4;
            data_ds2756.batt_transfer.TX_len = 3;
            data_ds2756.batt_transfer.request_completed = 0;
            data_ds2756.batt_transfer.request_new = 1;
            data_ds2756.batt_transfer.error.c = 0x00;

            state = 4;

            break;

        case 4:

            if( data_ds2756.batt_transfer.request_completed )
            {
                data_ds2756.batt_transfer.request_new = 0;
                if( !data_ds2756.batt_transfer.error.c )
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
                    state = 0;
            }
            break;

        case 5:
            compare_is.t_ms = get_time_ms();
            compare_is.U_mV = ds2756_raw_U_to_mV(data_ds2756.voltage_raw);
            compare_is.I_mA = ds2756_raw_I_to_mA(data_ds2756.current_raw);
            compare_is.Q_raw = data_ds2756.charge_raw;
            compare_is.T_cCelsius = ds2756_raw_T_to_cC(data_ds2756.temp_raw);

            set_batt_led_colour(); /* should be in battery.c */

            battery_news = 1;
            state = 6;
            break;

        case 6:
            state = 1;
            break;

    }
    return state >= 5;
}

//! fixed point conversion routine
/*! The ds2756 data on the EC could also be handled as raw values.
    (thus not needing a conversion after acquistion)
    But having the data in their "proper units" is better to maintain/debug.
    The conversion routines try to avoid 16 bit multiply
    and divide library functions but may have accuracy problems.
 */
int ds2756_raw_I_to_mA(int r)
{
#if 1

    return (int)((long)r * 1302 / 10000);

#else

    int j;
    unsigned char sign = 0;

    if( r < 0 )
    {
        r = -r;
        sign = 1;
    }
    // *0.1302(dec) is *0,001000010101010(bin)
    j  = (unsigned)r >> 3;

    j += (unsigned)r >> 8;

    j += (unsigned)r >> 10;

    if( sign )
        j = -j;

    return j;

#endif
}


//! difficult to handle if argument over-/underflows from 0x7fff to 0x8000
int ds2756_raw_Q_to_mAh(int r)
{
#if 1

    return (int)((long)r * 4167 / 10000);

#else

    unsigned int t;
    unsigned int q;
    unsigned char i;
    unsigned char sign = 0;

    if( r < 0 )
    {
        r = -r;
        sign = 1;
    }
    // *0.4167(dec) is *0,01101010101011001(bin)
    t = (unsigned)r >> 2;
    q = t;
    t >>= 1;
    q += t;
    i = 3;
    do
    {
        t >>= 2;
        q += t;
    } while(--i);

    if( sign )
        q = -q;

    return q;

#endif
}

unsigned int ds2756_raw_U_to_mV(unsigned int r)
{
#if 1

    return (int)((long)r * 3 / 10);

#else

    unsigned int u,t;
    unsigned char i;

    // *0.3(dec) is *0,010011001100110011...(bin)
    t = r >> 2;
    u = t;

    i = 3;
    do
    {
        t >>= 3;
        u += t;

        t >>= 1;
        u += t;
    } while(--i);

    return u;

#endif
}

//! converting to centi Celsius (1/100 degree Centigrade)
// OK below 0degC ?
unsigned int ds2756_raw_T_to_cC(signed int r)
{
    int t;
    unsigned char d;
    static unsigned char table[8] = { 0, 13, 25, 38, 50, 63, 75, 88 };

    // upper byte directly is degrees Celsius
    t = (r >> 8) * 100;

    // lower byte passed through table
    d = table[(unsigned char)r / 0x20];

    if( r<0 )
        return t - d;
    else
        return t + d;
}

void dump_ds2756()
{
    unsigned char i;

    if( data_ds2756.error.no_device )
        putstring("\r\nno batt");
    else
    {
        putstring("\r\nSer:");
        for( i = 0; i < sizeof data_ds2756.serial_number; i++ )
            puthex(data_ds2756.serial_number[i]);

        putstring(" U_raw:");
        puthex_u16(data_ds2756.voltage_raw);

        putstring(" I_raw:");
        puthex_u16(data_ds2756.current_raw);

        putstring(" I_avg_raw:");
        puthex_u16(data_ds2756.avg_current_raw);

        putstring(" Q_raw:");
        puthex_u16(data_ds2756.charge_raw);

        putstring(" T_raw:");
        puthex_u16(data_ds2756.temp_raw);

        putstring(" E:");
        puthex(data_ds2756.long_time_error.c);
        puthex(data_ds2756.error.c);

        PRINTF(" %umV",  ds2756_raw_U_to_mV(data_ds2756.voltage_raw));
        PRINTF(" %dmA",  ds2756_raw_I_to_mA(data_ds2756.current_raw));
        PRINTF(" %dmA",  ds2756_raw_I_to_mA(data_ds2756.avg_current_raw));
        PRINTF(" %dmAh", ds2756_raw_Q_to_mAh(data_ds2756.charge_raw));
        PRINTF(" %dcC", ds2756_raw_T_to_cC(data_ds2756.temp_raw));
    }
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
