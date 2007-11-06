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

/*! \file charge_sched.c

   Background: the proprietary firmware contains a sophisticated battery
   charging algorithm by GoldPeak. This algorithm is intended to be
   patented (according to a post on the openec mailing list 20070803).

   Instead of trying to reimplement that functionality in openec this
   code provides a table driven scheduler which reacts on entities like:
   (t, U, I, R, T, I*t).

   The scheduler "knows" what to do now and it can switch its state
   if one of these entities leaves a specified interval.

   Usually the tables the scheduler reacts on should be transmitted
   to the EC from a Linux userspace program running on the XO.
   (although tables for a time/voltage limited bootstrap charging
   algorithm might be present in EC firmware)

   The Linux userspace program can have access to much more and much
   more detailed data than the EC: it can keep a charging history,
   might keep a thermal and chemical model of the battery,
   could have access to the charging data acquired by the
   school based supercharger and can share its findings
   with the school server.

   See also: http://lists.laptop.org/pipermail/openec/2007-September/000068.html


   What can one do with the tables (some examples):

    - voltage or time limited charging,
    - very low speed charging (f.e. duty cycle of 1:10)
    - medium speed charging (low speed charging with intervals
      of high speed charging interjected)
    - signal the host every 10 or 60 seconds,
    - fall back to no charging if host has not updated
      after xx seconds,
    - and if host updates the tables pretty much anything
      should be within reach:
      (U limited charging (T corrected), I*t limited
      charging, -dU/dt or dT/dt limited charging)

   Note, the table driven approach moves complexity which
   otherwise would exist in program code into the data
   within the tables. So, yes, the tables might get itchy.
   One question is: can all the charging algorithms we might
   need be "mapped" onto this table driven approach?
 */

#include <stdbool.h>
#include "battery.h"
#include "charge_sched.h"
#include "led.h"
#include "power.h"
#include "states.h"
#include "timer.h"
#include "uart.h"

#define DEBUG 1

extern void set_charge_mode( unsigned char cur );


//! number of entries in the table. (Power of 2)
#define BATTERY_COMPARE_NUM (8)


/*! to avoid acting on tables while they are updated by the host */
bool charging_table_valid;

battery_is_type __xdata compare_is;

/*! pointer that select the table entry for the upper limit */
battery_compare_type __xdata * __pdata c_hi_ptr;
/*! pointer that select the table entry for the lower limit */
battery_compare_type __xdata * __pdata c_lo_ptr;


/*! RAM based array */
battery_compare_type __xdata compare_x_lo[BATTERY_COMPARE_NUM];
battery_compare_type __xdata compare_x_hi[BATTERY_COMPARE_NUM];

unsigned char __xdata num_in_use;

/*! ROM based built-in default for no charging at all. Might be used
   for upper and lower limits */
battery_compare_type __code compare_rom_off =
{
    {0, 0}                      /* initializes the complete struct to zero */
};


/*! built-in default for a simple time and voltage limited
   charging mode. Holds upper limits. */
battery_compare_type __code compare_rom_hi_charge_NiMH =
{
    {0,         0},             /* t_ms */
     3600*1000ul,               /* delta_t_ms */ /* give up after 1 minute / 1 hour? */
    {ROM_OFF,   5*1380},        /* U_mV */       /* 5 cells. Not T corrected */
    {ROM_OFF,   1000},          /* I_mA */       /* expecting about 400 mA */
    {0,         0},             /* Q_raw */      /* do not trust this blindly... */
    {0,         0},             /* R_mOhm */
    {ROM_OFF,   5200-500},      /* T_Celsius */  /* no charging above 52degC. Safety margin */
     0x80
};


/*! built-in default for a simple time and voltage limited
   charging mode. Holds lower limits. */
battery_compare_type __code compare_rom_lo_charge_NiMH =
{
    {0, 0},
     0,
    {ROM_OFF, 5*200},           /* low voltage. Short circuit? */
    {0, 0},                     /* low current. Open circuit? Could/Should have been detected by voltage */
    {0, 0},                     /* Q_raw */
    {0, 0},                     /* R_mOhm */
    {ROM_OFF,   0},             /* T_Celsius */  /* no charging below 0degC and no quick charge below +15degC? */
    0x80
};



