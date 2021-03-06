/*-------------------------------------------------------------------------
   flash.c - access the flash on the EC

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
#include "flash.h"

//! uses paged memory access to read a location anywhere in the SPI flash
/*! This routine uses XBISEG1 so it should not be in bank 1 (0x4000..0x7fff)
 */
unsigned char flash_read_byte(unsigned char page, unsigned int address)
{
#if defined( SDCC )

    unsigned char c;
    unsigned char ea_save;

    ea_save = EA;
    EA = 0;

    XBISEG1 = (unsigned char)(page << 2) |
              (unsigned char)(address >> 14) |
              0x80;
    c = *(unsigned char __xdata *)((address&0x7fff) | 0x4000);
    XBISEG1 = 0;

    EA = ea_save;

    return c;

#else

    return 0xff;

#endif
}
