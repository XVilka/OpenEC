/*-------------------------------------------------------------------------
   monitor.c - simple monitor

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

/*! \file monitor.c
   It's a hack. Adapt to what you need. Won't be in final firmware anyway:)

   Note, the monitor is implemented as a state-machine, so
   the main loop can still do its work.
 */

#include <stdbool.h>
#include <ctype.h>
#include "chip.h"
#include "adc.h"
#include "flash.h"
#include "reset.h"
#include "sfr_rw.h"
#include "sfr_dump.h"
#include "states.h"
#include "temperature.h"
#include "uart.h"

#define DEBUG 1
#if DEBUG && defined(SDCC)
# include <stdio.h>
# define PRINTF(...) printf_tiny(__VA_ARGS__)     /* printf is just here for debugging */
#else
# define PRINTF(...)
#endif

typedef enum 
{
    monitor_idle,
    command_m,
    command_M,
    command_set,
    command_and,
    command_or,
    command_error
} monitor_state;

static struct
{
    monitor_state state;
    unsigned int address_page;
    unsigned int address;
    unsigned int arg;
    unsigned char digits;
} __pdata m;


static unsigned char dump_address( unsigned int address, unsigned char area )
{
    unsigned char c = 0xff;

    switch( area )
    {
        case 0: /* idata/SFR */
            if( (unsigned char)address < 0x80 )
                c = *(unsigned char __idata *)address;
            else
                c = read_mcs51_sfr( (unsigned char)address );
            break;
        case 1: /* xdata/code */
            c = *(unsigned char __xdata *)address;
            break;
        case 2: /* code (anywhere in the SPI flash) */
            c = flash_read_byte(m.address_page, m.address);
            break;
    }
    puthex( c );
    return c;
}


static unsigned char get_next_digit( unsigned int __pdata *value, unsigned char c )
{
    static unsigned char __pdata my_digits;

    if( !c )
    {
       /* initialize */
       *value = 0x0000;
       my_digits = 0;
       return 0;
    }

    if( isxdigit( c ) )
    {
        putchar(c);
        *value <<= 4;
        my_digits++;
        if( (c >= '0') && (c <= '9') )
            *value |= (unsigned char)( c - '0' );
        else
            *value |= (unsigned char)( tolower(c) - 'a' + 10 );
    }
    else
        switch ( c )
        {
            case ' ':
                putchar( c );
                if( my_digits )
                {
                    my_digits = 0xff;
                }
                break;

            case '\r':
                if( !my_digits )
                {
                    my_digits = 0xff;
                }
                break;

            case '\b':
                if( my_digits )
                {
                    putstring( "\b \b" ); /* erase character */
                    my_digits--;
                    *value >>= 4;
                }
                else
                {
                    my_digits = 0xff;
                }
                break;

            default:
                my_digits = 0xff;
        }

   if( my_digits == 0xff )
   {
       *value = 0xffff;
       return my_digits;
   }
   else
       return c == '\r' ? my_digits : 0;
}


void prompt()
{
    putstring("\r\n> ");
}


