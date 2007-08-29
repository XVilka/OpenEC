/*-------------------------------------------------------------------------
   build.h - keeps version related info

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

/* locations might be frozen in far future */
extern char __code __at(0xf300) url_string[];

extern char __code __at(0xf390) name_string[];
extern char __code __at(0xf3a0) version_string[];
extern char __code __at(0xf3b0) compiler_version[];
extern char __code __at(0xf3c0) status_string[];
extern char __code __at(0xf3d0) target_string[];
extern char __code __at(0xf3e0) date_string[];
extern char __code __at(0xf3f0) time_string[];

extern unsigned long  __code __at(0xfffc) code_checksum;
extern unsigned long  __code __at(0xf3fc) code_checksum_reserved;
