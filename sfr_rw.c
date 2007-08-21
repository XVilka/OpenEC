/*-------------------------------------------------------------------------
   sfr_rw.c - reading/writing mcs-51 sfr

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

//! needs to be addressable as both code and xdata memory
static volatile unsigned char __xdata xdata_code_snippet[4];



//! routine to allow indexed read access to mcs-51 Special Function Registers
/*! The instruction set of the MCS-51 normally does not allow indexed
    access to SFR.
    Because the kb3700 uses unified memory access for code and xdata
    we can patch a code snippet and jump there.

    \note the kb3700 also has xdata addressable SFR (above 0xfc00).
    \note These are not meant here.
 */
unsigned char read_mcs51_sfr(unsigned char address) __naked
{
    address;     /* avoid warning about unused variable */

#if defined( SDCC )

    __asm
        mov     r2, dpl

        mov     dptr, #_xdata_code_snippet
        mov     a,#0x85      ; opcode for mov
        movx    @dptr, a
        inc     dptr

        mov     a,r2         ; src (address)
        movx    @dptr, a
        inc     dptr

        mov     a,#dpl       ; dest (dpl)
        movx    @dptr, a
        inc     dptr

        mov     a,#0x22      ; opcode for ret
        movx    @dptr, a

        ljmp    _xdata_code_snippet

    __endasm;

#endif

}


//! routine to allow indexed write access to mcs-51 Special Function Registers
/*! The instruction set of the MCS-51 normally does not allow indexed
    access to SFR.
    Because the kb3700 uses unified memory access for code and xdata
    we can patch a code snippet and jump there.

    \note the kb3700 also has xdata addressable SFR (above 0xfc00).
    \note These are not meant here.
 */
void write_mcs51_sfr(unsigned char address, unsigned char value) __naked
{

    address;    /* avoid warning about unused variable */
    value;      /* avoid warning about unused variable */

#if defined( SDCC )

    __asm
        mov     r2, dpl

        mov     dptr, #_xdata_code_snippet
        mov     a,#0x85
        movx    @dptr, a
        inc     dptr

        mov     a, _write_mcs51_sfr_PARM_2
        movx    @dptr, a
        inc     dptr

        mov     a,r2
        movx    @dptr, a
        inc     dptr

        mov     a,#0x22
        movx    @dptr, a

        ljmp    _xdata_code_snippet

    __endasm;

#endif

}
