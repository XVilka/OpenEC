/*-------------------------------------------------------------------------
   power.c - handle power button on the EC

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

#include <stdbool.h>
#include "kb3700.h"
#include "states.h"
#include "timer.h"
#include "power.h"

#define IS_MAIN_ON  (GPIOD18 & 0x04) /**< what does that mean? */
#define IS_SUS_ON   (GPIOD18 & 0x08) /**< what does that mean? */

#define SWITCH_ECPWRRQST_ON  do{GPIOED8 |=  0x80;}while(0)
#define SWITCH_ECPWRRQST_OFF do{GPIOED8 &= ~0x80;}while(0)

#define SWITCH_DCON_ON       do{GPIOED8 |=  0x20;}while(0)
#define SWITCH_DCON_OFF      do{GPIOED8 &= ~0x20;}while(0)

#define SWITCH_WLAN_ON       do{GPIOD00 |=  0x02;}while(0)
#define SWITCH_WLAN_OFF      do{GPIOD00 &= ~0x02;}while(0)



#define STATE_EMERGENCY_OFF  (10)

typedef enum
{
    XO_OFF,
    WAIT_FOR_POWER_BUTTON_RELEASE,
    XO_SWITCH_ON_1,
    XO_SWITCH_ON_2,
    XO_SWITCH_ON_3,

    XO_SWITCH_ON_4      = 5,
    XO_RUNNING,
    XO_IS_IT_SUSPEND,
    XO_SUSPEND,
    XO_SWITCH_OFF_1,

    XO_SWITCH_OFF_2     = 10,
    XO_EMERGENCY_OFF
} state;


struct
{
    unsigned char my_tick;
    unsigned char timer;
    enum
    {
      XO_OFF,
      WAIT_FOR_POWER_BUTTON_RELEASE,
      XO_SWITCH_ON_1,
      XO_SWITCH_ON_2,
      XO_SWITCH_ON_3,
      XO_SWITCH_ON_4,
      XO_RUNNING,
      XO_SWITCH_OFF,
      XO_EMERGENCY_SWITCH_OFF,
    } state___;
    state state;
    unsigned long wakeup_second;
} __pdata power_private;



void power_init(void)
{
    /* POWER_BUTTON# is input */
    *(volatile unsigned char __xdata *)0xfc64 |= 0x80;  /* avoiding name clash, GPIOEIN0 */
}

void power_IO_lines_init(void)
{
    /* MAIN_ON is input */
    /* SUS_ON is input */
    /* DCON is output */
    /* ECPWRRQST is output */
    /* DCON is output */
}

//! Switches On/Off power to the various subsystems
/*! potential inputs:
        POWER_BUTTON, host request (shutdown, suspend, wlan, dcon),
        powersupply, temperature, keyboard, keypad...
    outputs:
        ECPWRRQST
        DCON
        WLAN
 */
void handle_power(void)
{
    /* do not want to be called twice per tick */
    if( power_private.my_tick == (unsigned char)tick )
        return;
    power_private.my_tick = (unsigned char)tick;

    switch(power_private.state)
    {
        case XO_OFF:
            if( POWER_BUTTON_PRESSED )
            {
                power_private.timer++;
                if( power_private.timer == (unsigned char)(HZ/10) )
                {
                    LED_PWR_ON;
                    power_private.state = WAIT_FOR_POWER_BUTTON_RELEASE;
                }
            }
            else
                power_private.timer = 0;

            /*! use a GPT for this? */
            if( power_private.wakeup_second == get_time() )
                power_private.state = XO_SWITCH_ON_1;

            break;

        case WAIT_FOR_POWER_BUTTON_RELEASE:
            if( POWER_BUTTON_PRESSED )
            {
                if( power_private.timer != 0xff )
                    power_private.timer++;
                if( power_private.timer > (unsigned char)HZ )
                    LED_PWR_OFF;
            }
            else
            {
                /*! only proceed if button was not pressed too long */
                if( power_private.timer <= (unsigned char)HZ /* longer than 1 second */ )
                    power_private.state = XO_SWITCH_ON_1 ;
                else
                {
                    power_private.state = XO_OFF;
                    LED_PWR_OFF;
                }
            }
            break;

        case XO_SWITCH_ON_1:
            LED_PWR_ON;
            SWITCH_ECPWRRQST_ON;
            power_private.timer = 0;
            power_private.state = XO_SWITCH_ON_2;
            break;

        case XO_SWITCH_ON_2:
            if( 1 /*IS_MAIN_ON*/ )
            {
                power_private.state = XO_SWITCH_ON_3;
            }
            else
            {
                if( ++power_private.timer == (unsigned char)HZ )
                {
                    SWITCH_ECPWRRQST_OFF;
                    power_private.state = STATE_EMERGENCY_OFF;
                }
            }
            break;

        case XO_SWITCH_ON_3:
            SWITCH_DCON_ON;
            power_private.state = XO_SWITCH_ON_4;
            break;

        case XO_SWITCH_ON_4:
            SWITCH_WLAN_ON;
            power_private.state = XO_RUNNING;
            break;

        case XO_RUNNING:
            if( POWER_BUTTON_PRESSED )
            {
                power_private.state = XO_IS_IT_SUSPEND;
                power_private.timer = 0;

                /* also tell the host, how? */
            }
            else if ( 0 /* hosts requested power down */)
            {
                power_private.state = XO_SWITCH_OFF_2;
            }
            break;

        case XO_IS_IT_SUSPEND:
            if( power_private.timer != 0xff )
                power_private.timer++;

            if( POWER_BUTTON_PRESSED )
            {
                /* pressed for 1 second. */
                if( power_private.timer >= (unsigned char)HZ  )
                {
                    /* tell host power down */

                    power_private.state = XO_SWITCH_OFF_1;
                }

            }
            else
            {
                if( power_private.timer <= (unsigned char)HZ /* not longer than 0.5 second */ )
                {
                    /* also tell the host, how? */
                    power_private.state = XO_SUSPEND;
                }
                else
                    power_private.state = XO_OFF;

            }
            break;

        case XO_SUSPEND:
            if( power_private.wakeup_second == get_time() )
                power_private.state = XO_SWITCH_ON_1;
            else
            {
                if( POWER_BUTTON_PRESSED )
                {
                    power_private.state = WAIT_FOR_POWER_BUTTON_RELEASE;
                }
                else 
                {
                    if( ((unsigned char)tick & 0x7e) == 0x00 )
                        LED_PWR_ON;
                    else
                        LED_PWR_OFF;
                }
            }
            break;

        case XO_SWITCH_OFF_1:
            if( 0 /* host acknowleged power down */ )
            {
                power_private.state = XO_SWITCH_OFF_2;
            }
            else
            {
                if( ++power_private.timer == (unsigned char)HZ )
                {
                    /* Hmmm, host should have reacted by now! Shutting down anyway */
                    power_private.state = XO_EMERGENCY_OFF;
                }
            }
            break;

        case XO_SWITCH_OFF_2:
        case XO_EMERGENCY_OFF:
            LED_PWR_OFF;
            SWITCH_DCON_OFF;
            SWITCH_ECPWRRQST_OFF;
            /* SWITCH_WLAN_OFF; */

            power_private.state = XO_OFF;
            break;
    }

    STATES_UPDATE(power, power_private.state);
}