// LiFe are currently empty

/*! built-in default for a simple time and voltage limited
   charging mode. Holds lower limits. */
battery_compare_type __code compare_rom_lo_charge_LiFe =
{
    {0,         0}              /* initializes the complete struct to zero */
};

/*! built-in default for a simple time and voltage limited
   charging mode. Holds upper limits. */
battery_compare_type __code compare_rom_hi_charge_LiFe =
{
    {0,         0}              /* initializes the complete struct to zero */
};


/*! dummy */
void set_charge_mode( unsigned char cur )
{
    cur;
}


//! copy memory from code to xdata space.
/*! This is not a exaxt memcpy clone */
void memcpy_xc( unsigned char __xdata *dst,
                unsigned char __code *src,
                unsigned char cnt )
{
    do
    {
       *dst++ = *src++;
    }
    while( --cnt );
}

//! copy memory from xdata to xdata space.
/*! This is not a exaxt memcpy clone */
void memcpy_xx( unsigned char __xdata *dst,
                unsigned char __xdata *src,
                unsigned char cnt )
{
    do
    {
       *dst++ = *src++;
    }
    while( --cnt );
}

void battery_charging_table_set_lo( unsigned char num, unsigned char __xdata *src )
{
    memcpy_xx( (void *)&compare_x_lo[num], src, sizeof compare_x_lo[0]);
}

void battery_charging_table_set_hi( unsigned char num, unsigned char __xdata *src )
{
    memcpy_xx( (void *)&compare_x_hi[num], src, sizeof compare_x_hi[0]);
}

//! switch to the specified entry
void battery_charging_table_set_num( unsigned char num )
{
//    if( num >= BATTERY_COMPARE_NUM )
//        panic;

    c_hi_ptr = (void *)&compare_x_hi[num];
    c_lo_ptr = (void *)&compare_x_lo[num];

    /* time based events are relative to current time */
    c_hi_ptr->t_ms.val = compare_is.t_ms + c_hi_ptr->delta_t_ms;
    c_lo_ptr->t_ms.val = compare_is.t_ms + c_lo_ptr->delta_t_ms;

    /* and now switch battery charging accordingly... */
//    set_charge_mode( c_hi_ptr->charging &0xc0 ); /* c_hi_ptr->charging == c_lo_ptr->charging */

    num_in_use = num;

    STATES_UPDATE(charge_sched, num_in_use);
}

unsigned char battery_charging_table_get_num( void )
{
    return num_in_use;
}

//! initialize charging table from ROM
void battery_charging_table_init( void )
{
    charging_table_valid = 0;

    memcpy_xc( (void *)&compare_x_lo[0], (void *)&compare_rom_off, sizeof compare_x_lo[0] );
    memcpy_xc( (void *)&compare_x_hi[0], (void *)&compare_rom_off, sizeof compare_x_hi[0] );
    memcpy_xc( (void *)&compare_x_lo[ROM_OFF], (void *)&compare_rom_off, sizeof compare_x_lo[0] );
    memcpy_xc( (void *)&compare_x_hi[ROM_OFF], (void *)&compare_rom_off, sizeof compare_x_hi[0] );
    memcpy_xc( (void *)&compare_x_lo[2], (void *)&compare_rom_lo_charge_NiMH, sizeof compare_x_lo[0] );
    memcpy_xc( (void *)&compare_x_hi[2], (void *)&compare_rom_hi_charge_NiMH, sizeof compare_x_hi[0] );
    memcpy_xc( (void *)&compare_x_lo[3], (void *)&compare_rom_lo_charge_LiFe, sizeof compare_x_lo[0] );
    memcpy_xc( (void *)&compare_x_hi[3], (void *)&compare_rom_hi_charge_LiFe, sizeof compare_x_hi[0] );

    /* NiMH LiFe detection missing here */
    battery_charging_table_set_num(2);

    charging_table_valid = 1;
}

