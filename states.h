/*-------------------------------------------------------------------------
   states.h - skeleton for the Embedded Controler of the OLPC project

   Written By - Frieder Ferlemann <Frieder.Ferlemann AT web.de>

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

#define DEBUG_STATES (1)

#if DEBUG_STATES
# define STATES_UPDATE(x,y) do{states.x = (y);}while(0)
# define STATES_TIMESTAMP() do{states.last_timestamp=states.timestamp; \
                               states.timestamp=get_tick(); \
                               states.number++; \
                               }while(0)
#else
# define STATES_UPDATE(x,y) do{}while(0)
# define STATES_TIMESTAMP() do{}while(0)
#endif

typedef struct states_type {
     unsigned int timestamp;
     unsigned int last_timestamp; /* can be used to detect busy waiting:^) */
     unsigned int number;
     unsigned char command;
     unsigned char matrix_3x3;
     unsigned char power;
     unsigned char battery;
     unsigned char ds2756;
     unsigned char keyboard;
     unsigned char touchpad;
     unsigned char watchdog;
     unsigned int watchdog_pc;
};

//! this variable is meant to be used for debugging purposes only
/*! the state machines work on their private state variables (which
    are thus protected from manipulation).
    It's kind of a luxury variable and it might go away or be not
    handled in the release version.

    If the watchdog bites or external reset is applied this gives
    a high level view about the state the EC was in.

    Note: this variable is located at the end of the memory area.
    This area is intended not to be cleared on startup.
    Instead on startup this area is copied into another area
    so that the information stored here survives a reset and the
    state is still available when the EC has booted again.

    Currently the equivalent of memcpy(0xfb80, 0xfbc0, 0x40) is
    done on startup so this struct will be mirrored there.
 */
extern struct states_type __xdata __at (0xfbc0) states;
extern struct states_type __xdata __at (0xfb80) old_states;

void save_old_states( void );
void print_states (void);
void print_states_ruler (void);
