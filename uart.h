/*-------------------------------------------------------------------------
   uart.h - handle serial IO on the EC

   Copyright (C) 2007  

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
#include "chip.h"

void putchar(unsigned char c);
char getchar();
bool char_avail( void );
void tx_drain( void );

void uart_init();

void puthex(unsigned char c);
void puthex_u16(unsigned int i);
void putspace();
void putcrlf();
unsigned char putstring(unsigned char __code *p);

void uart_interrupt(void) __interrupt(4);