//! check whether measured ('is') parameter is out of range and act accordingly
bool handle_battery_charging_table( void )
{
    unsigned char action;
    //static unsigned char __xdata last_action;

    /* to avoid acting on tables while they are updated by the host */
    if( !charging_table_valid )
    {
        /* if( !charging_table_valid_countdown-- ) panic(); */
        return 0;
    }

    /* we only have new data every once in a while over the one-wire bus
       (typ. 3.4ms (U) or 88 ms (I) or 220 ms (T) or 2.8 s (I average) ) */
    if( !battery_news )
        return 0;
    battery_news = 0;

    #if 1
    /* no external power -> nothing to do?
       (Loss or reappearing of power could also be handled by an entry
       within the tables. But maybe it's better not to add the functionality
       there, rather tell: my table does not apply any more, feed me new entries?)
     */
    if( !IS_AC_IN_ON )
    {
        set_charge_mode( 0 );
        return 0;
    }
    #endif

    #if 0
        /* emergency stuff here? */
        if( compare_is.I_mA > 3000 )
           too_high?...
        else if( charging && compare_is.U_mV < 1000)
           shortcircuit?...
        else
    #endif

    /* check whether there something to do and if the measured ('is') parameter is out of range */
    if( c_hi_ptr->t_ms.act && ((c_hi_ptr->t_ms.val - compare_is.t_ms) & 0x80000000) ) // 3 weeks...
    {
        action = c_hi_ptr->t_ms.act;
    }
    /* the "rest" of the checks should be regular */
    else if( c_hi_ptr->U_mV.act && compare_is.U_mV > c_hi_ptr->U_mV.val )
    {
        action = c_hi_ptr->U_mV.act;
    }
    else if( c_hi_ptr->I_mA.act && compare_is.I_mA > c_hi_ptr->I_mA.val )
    {
        action = c_hi_ptr->I_mA.act;
    }
    else if( c_hi_ptr->Q_raw.act && compare_is.Q_raw > c_hi_ptr->Q_raw.val )
    {
        action = c_hi_ptr->Q_raw.act;
    }
    else if( c_hi_ptr->T_cCelsius.act && compare_is.T_cCelsius > c_hi_ptr->T_cCelsius.val )
    {
        action = c_hi_ptr->T_cCelsius.act;
    }
    /* and now for the pointer holding the lower margins */
    else if( c_lo_ptr->U_mV.act && compare_is.U_mV < c_lo_ptr->U_mV.val )
    {
        action = c_lo_ptr->U_mV.act;
    }
    else if( c_lo_ptr->I_mA.act && compare_is.I_mA < c_lo_ptr->I_mA.val )
    {
        action = c_lo_ptr->I_mA.act;
    }
    else if( c_lo_ptr->Q_raw.act && compare_is.Q_raw < c_lo_ptr->Q_raw.val )
    {
        action = c_lo_ptr->Q_raw.act;
    }
    else if( c_lo_ptr->T_cCelsius.act && compare_is.T_cCelsius < c_lo_ptr->T_cCelsius.val )
    {
        action = c_lo_ptr->T_cCelsius.act;
    }
    else
        return 0;


    // if( action & 0x80 ) powerup_host(); ?

    action &= BATTERY_COMPARE_NUM - 1;

    c_hi_ptr = (void *)&compare_x_hi[action];
    c_lo_ptr = (void *)&compare_x_lo[action];

    /* time based events are relative to current time */
    c_hi_ptr->t_ms.val = compare_is.t_ms + c_hi_ptr->delta_t_ms;
    c_lo_ptr->t_ms.val = compare_is.t_ms + c_lo_ptr->delta_t_ms;


    /* and now switch battery charging accordingly... */
//    set_charge_mode( c_hi_ptr->charging &0xc0 ); /* c_hi_ptr->charging == c_lo_ptr->charging */

    /* LED colour set just to see something */
    BATT_LED_JINGLE_500ms();

#if DEBUG
    putstring("\r\nBatt:");
    puthex(action);
#endif

    /* also tell host... */

    num_in_use = action;

    STATES_UPDATE(charge_sched, num_in_use);

    return 1;
}
