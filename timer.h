/*-------------------------------------------------------------------------
   timer.h - timer routines for the Embedded Controler of the OLPC project

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

//! number of timer IRQs per second
#define HZ (100u)
#define SYSCLOCK (32000000uL)  /**< in Hertz, true? */
#define GPTCLOCK (32768u)      /**< in Hertz, true? */

extern volatile unsigned int __pdata tick;
extern volatile unsigned long __pdata second;

extern void timer_gpt3_init(void);

extern int get_tick(void);

extern unsigned long get_time(void);

extern unsigned long get_time_ms(void);

extern void set_time(unsigned long s);

extern void timer_gpt3_interrupt(void) __interrupt(0x17);
