/*-------------------------------------------------------------------------
   battery.h - skeleton for battery handling with the Embedded Controler

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
#include "compiler.h"

typedef struct {
         unsigned int bat_too_hot:1;
         unsigned int bat_over_voltage:1;
         unsigned int bat_under_voltage:1;
         unsigned int bat_implausible_capacity:1;
         unsigned int bat_state_machine_error:1;
         unsigned int bat_no_battery_detected:1;
         } battery_error_type ;

//extern  battery_error_type __xdata battery_error;

typedef struct {
         unsigned int voltage_mV;
           signed int current_mA;
         unsigned long charge_mAs;
         unsigned int charge_mAs_unreliable:1;
         unsigned int may_charge:1;
         unsigned int may_trickle_charge:1;
           signed int temp_degC;
         unsigned char bat_chemistry;

         // as sent to host
         union {
             unsigned char b;
             struct{
                 unsigned int battery_exists:1;
                 unsigned int battery_full_charged:1;
                 unsigned int battery_low:1;
                 unsigned int battery_destroyed:1;
                 unsigned int AC_in:1;
                 unsigned int reserved:3;
             } bits;
         } status_0x15;

         // state of charge as sent to host
         unsigned char soc_0x16;

         // as sent to host
         unsigned char errorcode_0x1f;
       } battery_type;

extern battery_type __xdata battery;


enum {
      PS_TYPE_UNKNOWN,
      PS_TYPE_SOLAR,
      PS_TYPE_Pb,
      PS_TYPE_WALL_ADAPTER,
      PS_TYPE_DONT_CARE_LINUX_SETS_PWM
     };

#if 0
extern struct {
        unsigned char ps_type;
        unsigned int voltage_mV;
        unsigned char unstable; /* counts up */
      } __xdata power_supply;
#endif

typedef enum
{
    IGNORE,
    ROM_OFF,
    ACTION_0,
    ACTION_1,
    ACTION_2,
    //GET_LAST_USED,
} action_type;


typedef struct
{
//(t, I, V, R, T, I*t, dV/dt, dT/dt).
       uint32_t t_ms;
       uint32_t delta_t_ms;
       uint16_t U_mV;
        int16_t I_mA;
       uint16_t Q_raw;
       uint16_t R_mOhm;
        int16_t T_cCelsius; /* 1/100 degree Celsius */
} battery_is_type;

extern bool battery_news;

bool handle_battery(void);
