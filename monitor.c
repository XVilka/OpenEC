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
-------------------------------------------------------------------------*/

/*
   It's a hack. Adapt to what you need. Won't be in final firmware anyway:)
 */

#include <stdbool.h>
#include <ctype.h>
#include "kb3700.h"
#include "uart.h"
#include "sfr_rw.h"
#include "sfr_dump.h"
#include "states.h"

typedef enum {monitor_idle, command_m, command_set, command_error } monitor_state;

static struct
{
    monitor_state state;
    unsigned int address;
    unsigned int arg;
    unsigned char digits;
} __pdata m;


static void dump_address( unsigned int address, unsigned char is_xdata )
{
    if( is_xdata )
        puthex( *(unsigned char __xdata *)address );
    else
        if( (unsigned char)address < 0x80 )
            puthex( *(unsigned char __idata *)address );
        else
            puthex( read_mcs51_sfr( (unsigned char)address ) );
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

    if( !RI )
        return;
    c = getchar();
    if( c == '\n' )
        return;

    switch( m.state )
    {
        case 0:
            putchar( c );
            switch( c )
            {
                case '-':
                    m.address-=2;
                case '+':
                    m.address++;
                    putchar('\r');
                case '\r':
                    if( m.digits > 2 )
                        puthex_u16( m.address );
                    else
                        puthex( (unsigned char)m.address );
                    putstring( ": " );
                    dump_address( m.address, m.digits > 2 );
                    prompt();
                    break;
                case '?':
                    putstring( "\r\n?dgGmsS= see \"" __FILE__ "\"");
                    prompt();
                    break;
                case 'd':
                    dump_mcs51();
                    prompt();
                    break;
                case 'g':
                    dump_xdata_sfr();
                    prompt();
                    break;
                case 'G':
                    dump_gpio();
                    prompt();
                    break;
                case 'm':
                    get_next_digit( &m.address, 0x00 );
                    m.state = command_m;
                    break;
                case 's':
                    print_states_enable = 0;
                    prompt();
                    break;
                case 'S':
                    print_states_enable = 1;
                    prompt();
                    break;
                case '=':
                    get_next_digit( &m.arg, 0x00 );
                    m.state = command_set;
                    break;
                default:
                    m.state = command_error;
            }
            break;

        case command_m:
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

        case command_set:
            {
                unsigned char digits;

                digits = get_next_digit( &m.arg, c);
                if( digits > 2 )
                    m.state = command_error;
                else
                {
                    if( digits )
                    {
                        if( m.digits > 2 )
                        {
                            *(unsigned char __xdata *)m.address = (unsigned char)m.arg;
                            gpio_check_IO_direction();
                        }
                        else
                            if( m.address < 0x80 )
                            {
                                *(unsigned char __data *)m.address = (unsigned char)m.arg;
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