void monitor()
{
    unsigned char c;

    if( !char_avail() )
        return;

    c = getchar();
    if( c == '\n' ) /* ignore '\n', we care for '\r' */
        return;

    switch( m.state )
    {
        case monitor_idle:
            putchar( c );
            switch( c )
            {
                case '-': /* decrements address */
                    m.address-=2;
                    /* intentionally no break here */
                case '+': /* increments address */
                    m.address++;
                    putchar('\r');
                    /* intentionally no break here */
                case '\r':
                    if( m.digits == 5 )
                        puthex( m.address_page );
                    if( m.digits > 2 )
                        puthex_u16( m.address );
                    else
                        puthex( (unsigned char)m.address );
                    putstring( ": " );
                    {
                        unsigned char d;
                        d = dump_address( m.address, (m.digits == 5) ? 2 : (m.digits > 2) );
                        if( d >= ' ' && d <= 0x7e )
                        {
                            putspace();
                            putchar( d );
                        }
                    }
                    prompt();
                    break;

                case '?': /* list of commands */
                    putstring( "\r\n?bBcdgGmMrsSw+-=&| see \"" __FILE__ "\"");
                    prompt();
                    break;

                case 'b': /* battery */
                    dump_ds2756();
                    prompt();
                    break;

                case 'B': /* all of battery */
                    dump_ds2756_all();
                    prompt();
                    break;

                case 'c': /* temperature of internal sensor */
#if DEBUG && defined(SDCC)
                    PRINTF( "\r\n%d degC", adc_to_degC( adc_cache[0] ) );
#else

                    putstring( "\r\n0x" );
                    puthex( adc_to_degC( adc_cache[0] ) );
                    putstring( " degC" );
#endif
                    prompt();
                    break;
#if DEBUG && defined(SDCC)

                /* temporarily here: */
                case 'C': /* dump temperature conversion */
                    {
                        unsigned char i = 0;
                        do
                        {
                            if(!(i&0x0f))
                            {
                                putcrlf();
                                puthex(i);
                                putchar(':');
                            }
                            PRINTF(" %d", adc_to_degC(i));
                        } while(++i);
                      }
                    break;
#endif

                case 'd': /* 8051 data memory */
                    dump_mcs51();
                    prompt();
                    break;

                case 'g': /* kb3700 xdata SFR */
                    dump_xdata_sfr();
                    prompt();
                    break;

                case 'G': /* detailled output of GPIO settings */
                    dump_gpio();
                    prompt();
                    break;

                case 'm': /* set address */
                    get_next_digit( &m.address, 0x00 );
                    m.state = command_m;
                    break;

                case 'M': /* set address of 64k block  */
                    /* this allows to address ANY byte in the flash!
                       (not only 0x0000..0xffff but 0x000000..0x0Fffff) */
                    get_next_digit( &m.address_page, 0x00 );
                    m.state = command_M;
                    break;

                case 'r': /* rebooting */
                    reboot();
                    break;

                case 's': /* dump states once */
                    print_states_enable = 1;
                    print_states();
                    print_states_enable = 0;
                    prompt();
                    break;

                case 'S': /* toggle continuous output mode for states info */
                    print_states_ruler();
                    print_states_enable = !print_states_enable;
                    prompt();
                    break;

                case 'w': /* watchdog reboot */
                    putstring("\r\nWatchdog bites?");
                    while (1)
                        ;
                    break;

                case '=': /* set address to value (can access either data, SFR, xdata, or xdata SFR) */
                    get_next_digit( &m.arg, 0x00 );
                    m.state = command_set;
                    break;

                case '|': /* bitwise or to previously set address */
                    get_next_digit( &m.arg, 0x00 );
                    m.state = command_or;
                    break;

                case '&': /* bitwise and to previously set address */
                    get_next_digit( &m.arg, 0x00 );
                    m.state = command_and;
                    break;

                default:
                    m.state = command_error;
            }
            break;

        case command_m: /* get memory address (2 or 4 digits) */
            {
                unsigned char digits;

                digits = get_next_digit( &m.address, c);
                if( digits > 4 )
                    m.state = command_error;
                else
                {
                    if( digits )
                    {
                        m.digits = digits;
                        m.state = 0;
                        prompt();
                    }
                }
            }
            break;

        // just a hack. Likely will be removed.
        case command_M: /* get single digit for 64k page */
            {
                unsigned char digits;

                digits = get_next_digit( &m.address_page, c);
                if( digits )
                {
                    m.digits = 5; /* 0xabcde */
                    m.state = 0;
                    prompt();
                }
            }
            break;

        case command_set:
        case command_and:
        case command_or:
            {
                unsigned char digits;

                digits = get_next_digit( &m.arg, c);
                if( digits > 2 )
                    m.state = command_error;
                else
                {
                    if( digits )
                    {
                        unsigned char t = (unsigned char)m.arg;

                        if( m.digits > 2 )
                        {
                            switch( m.state )
                            {
                                case command_set:
                                     *(unsigned char __xdata *)m.address = t;
                                     break;
                                case command_and:
                                     EA = 0;
                                     *(unsigned char __xdata *)m.address &= t;
                                     EA = 1;
                                     break;
                                case command_or:
                                     EA = 0;
                                     *(unsigned char __xdata *)m.address |= t;
                                     EA = 1;
                                     break;
                            }
                            gpio_check_IO_direction();
                            m.state = 0;
                            prompt();
                        }
                        else
                        {
                            if( m.address < 0x80 )
                            {
                                switch( m.state )
                                {
                                    case command_set:
                                         *(unsigned char __data *)m.address = t;
                                         break;
                                    case command_and:
                                         EA = 0;
                                         *(unsigned char __data *)m.address &= t;
                                         EA = 1;
                                         break;
                                    case command_or:
                                         EA = 0;
                                         *(unsigned char __data *)m.address |= t;
                                         EA = 1;
                                         break;
                                }
                                m.state = 0;
                                prompt();
                            }
                            else
                            {
                                /* sorry, no    write_mcs51_sfr() */
                                m.state = command_error;
                            }
                        }
                    }
                }
            }
            break;

        default:
            m.state = command_error;
    }

    if( m.state == command_error )
    {
        putspace();
        putchar( c );
        putchar('\a'); /* BEL */
        putchar('?');
        prompt();
        m.state = 0;
    }

    return;
}
