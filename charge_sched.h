/*-------------------------------------------------------------------------
   charge_sched.c - table based charge current scheduler

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

#include <stdint.h>
#include <stdbool.h>
#include "kb3700.h"

typedef struct
{
   uint8_t act;
   uint16_t val;
} action_and_val_uint16_type;

typedef struct
{
   uint8_t act;
   uint32_t val;
} action_and_val_uint32_type;

typedef struct
{
   uint8_t act;
   int16_t val;
} action_and_val_int16_type;


typedef struct
{
    //(t, V, I, Q, R, T, eventually: dV/dt, dT/dt).
    action_and_val_uint32_type t_ms;
    uint32_t delta_t_ms;

    action_and_val_uint16_type U_mV;
    action_and_val_int16_type  I_mA;
    action_and_val_uint16_type Q_raw;        /* ... */
    action_and_val_uint16_type R_mOhm;       /* not handled */
    action_and_val_int16_type  T_cCelsius;   /* in 1/100 degree Celsius */

    /* if needed host should keep this variables.
       We do not want to come near these critical entities and
       do not want to pretend to be able to handle them reasonably.
       So better do not have these entities here?
    action_and_val_int16_type dU_dT_in_mV_per_s; 
    action_and_val_int16_type dT_dt_in_mC_per_s;
    */

    uint8_t charging;

} battery_compare_type;


extern bool charging_table_valid;

void battery_charging_table_init( void );
void battery_charging_table_set_lo( unsigned char num, unsigned char __xdata *src );
void battery_charging_table_set_hi( unsigned char num, unsigned char __xdata *src );
void battery_charging_table_set_num( unsigned char num );
unsigned char battery_charging_table_get_num();

bool handle_battery_charging_table( void );
