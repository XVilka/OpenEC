/*-------------------------------------------------------------------------
   power.h - handle power button on the EC

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

#define LED_PWR_OFF()  do{GPIOD08 |=  0x02;}while(0)
#define LED_PWR_ON()   do{GPIOD08 &= ~0x02;}while(0)
#define LED_PWR_IS_ON  (!(GPIOD08 & 0x02))

#define IS_AC_IN_ON    (GPIADIN & 0x04)

extern bool XO_suspended;

void power_init(void);

void handle_power(void);
