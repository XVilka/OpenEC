/*-------------------------------------------------------------------------
   matrix_3x3.h - routines to handle 3x3 matrix within the OLPC project

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

//! keeps the externally visible data to handle the 3x3 matrix
typedef struct cursors
{
    //! keycode we'd want to transmit to the host
    unsigned char keycode[3];

    //! 9 bits of key status as transmitted to the host
    unsigned char game_key_status[2];
    
    // unsigned int keycode_timestamp;

    //! Flag for handshaking
    /*! If there's a new keycode this flag is set.
        If transmitted another routine will reset this flag.
        Note, this currently is the only variable that might
        be changed from "outside" (from a routine that is
        not within matrix_3x3.c)
     */
    unsigned int keycode_updated:1;
};

extern struct cursors __pdata cursors;

extern bool handle_cursors(void);

