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
#include "kb3700.h"

//! data from battery sensor
/*! adapt to Maxim/Dallas DS2756 */
typedef struct data_ds2756_type{
         unsigned int voltage_raw;
           signed int current_raw;
         unsigned long charge_raw;
           signed int temp_raw;

         unsigned char serial_number[6];
         unsigned int serial_number_valid:1;

         unsigned char eeprom_address_from_host;
         unsigned char eeprom_data_to_or_from_host;
         unsigned int eeprom_host_has_a_request:1;
         unsigned int eeprom_request_is_a_write:1;
         unsigned int eeprom_ec_completed_request:1;

         union
         {
             // as sent to host?
             unsigned char c;

             struct
             {
                 unsigned int internal_error:1;
                 unsigned int busy_wait:1;
                 unsigned int illegal_write:1;
                 unsigned int crc_fail:1;

                 unsigned int no_device:1;
                 unsigned int no_device_flag_is_invalid:1;
                 unsigned int line_stuck_low:1;
                 unsigned int line_stuck_high:1;
             };
         } error;
       };

extern struct data_ds2756_type __xdata data_ds2756;



bool handle_ds2756(void);
