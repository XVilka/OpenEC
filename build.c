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

/* locations might be frozen in far future */
char __code __at(0xff00) url_string[]     = "http://www.laptop.org";

char __code __at(0xffa0) name_string[]    = "open-ec";
char __code __at(0xffb0) version_string[] = "0.0.1p0";
char __code __at(0xffc0) status_string[]  = "dangerous!";
char __code __at(0xffd0) target_string[]  = "B2";
char __code __at(0xffe0) date_string[]    = __DATE__; /* YYYY-MM-DD would be nicer, see ISO 8601 */
char __code __at(0xfff0) time_string[]    = __TIME__;

//! set so that the 16 bit overall little endian checksum over 0x0000-0xffff is zero
/*! see arguments to srec_cat in Makefile */
int  __code __at(0xfffe) code_checksum;
