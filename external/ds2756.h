/*-------------------------------------------------------------------------
   ds2756.h - handle Maxim/Dallas DS2756

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

/*
   Status: committed "as is".
 */


#include <stdbool.h>
#include "one_wire.h"

//! data from battery sensor
/*! adapt to Maxim/Dallas DS2756 */
typedef struct data_ds2756_type{
         unsigned int voltage_raw;
           signed int current_raw;
           signed int charge_raw;
           signed int temp_raw;
           signed int avg_current_raw;

         unsigned char serial_number[6];
         unsigned int serial_number_valid:1;

         struct ow_transfer_type host_transfer;
         struct ow_transfer_type batt_transfer;

         union ow_error_type error;
         union ow_error_type long_time_error;
       };

extern struct data_ds2756_type __xdata data_ds2756;

bool handle_ds2756_requests(void);
bool handle_ds2756_readout(void);
void dump_ds2756(void);
bool dump_ds2756_all();

int ds2756_raw_I_to_mA(int r);
int ds2756_raw_Q_to_mAh(int r);
unsigned int ds2756_raw_U_to_mV(unsigned int r);
int ds2756_raw_I_to_mA(int r);
int ds2756_raw_Q_to_mAh(int r);
unsigned int ds2756_raw_U_to_mV(unsigned int r);
unsigned int ds2756_raw_T_to_cC(signed int r);
