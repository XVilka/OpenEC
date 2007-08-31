/*-------------------------------------------------------------------------
   battery.c - skeleton for battery handling with the Embedded Controller

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

/* This is just to have something there.

   See also:
   http://dev.laptop.org/git.do?p=olpc-2.6;a=tree;f=drivers/power
 */
#include <stdbool.h>
#include "kb3700.h"
#include "timer.h"
#include "states.h"

#define EXT_VOLTAGE_OK_FOR_CHARGING_mV (9600) /* fix */
#define EXT_VOLTAGE_OK_FOR_CHARGING_FROM_Pb_mV (11890+100) /* fix.. 11890mV is probably 0% */

#define VOLTAGE_MAX_FOR_NiMH_mV (6000)  /* fix. Eventually a function of T */
#define VOLTAGE_MAX_FOR_LiFe_mV (6000)  /* fix. Eventually a function of T */
#define VOLTAGE_MIN_FOR_NiMH_mV (5500)  /* fix. Eventually a function of T */
#define VOLTAGE_MIN_FOR_LiFe_mV (5500)  /* fix. Eventually a function of T */

#define MAX_NOMINAL_CAPACITY_FOR_NiMH_mAs (3800uL*60*60) /* fix, evtl. do not load fully */
#define MAX_NOMINAL_CAPACITY_FOR_LiFe_mAs (3800uL*60*60) /* fix */

#define MAX_PLAUSIBLE_CAPACITY_FOR_NiMH_mAs (MAX_NOMINAL_CAPACITY_FOR_NiMH_mAs*14/10) 
#define MAX_PLAUSIBLE_CAPACITY_FOR_LiFe_mAs (MAX_NOMINAL_CAPACITY_FOR_LiFe_mAs*14/10)

#define PWM_MAX (0xfe)

//! battery error flags
/*! read out by host resets? */
struct { unsigned int bat_too_hot:1;
         unsigned int bat_over_voltage:1;
         unsigned int bat_under_voltage:1;
         unsigned int bat_implausible_capacity:1;
         unsigned int bat_state_machine_error:1;
         unsigned int bat_no_battery_detected:1;
         } __xdata battery_error;

//! High level view of battery
struct {
         unsigned int timestamp;
         unsigned int voltage_mV;
           signed char temp_degC;
           signed int current_mA;
         unsigned long charge_mAs;
         unsigned int charge_mAs_unreliable:1;
         unsigned int may_charge:1;
         unsigned int may_trickle_charge:1;
           //unsigned int bat_chem_LiFe:1;
       } __xdata battery;

bool bat_chem_LiFe;

//! Power Supply type
/*! Should about powersupply we care? Probably yes.
    Move into separate file. */
enum {
      PS_TYPE_UNKNOWN,
      PS_TYPE_SOLAR,
      PS_TYPE_Pb,
      PS_TYPE_WALL_ADAPTER,
     };

struct {
        unsigned char ps_type;
        unsigned int voltage_mV;
        /* unsigned int voltage_max_eff_mV; */
        unsigned char unstable; /* counts up to 0xff */
      } __xdata power_supply;


//! fake register
volatile unsigned char __xdata pwm1;
//! should be from ADC
volatile unsigned int __xdata ext_voltage;



//! the state machine that handles the battery.
/*! This routine expects to be called at least every  
    xx ms while the XO is up and running
    xx ms while the XO is powered off.  
    Just use it as a skeleton. It's not in any way
    adapted (neither to the DS2756 nor to linux)...
    And it's buggy anyway.
 */
bool handle_battery(void)
{
    static enum{
        bat_init,
        bat_get_info,
        bat_trickle_charge_unknown_battery,
        bat_ramp_up_charge_current,
        bat_charge,
        bat_trickle_charge_full_battery,
        bat_run_from_bat,
        bat_not_present,
    } __pdata state = bat_init;

    static unsigned char __pdata my_tick;

    /* do not run more than once per tick.
       Note, comparing only the LSB of tick avoids problems
       with tick being a volatile non-atomic variable 
       (and as a not unwelcome side-effect it results in 
       shorter code) */
    if( my_tick == (unsigned char)tick)
        return 0;


    switch(state){

        case bat_init:
            state = bat_get_info;
            break;

        case bat_get_info:
            state = bat_trickle_charge_unknown_battery;
            break;

        case bat_trickle_charge_unknown_battery:
            if( battery.may_charge )
                state = bat_ramp_up_charge_current;
            break;

        case bat_charge:
        case bat_ramp_up_charge_current:


            if( battery.charge_mAs >= (bat_chem_LiFe ?
                                       (unsigned long)MAX_NOMINAL_CAPACITY_FOR_LiFe_mAs :
                                       MAX_NOMINAL_CAPACITY_FOR_NiMH_mAs))
            {
                state = bat_trickle_charge_unknown_battery;
                battery.charge_mAs_unreliable = 1;
                battery.may_trickle_charge = 0;
            }
            else
            {
                /* Hmm, there is more to it:
                   say a suspended XO would consume a trickle charge
                   of 100mA at (EXT_VOLTAGE_OK_FOR_CHARGING_mV - 50mV)
                   and would then at (EXT_VOLTAGE_OK_FOR_CHARGING_mV + 50mV)
                   draw 1000mA then the XO would present a load
                   with a resistance of -900mA/100mV = -9.0 Ohm to the
                   power supply.
                   A negative input resistance is a fine ingredient for an
                   oscillator. We don't want an oscillator and much less
                   we want a class room of them =:)
                   */
                switch(power_supply.ps_type)
                {
                    case PS_TYPE_WALL_ADAPTER:
                         /* no fuzzing around, gimme what I want */
                         pwm1 = PWM_MAX;
                         break;

                    case PS_TYPE_Pb:    /* should be very scrupulous to avoid
                                           deep cycling Pb. */
                        if (ext_voltage >= EXT_VOLTAGE_OK_FOR_CHARGING_FROM_Pb_mV)
                        {
                          /* relatively slow ramp up */
                          if( pwm1 <= PWM_MAX )
                              pwm1++;
                        }
                        else
                        {
                           if( pwm1 )
                               pwm1--;
                           /* ramp down more quickly */
                           if( pwm1 )
                               pwm1--;
                        }
                        break;

                    case PS_TYPE_SOLAR: /* should try to optimize efficiency
                                           of the solar panel */
                    case PS_TYPE_UNKNOWN:
                    default:
                        if (ext_voltage >= EXT_VOLTAGE_OK_FOR_CHARGING_mV)
                        {
                           /* relatively slow ramp up */
                           if( pwm1 <= PWM_MAX )
                              pwm1++;
                        }
                        else
                        {
                           if( pwm1 )
                               pwm1--;
                           /* ramp down more quickly */
                           if( pwm1 )
                               pwm1--;
                           if( power_supply.unstable != 0xff )
                               power_supply.unstable++;
                        }
                        break;
                  }
            }
            break;

        default: 
            battery_error.bat_state_machine_error = 1;
            state = bat_init;

    }

    if( battery.voltage_mV <= (bat_chem_LiFe ?
                                 VOLTAGE_MIN_FOR_LiFe_mV :
                                 VOLTAGE_MIN_FOR_NiMH_mV))
    {
        battery_error.bat_under_voltage = 1;
    }

    /* this is time consuming and needs an sloc */
    battery.charge_mAs += battery.current_mA *
                          ((unsigned char)tick - my_tick) / HZ;

    my_tick = (unsigned char) tick;

    /* for debugging */
    STATES_UPDATE(battery, state);

    /* the only exit of this function, exept the one on top? */
    return 0;
}

