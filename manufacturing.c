/*-------------------------------------------------------------------------
   manufacturing.c - access the manufacturing data on the EC

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

/*
   see http://wiki.laptop.org/go/Manufacturing_Data
 */

#include <stdbool.h>
#include <ctype.h>
#include "chip.h"
#include "flash.h"
#include "manufacturing.h"
#include "uart.h"


__pdata tagtype tag;


unsigned char manufacturing_read_byte(unsigned int address)
{
    return flash_read_byte(0x0e, address);
}


bool manufacturing_get_first_tag()
{
    tag.len = 0;
    tag.data_address = 0x0000; /* address of data byte (if any) */

    return manufacturing_get_next_tag();
}


bool manufacturing_get_next_tag()
{
    unsigned int address;
    unsigned char len, len_cpl;

    address = tag.data_address;

    tag.name[1] = manufacturing_read_byte( --address );
    tag.name[0] = manufacturing_read_byte( --address );
    len         = manufacturing_read_byte( --address );
    len_cpl     = manufacturing_read_byte( --address );

    if( (len & 0x80) ||
        (len ^ len_cpl) != 0xff )
        return 0;

    tag.len = len;

    address -= len;
    if( address < 0xf800 )
        return 0;

    tag.data_address = address;

    return 1;
}


void manufacturing_print_tag()
{
    unsigned char i,c;

    putchar( tag.name[0] );
    putchar( tag.name[1] );
    putspace();
    putchar('"');

    for( i = 0; i < tag.len; i++ )
    {
        c = manufacturing_read_byte( tag.data_address + i );
        if( isprint(c) )
            putchar(c);
        else
        {
            putchar('\\');
            putchar('x');
            puthex(c);
        }
    }
    putchar('"');
}


void manufacturing_print_all()
{
    if( manufacturing_get_first_tag() )
    {
        do
        {
            putcrlf();
            manufacturing_print_tag();
        } while( manufacturing_get_next_tag() );
    }
    putcrlf();
}


bool manufacturing_find_tag( unsigned char __code * ptr )
{
    unsigned char n0 = *ptr++;
    unsigned char n1 = *ptr;

    if( manufacturing_get_first_tag() )
    {
        do
        {
            if( (tag.name[0] == n0 &&
                (tag.name[1] == n1) ))
                return 1;
        } while( manufacturing_get_next_tag() );
    }
    return 0;
}
