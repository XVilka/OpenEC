/*-------------------------------------------------------------------------
   fs_entry.c - trampoline for a bankswitching routine

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

#if defined(SDCC)
//! This hook might be jumped to from a bankswitching routine
/*! see code in failsafe.c
 */
void bankswitch_entry(void) __naked
{
    __asm

        .area BS_ENTRY (CODE,ABS)
        .org 0x0200

        ajmp 0x0000 ; jump to the reset vector

    __endasm;
}
#endif


#if 0
// this could have done as well:
// unsigned char __code __at(0x0200) failsafe_hook[] = { 0x02,0x00,0x00 /* ljmp 0x0000 */};
#endif
