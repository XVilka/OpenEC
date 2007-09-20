/*-------------------------------------------------------------------------
   adc.h - handle the ADC on the EC

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

#define ADC_START_CONVERSION do{ ADCTRL |= 0x01; } while(0)

extern unsigned char __xdata adc_cache[2];
extern unsigned char __xdata board_id;

void adc_interrupt(void) __interrupt(0x1f);

void adc_init(void);

void get_board_id(void);

unsigned char __code * board_id_to_string(void);
