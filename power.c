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

/*
   see http://wiki.laptop.org/go/Hardware_Power_Domains
 */

#include <stdbool.h>
#include "kb3700.h"
#include "states.h"
#include "timer.h"
#include "power.h"

/* Might be dangerous */
#define ENABLE_OUTPUTS (0)


#define SWITCH_MAIN_ON_ON    do{GPIOD18 |=  0x04;}while(0)  /* GPIO1A */
#define SWITCH_MAIN_ON_OFF   do{GPIOD18 &= ~0x04;}while(0)

#define SWITCH_SUS_ON_ON     do{GPIOD18 |=  0x08;}while(0)  /* GPIO1B */
#define SWITCH_SUS_ON_OFF    do{GPIOD18 &= ~0x08;}while(0)

#define SWITCH_ECPWRRQST_ON  do{GPIOED8 |=  0x80;}while(0)  /* GPIOEF */
#define SWITCH_ECPWRRQST_OFF do{GPIOED8 &= ~0x80;}while(0)

#define SWITCH_PWR_BUT_ON    do{GPIOD08 &= ~0x10;}while(0)  /* GPIO0C# */
#define SWITCH_PWR_BUT_OFF   do{GPIOD08 |=  0x10;}while(0)  /* # */

#define SWITCH_DCON_EN_ON    do{GPIOED8 |=  0x20;}while(0)  /* GPIOED */
#define SWITCH_DCON_EN_OFF   do{GPIOED8 &= ~0x20;}while(0)

#define SWITCH_WLAN_ON       do{GPIOD00 |=  0x02;}while(0)  /* GPIO01 */
#define SWITCH_WLAN_OFF      do{GPIOD00 &= ~0x02;}while(0)

#define SWITCH_SWI_ON        do{GPIOD00 &= ~0x04;}while(0)  /* GPIO02# */
#define SWITCH_SWI_OFF       do{GPIOD00 |=  0x04;}while(0)  /* # */

#define SWITCH_VR_ON_ON      do{GPIOD00 &= ~0x01;}while(0)  /* GPIO00# */
#define SWITCH_VR_ON_OFF     do{GPIOD00 |=  0x01;}while(0)  /* # */

#define SWITCH_USB_PWR_ENn_B1B2_ON   do{}while(0) /* ToBeDone (only B1, B2) */
#define SWITCH_USB_PWR_ENn_B1B2_OFF  do{}while(0)


typedef enum
{
    XO_OFF,
    WAIT_FOR_POWER_BUTTON_RELEASE,
    XO_SWITCH_ON_FROM_WAKEUP,
    XO_SWITCH_ON_1,
    XO_SWITCH_ON_2,

    XO_SWITCH_ON_3,
    XO_SWITCH_ON_4,
    XO_SWITCH_ON_5,
    XO_RUNNING,
    XO_IS_IT_SUSPEND,

    XO_SUSPEND,
    XO_SWITCH_TO_SUSPEND,
    XO_SWITCH_OFF_SHORT_BLINK,
    XO_SWITCH_OFF_1,
    XO_SWITCH_OFF_2,
    XO_EMERGENCY_OFF
} state;


static struct
{
    unsigned char my_tick;
    unsigned char timer;
    state state;
    unsigned long wakeup_second;
} __pdata power_private;


void power_init(void)
{
    /*! POWER_BUTTON# is input */
    GPIOEIN0_0xfc64 |= 0x80;  /* avoiding name clash, GPIOEIN0 */

    /*! ACIN is input */
//    GPIADIE0 |= 0x04;

    /*! all outputs to off before enabling them as output */
    SWITCH_VR_ON_OFF;
    SWITCH_DCON_EN_OFF;
    SWITCH_ECPWRRQST_OFF;
    SWITCH_WLAN_OFF;
    SWITCH_SWI_OFF;
    SWITCH_PWR_BUT_OFF;
    SWITCH_MAIN_ON_OFF;

#if ENABLE_OUTPUTS
# warning ENABLE_OUTPUTS might be dangerous!

    /*! ECPWRRQST, DCON_EN are output */
    GPIOEOE8 |= 0xa0;

    /*! VR_ON#, WLAN_EN are output */
    GPIOOE00 |= 0x03;

    /*! PWR_BUT# is output */
    //GPIOOE08 |= 0x10;

    /*! MAIN_ON, SUS_ON is output */
    GPIOOE18 |= 0x0c;   // <------ ec-dump.fth shows 0x01 here!!!

#endif

    /*! notime soon:) */
    power_private.wakeup_second = 0xFFFFffff;
    //power_private.wakeup_second = 10;
}


//! Switches On/Off power to the various subsystems
/*! potential inputs:
        POWER_BUTTON, host request (shutdown, suspend, wlan, dcon),
        powersupply, temperature, keyboard, keypad...
    outputs:
        ECPWRRQST
        DCON
        WLAN
        others
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
            {
                power_private.state = XO_SWITCH_ON_FROM_WAKEUP;

                power_private.wakeup_second ^= 0x80000000uL; /**< kludge to make it happen only once */
            }

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

        case XO_SWITCH_ON_FROM_WAKEUP:
        case XO_SWITCH_ON_1:
            LED_PWR_ON;
            SWITCH_VR_ON_ON;
            power_private.timer = 0;
            power_private.state = XO_SWITCH_ON_2;
            break;

        case XO_SWITCH_ON_2:
            SWITCH_SUS_ON_ON;
            power_private.state = XO_SWITCH_ON_3;
            break;

        case XO_SWITCH_ON_3:
            SWITCH_DCON_EN_ON;
            power_private.state = XO_SWITCH_ON_4;
            break;

        case XO_SWITCH_ON_4:
            SWITCH_MAIN_ON_ON;
            power_private.state = XO_SWITCH_ON_5;
            break;

        case XO_SWITCH_ON_5:
            SWITCH_WLAN_ON;             /**< currently done here */
            power_private.state = XO_RUNNING;
            break;

        case XO_RUNNING:
            if( POWER_BUTTON_PRESSED )
            {
                power_private.state = XO_IS_IT_SUSPEND;
                power_private.timer = 0;

                /* also tell the host, how? */
            }
            else if ( 0 /* hosts requested power down */ /* hosts requested suspend */)
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

                    power_private.state = XO_SWITCH_OFF_SHORT_BLINK;
                    power_private.timer = 0;
                }

            }
            else
            {
                if( power_private.timer <= (unsigned char)HZ /* not longer than 0.5 second */ )
                {
                    /* also tell the host, how? */
                    power_private.state = XO_SWITCH_TO_SUSPEND;
                }
                else
                {
                    power_private.state = XO_SWITCH_OFF_SHORT_BLINK;
                    power_private.timer = 0;
                }
            }
            break;

        case XO_SWITCH_TO_SUSPEND:
            SWITCH_MAIN_ON_OFF;
            power_private.state = XO_SUSPEND;
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

        case XO_SWITCH_OFF_SHORT_BLINK:
            if( power_private.timer != 0xff )
                power_private.timer++;

            if( power_private.timer >= 3 )
                power_private.state = XO_SWITCH_OFF_1;
            else
                LED_PWR_ON;
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
            SWITCH_MAIN_ON_OFF;
            SWITCH_DCON_EN_OFF;
            SWITCH_SUS_ON_OFF;
            SWITCH_WLAN_OFF;
// SWITCH_PWR_BUT_OFF;
// SWITCH_ECPWRRQST_OFF;
// SWITCH_SWI_OFF;

            LED_PWR_OFF;
            power_private.state = XO_OFF;
            break;
    }

    STATES_UPDATE(power, power_private.state);
}
