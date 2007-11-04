/*-------------------------------------------------------------------------
   watchdog.h - handle watchdog of the EC

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

#define WATCHDOG_MAIN_LOOP_IS_FINE    (0x80)
#define WATCHDOG_PS2_IS_FINE          (0x40)
#define WATCHDOG_BATTERY_CODE_IS_FINE (0x20)
#define WATCHDOG_ONE_WIRE_IS_FINE     (0x10)
#define WATCHDOG_IRQ_OCCURED          (0x01)

extern unsigned char __data watchdog_all_up_and_well;

void watchdog_init(void);

void watchdog_interrupt(void) __interrupt(0x08);
