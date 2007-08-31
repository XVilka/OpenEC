/*-------------------------------------------------------------------------
   sfr_dump.c - dump register settings

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
#include "uart.h"
#include "sfr_rw.h"

typedef struct
{
    unsigned char __code *name;
    unsigned char __code *address;
    unsigned char len;
} ec_range_type;


static ec_range_type __code ec_range[] =
{
    {"GPIOO",  (unsigned char __code *)0xfc00, 0x04},
    {"GPIOE",  (unsigned char __code *)0xfc10, 0x06},
    {"GPIOD",  (unsigned char __code *)0xfc20, 0x06},
    {"GPIOIN", (unsigned char __code *)0xfc30, 0x07},
    {"GPIOPU", (unsigned char __code *)0xfc40, 0x06},
    {"GPIOOD", (unsigned char __code *)0xfc50, 0x04},
    {"GPIOIE", (unsigned char __code *)0xfc60, 0x07},
    {"GPIOM",  (unsigned char __code *)0xfc70, 0x01},
    {"KBC",    (unsigned char __code *)0xfc80, 0x07},
    {"PWM",    (unsigned char __code *)0xfe00, 0x0e},
    {"GPT",    (unsigned char __code *)0xfe50, 0x0a},
    {"WDT",    (unsigned char __code *)0xfe80, 0x06},
    {"LPC",    (unsigned char __code *)0xfe90, 0x10},
    {"SPI",    (unsigned char __code *)0xfea0, 0x10},
    {"PS2",    (unsigned char __code *)0xfee0, 0x07},
    {"EC",     (unsigned char __code *)0xff00, 0x20},
    {"GPWUEN", (unsigned char __code *)0xff30, 0x04},
    {"GPWUPF", (unsigned char __code *)0xff40, 0x04},
    {"GPWUPS", (unsigned char __code *)0xff50, 0x04},
    {"GPWUEL", (unsigned char __code *)0xff60, 0x04},
};


void dump_mcs51_sfr( void )
{
    unsigned char i = 0x80;

    do
    {
        if( (i & 0x07) == 0)
        {
            putstring( "\r\nSFR " );
            puthex( i );
            putchar(':');
        }
        else
        {
            if( (i & 0x03) == 0)
                putspace();
        }
        putspace();
        puthex( read_mcs51_sfr( i++ ) );

    } while( (unsigned char)i );


}


void dump_xdata_sfr( void )
{
    unsigned char i,k;

    for( i=0; i<sizeof ec_range / sizeof ec_range[0]; i++ )
    {
        putstring( "\r\n" );
        k = putstring( ec_range[i].name );
        while( k++ < 7 )
            putspace();
        puthex_u16( (unsigned int)ec_range[i].address );
        putchar(':');
        for( k=0; k<ec_range[i].len; k++ )
        {
            if( (k & 0x03) == 0)
                putspace();
            putspace();
            puthex( *(ec_range[i].address + k) );
        }
    }
}
