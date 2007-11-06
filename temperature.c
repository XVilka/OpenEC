/*-------------------------------------------------------------------------
   temperature.c - routines for the NTC on the EC

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

/*! \file temperature.c
   see f.e.:
   http://www.specsensors.com/ntc-engineering.asp
   http://pdfserv.maxim-ic.com/en/an/AN817.pdf
   http://lists.laptop.org/pipermail/openec/2007-September/000092.html
 */

#include <stdbool.h>
#include "kb3700.h"
#include "adc.h"
#include "uart.h"


/* extracting some data of AN817 figure 5 by hand.
   Just to have something.

   I arbitrarily picked the middle curve (Beta = 4100 Kelvin)
   and coarsly extracted some numbers.
   (Above 70 degrees and below -20 degrees they will be boguous anyway)

   R363 (the NTC used) most likely is different. What is its Beta?
 */
#define T_0x00 (110) /* delta <= 16 */
#define T_0x10 ( 95) /* delta <= 16 */
#define T_0x20 ( 80) /* delta <= 16 */
#define T_0x30 ( 65)
#define T_0x40 ( 53)
#define T_0x50 ( 46)
#define T_0x60 ( 40)
#define T_0x70 ( 32)
#define T_0x80 ( 25)
#define T_0x90 ( 20)
#define T_0xa0 ( 13)
#define T_0xb0 (  8)
#define T_0xc0 (  2)
#define T_0xd0 ( -5)
#define T_0xe0 (-12)
#define T_0xf0 (-25)
#define T_0x100 (-40) /* needed for interpol */

char __code table[16] =
{
    T_0x00, T_0x10, T_0x20, T_0x30,
    T_0x40, T_0x50, T_0x60, T_0x70,
    T_0x80, T_0x90, T_0xa0, T_0xb0,
    T_0xc0, T_0xd0, T_0xe0, T_0xf0
};

char __code table_delta[16] =
{
    /* all deltas should be <= 16 (>= -16) */
    T_0x00 - T_0x10,
    T_0x10 - T_0x20,
    T_0x20 - T_0x30,
    T_0x30 - T_0x40,

    T_0x40 - T_0x50,
    T_0x50 - T_0x60,
    T_0x60 - T_0x70,
    T_0x70 - T_0x80,

    T_0x80 - T_0x90,
    T_0x90 - T_0xa0,
    T_0xa0 - T_0xb0,
    T_0xb0 - T_0xc0,

    T_0xc0 - T_0xd0,
    T_0xd0 - T_0xe0,
    T_0xe0 - T_0xf0,
    T_0xf0 - T_0x100
};


//! convert the ADC readout voltage to temperature
char adc_to_degC(unsigned char c)
{
    /* not a generic interpolation routine. Needs monotonously falling values */
    return table[c >> 4] - (unsigned char)(table_delta[c >> 4] * (c & 0x0f) + 8) / 16;
}
