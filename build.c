/*-------------------------------------------------------------------------
   build.c - keeps version related info

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
#include "compiler.h"
#include "build.h"


#define PATCHLEVEL "p0"


#ifdef SDCC
# if (SDCC <270)
#  warning please use SDCC 2.7.0 and up
# else
#  define COMPILER_VERSION {'0' + SDCC / 100, \
                            '.', \
                            '0' + SDCC % 100 / 10, \
                            '.', \
                            '0' + SDCC % 10, \
                            0x00 }
# endif
#else
#  define COMPILER_VERSION "?.?.?"
#endif

/* these cookies are temporarily here.
   Used for checking bankswitching */
char __code __at(0x4000) cookie_1 = 0x01;
char __code __at(0x8000) cookie_2 = 0x02;
char __code __at(0xc000) cookie_3 = 0x03;



/* locations might be frozen in far future */
char __code __at(0xf300) url_string[]     = "http://www.laptop.org";

char __code __at(0xf390) name_string[]    = "openec";
char __code __at(0xf3a0) version_string[] = "0.0.3" PATCHLEVEL;
char __code __at(0xf3b0) compiler_version[] = COMPILER_VERSION;
char __code __at(0xf3c0) status_string[]  = "dangerous!";
char __code __at(0xf3d0) target_string[]  = "B1-B1";
char __code __at(0xf3e0) date_string[]    = __DATE__; /* YYYY-MM-DD would be nicer, see ISO 8601 */
char __code __at(0xf3f0) time_string[]    = __TIME__;

//! set so that the 32 bit overall little endian checksum over 0x0000-0xffff is zero
/*! see arguments to srec_cat in Makefile. There might be another checksum
    at 0xf3fc as addresses above 0xf400 are not mapped into code memory */
unsigned long  __code __at(0xfffc) code_checksum;
unsigned long  __code __at(0xf3fc) code_checksum_reserved;
