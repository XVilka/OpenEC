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
#include "compiler.h"

//! some mcs51 have unified memory
/*! can execute code from xdata and code memory.
    This seems not to be the case for the kb3700
    (although the data sheet section 4.12 says
    "the 8051 uses MOVX and MOVC instructions to read [..] XRAM [..]"
    Maybe there is a bit to turn this on?)
 */
#define UNIFIED_MEMORY (0)


//! needs to be addressable as both code and xdata memory
static volatile unsigned char __xdata xdata_code_snippet[4];



//! routine to allow indexed read access to mcs-51 Special Function Registers
/*! The instruction set of the MCS-51 normally does not allow indexed
    access to SFR. 
    If there is unified memory access for code and xdata
    we can patch a code snippet and jump there.
    Otherwise there is a LARGE table.

    \note the kb3700 also has xdata addressable SFR (above 0xfc00).
    \note These are not meant here.
 */
unsigned char read_mcs51_sfr(unsigned char address) __naked
{
    address;     /* avoid warning about unused variable */

#if defined( SDCC ) && UNIFIED_MEMORY

    __asm
        ar2 = 0x02

        push    ar2

        mov     r2, dpl

        mov     a,#0x85      ; opcode for mov
        mov     dptr, #_xdata_code_snippet
        movx    @dptr, a

        mov     a,r2         ; src (address)
        inc     dptr
        movx    @dptr, a

        mov     a,#dpl       ; dest (dpl), # is intended
        inc     dptr
        movx    @dptr, a

        mov     a,#0x22      ; opcode for ret
        inc     dptr
        movx    @dptr, a

        pop     ar2

        ljmp    _xdata_code_snippet

    __endasm;

#elif defined( SDCC ) && !UNIFIED_MEMORY

    __asm
        mov     b, #0x00
        mov     a, dpl
        xrl     a, #0x80
        add     a, acc
        mov     _B_1, c
        add     a, acc
        mov     _B_0, c         ; now ACC and B hold SFR number (-0x80) multiplied by 4
        ;
        add     a, #jumptable
        push    acc
        mov     a,b
        addc    a, #(jumptable >> 8)
        push    acc
        ;
        ret                     ; "return" to the address pushed onto the stack

jumptable:
        .db     0x85, 0x80, 0x82, 0x22 ; the equivalent to: mov dpl,0x80  ret
        .db     0x85, 0x81, 0x82, 0x22 ; the equivalent to: mov dpl,0x81  ret
        .db     0x85, 0x82, 0x82, 0x22
        .db     0x85, 0x83, 0x82, 0x22
        .db     0x85, 0x84, 0x82, 0x22
        .db     0x85, 0x85, 0x82, 0x22
        .db     0x85, 0x86, 0x82, 0x22
        .db     0x85, 0x87, 0x82, 0x22
        .db     0x85, 0x88, 0x82, 0x22
        .db     0x85, 0x89, 0x82, 0x22
        .db     0x85, 0x8a, 0x82, 0x22
        .db     0x85, 0x8b, 0x82, 0x22
        .db     0x85, 0x8c, 0x82, 0x22
        .db     0x85, 0x8d, 0x82, 0x22
        .db     0x85, 0x8e, 0x82, 0x22
        .db     0x85, 0x8f, 0x82, 0x22

        .db     0x85, 0x90, 0x82, 0x22
        .db     0x85, 0x91, 0x82, 0x22
        .db     0x85, 0x92, 0x82, 0x22
        .db     0x85, 0x93, 0x82, 0x22
        .db     0x85, 0x94, 0x82, 0x22
        .db     0x85, 0x95, 0x82, 0x22
        .db     0x85, 0x96, 0x82, 0x22
        .db     0x85, 0x97, 0x82, 0x22
        .db     0x85, 0x98, 0x82, 0x22
        .db     0x85, 0x99, 0x82, 0x22
        .db     0x85, 0x9a, 0x82, 0x22
        .db     0x85, 0x9b, 0x82, 0x22
        .db     0x85, 0x9c, 0x82, 0x22
        .db     0x85, 0x9d, 0x82, 0x22
        .db     0x85, 0x9e, 0x82, 0x22
        .db     0x85, 0x9f, 0x82, 0x22

        .db     0x85, 0xa0, 0x82, 0x22
        .db     0x85, 0xa1, 0x82, 0x22
        .db     0x85, 0xa2, 0x82, 0x22
        .db     0x85, 0xa3, 0x82, 0x22
        .db     0x85, 0xa4, 0x82, 0x22
        .db     0x85, 0xa5, 0x82, 0x22
        .db     0x85, 0xa6, 0x82, 0x22
        .db     0x85, 0xa7, 0x82, 0x22
        .db     0x85, 0xa8, 0x82, 0x22
        .db     0x85, 0xa9, 0x82, 0x22
        .db     0x85, 0xaa, 0x82, 0x22
        .db     0x85, 0xab, 0x82, 0x22
        .db     0x85, 0xac, 0x82, 0x22
        .db     0x85, 0xad, 0x82, 0x22
        .db     0x85, 0xae, 0x82, 0x22
        .db     0x85, 0xaf, 0x82, 0x22

        .db     0x85, 0xb0, 0x82, 0x22
        .db     0x85, 0xb1, 0x82, 0x22
        .db     0x85, 0xb2, 0x82, 0x22
        .db     0x85, 0xb3, 0x82, 0x22
        .db     0x85, 0xb4, 0x82, 0x22
        .db     0x85, 0xb5, 0x82, 0x22
        .db     0x85, 0xb6, 0x82, 0x22
        .db     0x85, 0xb7, 0x82, 0x22
        .db     0x85, 0xb8, 0x82, 0x22
        .db     0x85, 0xb9, 0x82, 0x22
        .db     0x85, 0xba, 0x82, 0x22
        .db     0x85, 0xbb, 0x82, 0x22
        .db     0x85, 0xbc, 0x82, 0x22
        .db     0x85, 0xbd, 0x82, 0x22
        .db     0x85, 0xbe, 0x82, 0x22
        .db     0x85, 0xbf, 0x82, 0x22

        .db     0x85, 0xc0, 0x82, 0x22
        .db     0x85, 0xc1, 0x82, 0x22
        .db     0x85, 0xc2, 0x82, 0x22
        .db     0x85, 0xc3, 0x82, 0x22
        .db     0x85, 0xc4, 0x82, 0x22
        .db     0x85, 0xc5, 0x82, 0x22
        .db     0x85, 0xc6, 0x82, 0x22
        .db     0x85, 0xc7, 0x82, 0x22
        .db     0x85, 0xc8, 0x82, 0x22
        .db     0x85, 0xc9, 0x82, 0x22
        .db     0x85, 0xca, 0x82, 0x22
        .db     0x85, 0xcb, 0x82, 0x22
        .db     0x85, 0xcc, 0x82, 0x22
        .db     0x85, 0xcd, 0x82, 0x22
        .db     0x85, 0xce, 0x82, 0x22
        .db     0x85, 0xcf, 0x82, 0x22

        .db     0x85, 0xd0, 0x82, 0x22
        .db     0x85, 0xd1, 0x82, 0x22
        .db     0x85, 0xd2, 0x82, 0x22
        .db     0x85, 0xd3, 0x82, 0x22
        .db     0x85, 0xd4, 0x82, 0x22
        .db     0x85, 0xd5, 0x82, 0x22
        .db     0x85, 0xd6, 0x82, 0x22
        .db     0x85, 0xd7, 0x82, 0x22
        .db     0x85, 0xd8, 0x82, 0x22
        .db     0x85, 0xd9, 0x82, 0x22
        .db     0x85, 0xda, 0x82, 0x22
        .db     0x85, 0xdb, 0x82, 0x22
        .db     0x85, 0xdc, 0x82, 0x22
        .db     0x85, 0xdd, 0x82, 0x22
        .db     0x85, 0xde, 0x82, 0x22
        .db     0x85, 0xdf, 0x82, 0x22

        .db     0x85, 0xe0, 0x82, 0x22
        .db     0x85, 0xe1, 0x82, 0x22
        .db     0x85, 0xe2, 0x82, 0x22
        .db     0x85, 0xe3, 0x82, 0x22
        .db     0x85, 0xe4, 0x82, 0x22
        .db     0x85, 0xe5, 0x82, 0x22
        .db     0x85, 0xe6, 0x82, 0x22
        .db     0x85, 0xe7, 0x82, 0x22
        .db     0x85, 0xe8, 0x82, 0x22
        .db     0x85, 0xe9, 0x82, 0x22
        .db     0x85, 0xea, 0x82, 0x22
        .db     0x85, 0xeb, 0x82, 0x22
        .db     0x85, 0xec, 0x82, 0x22
        .db     0x85, 0xed, 0x82, 0x22
        .db     0x85, 0xee, 0x82, 0x22
        .db     0x85, 0xef, 0x82, 0x22

        .db     0x85, 0xf0, 0x82, 0x22
        .db     0x85, 0xf1, 0x82, 0x22
        .db     0x85, 0xf2, 0x82, 0x22
        .db     0x85, 0xf3, 0x82, 0x22
        .db     0x85, 0xf4, 0x82, 0x22
        .db     0x85, 0xf5, 0x82, 0x22
        .db     0x85, 0xf6, 0x82, 0x22
        .db     0x85, 0xf7, 0x82, 0x22
        .db     0x85, 0xf8, 0x82, 0x22
        .db     0x85, 0xf9, 0x82, 0x22
        .db     0x85, 0xfa, 0x82, 0x22
        .db     0x85, 0xfb, 0x82, 0x22
        .db     0x85, 0xfc, 0x82, 0x22
        .db     0x85, 0xfd, 0x82, 0x22
        .db     0x85, 0xfe, 0x82, 0x22
        .db     0x85, 0xff, 0x82, 0x22

    __endasm;
#else

    return 0xff;

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
        ar2 = 0x02

        push    ar2

        mov     r2, dpl

        mov     a,#0x85
        mov     dptr, #_xdata_code_snippet
        movx    @dptr, a

        mov     a, _write_mcs51_sfr_PARM_2
        inc     dptr
        movx    @dptr, a

        mov     a,r2
        inc     dptr
        movx    @dptr, a

        mov     a,#0x22
        inc     dptr
        movx    @dptr, a

        pop     ar2

        ljmp    _xdata_code_snippet

    __endasm;

#endif

}
