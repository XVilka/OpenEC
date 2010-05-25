/*-------------------------------------------------------------------------
   power.c - handle power on the EC

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

   As a special exception, you may use this file as part of a free software
   library for the XO of the One Laptop per Child project without restriction.
   Specifically, if other files instantiate
   templates or use macros or inline functions from this file, or you compile
   this file and link it with other files to produce an executable, this
   file does not by itself cause the resulting executable to be covered by
   the GNU General Public License.  This exception does not however
   invalidate any other reasons why the executable file might be covered by
   the GNU General Public License.
-------------------------------------------------------------------------*/

/*! \file power.c
   see http://wiki.laptop.org/go/Hardware_Power_Domains
   see CS5536 data sheet
   see http://dev.laptop.org/git?p=olpc-2.6;a=blob;f=arch/i386/kernel/olpc-pm.c
   see http://dev.laptop.org/ticket/218
   see http://www.aosmd.com/pdfs/datasheet/AOZ1010.pdf
 */

#include <stdbool.h>
#include "chip.h"
#include "states.h"
#include "timer.h"
#include "one_wire.h"
#include "power.h"

/* dangerous! */
#define ENABLE_OUTPUTS (0)
#define SIMPLE (1)

#define IS_MAIN_ON    (GPIOD18 & 0x04) /* (B1 about 68mm, 108mm)U13,6  CS5536 WORKING? Bidirectional! */
#define IS_SUS_ON     (GPIOD18 & 0x08) /* (B1 near SD-Card)U33,6  CS5536 WORK_AUX? */


#define IS_MAIN_ON_AND_SUS_ON ((GPIOD18 & 0x0c) == 0x0c)
#define IS_MAIN_OFF_AND_SUS_OFF (!(GPIOD18 & 0x0c))

#define SWITCH_MAIN_ON_PU_ON()  do{ GPIOPU18 |=  0x04; }while(0)  /* pullup */
#define SWITCH_MAIN_ON_PU_OFF() do{ GPIOPU18 &= ~0x04; }while(0)  /* pullup */

#define SWITCH_ECPWRRQST_ON()  do{GPIOED8 |=  0x80;}while(0)  /* GPIOEF (to DCON) */
#define SWITCH_ECPWRRQST_OFF() do{GPIOED8 &= ~0x80;}while(0)

#define SWITCH_PWR_BUTn_ON()   do{GPIOD08 &= ~0x10;}while(0)  /* GPIO0C# (to CS5536 GPIO28 PWR_BUT#) */
#define SWITCH_PWR_BUTn_OFF()  do{GPIOD08 |=  0x10;}while(0)  /* # */

#define SWITCH_DCON_EN_ON()    do{GPIOED8 |=  0x20;}while(0)  /* GPIOED */
#define SWITCH_DCON_EN_OFF()   do{GPIOED8 &= ~0x20;}while(0)

#define SWITCH_WLAN_ON()       do{GPIOD00 |=  0x02;}while(0)  /* GPIO01 (B1)U15,6(near WLAN) */
#define SWITCH_WLAN_OFF()      do{GPIOD00 &= ~0x02;}while(0)

#define SWITCH_SWIn_ON()       do{GPIOD00 &= ~0x04;}while(0)  /* GPIO02# to CS5536 GPIO25, LOW_BAT#?
                                                               http://wiki.laptop.org/go/GPIO_Map says its an output(!) of CS5536 */
#define SWITCH_SWIn_OFF()      do{GPIOD00 |=  0x04;}while(0)  /* # */

#define SWITCH_VR_ONn_ON()     do{GPIOD00 &= ~0x01;}while(0)  /* GPIO00# Q26 (near CN24 and reset switch) */
#define SWITCH_VR_ONn_OFF()    do{GPIOD00 |=  0x01;}while(0)  /* # */

#define SWITCH_USB_PWR_ENn_B1B2_ON()   do{}while(0)           /* ToBeDone */
#define SWITCH_USB_PWR_ENn_B1B2_OFF()  do{}while(0)

#define POWER_BUTTON_PRESSED !(GPIOEIN0 & 0x80)             /* R74 next to SD-Card */


typedef enum
{
    XO_OFF,
    WAIT_FOR_POWER_BUTTON_RELEASE,
    XO_POWER_BUTTON_TOO_LONG_FOR_POWER_ON,
    XO_SWITCH_ON_FROM_WAKEUP,
    XO_SWITCH_ON_1,

    XO_SWITCH_ON_2,
    XO_SWITCH_ON_3,
    XO_SWITCH_ON_4,
    XO_SWITCH_ON_5,
    XO_RUNNING,

    XO_IS_IT_SUSPEND,
    XO_SWITCH_OFF_FROM_RUNNING,
    XO_SUSPEND,
    XO_SWITCH_TO_SUSPEND,
    SUSPEND_WAIT_FOR_POWER_BUTTON_RELEASE,

    XO_SWITCH_OFF_FROM_SUSPEND,
    XO_SWITCH_OFF_SHORT_BLINK,
    XO_SWITCH_OFF_1,
    XO_SWITCH_OFF_2,
    XO_MAIN_OR_SUS_ERROR,

    XO_EMERGENCY_OFF
} state;


static struct
{
    unsigned char my_tick;
    unsigned char timer;
    state state;
    unsigned long wakeup_second;
} __pdata power_private;

bool XO_suspended;


#if SIMPLE

void power_init(void)
{
    /*! POWER_BUTTON# is input */
    GPIOEIN0_0xfc64 |= 0x80;  /* avoiding name clash, GPIOEIN0 */

    /*! ACIN is input */
    GPIADIE0 |= 0x04;

    /*! MAIN_ON, SUS_ON are input */
    GPIOIE18 |= 0x0c;


    /*! all outputs to off before enabling them as output */
    SWITCH_VR_ONn_OFF();
    SWITCH_DCON_EN_OFF();
    SWITCH_ECPWRRQST_OFF();
    SWITCH_WLAN_OFF();
    SWITCH_SWIn_ON();
    SWITCH_PWR_BUTn_OFF();
    SWITCH_MAIN_ON_PU_OFF(); /* pullup off */

    /*! WLAN_EN is output. This one is here because the pin is floating on a B1. (remove) */
    GPIOOE00 |= 0x02;

#if ENABLE_OUTPUTS
# warning ENABLE_OUTPUTS might be dangerous!

    /*! ECPWRRQST, DCON_EN are output */
    GPIOEOE8 |= 0xa0;

    /*! VR_ON#, WLAN_EN are output */
    GPIOOE00 |= 0x03;

    /*! PWR_BUT# is output */
//    GPIOOE08 |= 0x10;

#endif

    /*! notime soon:) */
    power_private.wakeup_second = 0xFFFFffff;
    //power_private.wakeup_second = 10;
}


//! Switches On/Off power to the various subsystems
/*! This one is just a HACK to try to get things going.


    \verbatim
    approximate power on timing as measured on B1 Q2C23

    VR_ON#      ----__________________________________

    WLAN_EN     __------------------------------------

    SWI#        ____-----------------_________________

    PWR_BUT#    ____-----------------_________________

    MAIN_ON     ____----------------------------------

    SUS_ON      _______________________---------------



                --+-+----------------+-+-------------->
                  0 1
                    0

    \endverbatim

    (WLAN Power on seems very early)

 */
void handle_power(void)
{
    /* do not want to be called twice per tick */
    if( power_private.my_tick == (unsigned char)tick )
        return;
    power_private.my_tick = (unsigned char)tick;

    switch( power_private.state )
    {
        case 0:
            if( POWER_BUTTON_PRESSED )
            {
                power_private.timer++;
                if( power_private.timer == HZ/10 )
                {
                    LED_PWR_ON();
                    power_private.state = 1;
                }
            }
            else
                power_private.timer = 0;
            break;

        case 1:
            SWITCH_WLAN_ON();
            power_private.state = 2;
            break;

        case 2:
            /* only proceed if the 1-wire bus is not active.
               (the Geode and the EC are using the same LPC flash.
               The Geode fetching would most likely spoil timing
               of the 1-wire bus)
             */
            if( !ow_busy() )
            {
                SWITCH_PWR_BUTn_OFF();
                SWITCH_VR_ONn_ON();
                SWITCH_SWIn_OFF();
                SWITCH_MAIN_ON_PU_ON();
                SWITCH_DCON_EN_ON();

                power_private.timer = 0;
                power_private.state = 3;
            }
            break;

        case 3:
            /* just a timer... should probably check also some "ready" bit? */
            if( ++power_private.timer == HZ/4 )
            {
                SWITCH_PWR_BUTn_ON();

                // maybe:  SWITCH_MAIN_ON_PU_OFF;

                power_private.timer = 0;
                power_private.state = 4;
            }
            break;

        case 4:
            if( POWER_BUTTON_PRESSED )
            {
                /* powerdown procedure, maybe see http://dev.laptop.org/ticket/218 */
                SWITCH_PWR_BUTn_ON();
                SWITCH_MAIN_ON_PU_OFF();
                power_private.timer = 0;
                power_private.state = 5;
            }
            break;

        case 5:
            if(  /* wait for Southbridge to turn these off */
                 IS_MAIN_OFF_AND_SUS_OFF ||
                 /* but do not wait forever */
                 (++power_private.timer == HZ/10)  )
            {
                SWITCH_VR_ONn_OFF();
                SWITCH_SWIn_ON();
                SWITCH_DCON_EN_OFF();
                SWITCH_WLAN_OFF();
                LED_PWR_OFF();
                power_private.state = 6;
            }
            break;

        case 6:
            if( !POWER_BUTTON_PRESSED )
            {
                power_private.timer = 0;
                power_private.state = 0;
            }
            break;
    }

    STATES_UPDATE(power, power_private.state);
}


#else


















//! this was the first try with some more states
/*! but as it did not do the expected, I tried the
    simpler version above.
 */
void power_init(void)
{
    /*! POWER_BUTTON# is input */
    GPIOEIN0_0xfc64 |= 0x80;  /* avoiding name clash, GPIOEIN0 */

    /*! ACIN is input */
//    GPIADIE0 |= 0x04;

    /*! MAIN_ON, SUS_ON are input */
    GPIOIE18 |= 0x0c;


    /*! all outputs to off before enabling them as output */
    SWITCH_VR_ONn_OFF();
    SWITCH_DCON_EN_OFF();
    SWITCH_ECPWRRQST_OFF();
    SWITCH_WLAN_OFF();
    SWITCH_SWIn_OFF();
    SWITCH_PWR_BUTn_OFF();

#if ENABLE_OUTPUTS
# warning ENABLE_OUTPUTS might be dangerous!

    /*! ECPWRRQST, DCON_EN are output */
    GPIOEOE8 |= 0xa0;

    /*! VR_ON#, WLAN_EN are output */
    GPIOOE00 |= 0x03;

    /*! PWR_BUT# is output */
    GPIOOE08 |= 0x10;

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
                    LED_PWR_ON();
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
            if( !POWER_BUTTON_PRESSED )
            {
                power_private.state = XO_SWITCH_ON_1 ;
            }
            else
            {
                if( ++power_private.timer == (unsigned char)HZ )
                {
                    LED_PWR_OFF();
                    power_private.state = XO_POWER_BUTTON_TOO_LONG_FOR_POWER_ON;
                }
            }
            break;

        case XO_POWER_BUTTON_TOO_LONG_FOR_POWER_ON:
            if( !POWER_BUTTON_PRESSED )
            {
                power_private.state = XO_OFF;
            }
            else
            {
                if( power_private.timer != 0xff )
                    power_private.timer++;
            }
            break;

        case XO_SWITCH_ON_FROM_WAKEUP:
        case XO_SWITCH_ON_1:
            LED_PWR_ON();
            SWITCH_VR_ONn_ON();
SWITCH_DCON_EN_ON();

            power_private.timer = 0;
            power_private.state = XO_SWITCH_ON_2;
            break;

        case XO_SWITCH_ON_2:
            if( IS_MAIN_ON_AND_SUS_ON )
            {
                power_private.state = XO_SWITCH_ON_3;
            }
            else
            {
                if( ++power_private.timer == (unsigned char)HZ * 2 )
                    power_private.state = XO_MAIN_OR_SUS_ERROR;
            }
            break;

        case XO_SWITCH_ON_3:
            SWITCH_DCON_EN_ON();
            power_private.state = XO_SWITCH_ON_4;
            break;

        case XO_SWITCH_ON_4:
            power_private.state = XO_SWITCH_ON_5;
            break;

        case XO_SWITCH_ON_5:
            SWITCH_WLAN_ON();             /**< currently done here */
            power_private.state = XO_RUNNING;
            break;

        case XO_RUNNING:
            if( POWER_BUTTON_PRESSED )
            {
                power_private.state = XO_IS_IT_SUSPEND;
                power_private.timer = 0;
            }
            else if ( 0 /* hosts requested power down */ /* hosts requested suspend */)
            {
                power_private.state = XO_SWITCH_OFF_2;
            }
            break;

        case XO_IS_IT_SUSPEND:
            if( !POWER_BUTTON_PRESSED )
            {
                SWITCH_PWR_BUTn_OFF();
                power_private.state = XO_SWITCH_TO_SUSPEND;
            }
            else
            {
                if( ++power_private.timer == (unsigned char)HZ )
                {
                    /* tell host to power off */
                    power_private.state = XO_SWITCH_OFF_FROM_RUNNING;
                    power_private.timer = 0;
                    /* also tell the host, how? */
                    SWITCH_PWR_BUTn_ON(); /**< needs to be "pressed" for >=62 us (unfiltered) */
                }
            }
            break;


        case XO_SWITCH_OFF_FROM_RUNNING:

            power_private.state = XO_SWITCH_OFF_SHORT_BLINK;
            power_private.timer = 0;

#if 0
            /* southbrigde has turned power off? */
            if( IS_MAIN_OFF_AND_SUS_OFF )
            {
                power_private.state = XO_SWITCH_OFF_SHORT_BLINK;
                power_private.timer = 0;
            }
#endif
            break;


        case XO_SWITCH_TO_SUSPEND:
            /* dummy */
            power_private.state = XO_SUSPEND;
            break;

        case XO_SUSPEND:
            if( power_private.wakeup_second == get_time() )
                power_private.state = XO_SWITCH_ON_1;
            else
            {
                if( POWER_BUTTON_PRESSED )
                {
                    power_private.state = SUSPEND_WAIT_FOR_POWER_BUTTON_RELEASE;
                }
                else 
                {
                    if( ((unsigned char)tick & 0x7e) == 0x00 )
                        LED_PWR_ON();
                    else
                        LED_PWR_OFF();
                }
            }
            break;


        case SUSPEND_WAIT_FOR_POWER_BUTTON_RELEASE:
            if( !POWER_BUTTON_PRESSED )
            {
                power_private.state = XO_SWITCH_ON_1 ;
            }
            else
            {
                if( ++power_private.timer == (unsigned char)HZ )
                {
                    LED_PWR_OFF();
                    power_private.state = XO_SWITCH_OFF_FROM_SUSPEND;
                    power_private.timer = 0;
                }
            }
            break;

        case XO_SWITCH_OFF_FROM_SUSPEND:
        case XO_SWITCH_OFF_SHORT_BLINK:
            if( power_private.timer >= 3 )
                power_private.state = XO_SWITCH_OFF_1;
            else
            {
                power_private.timer++;
                LED_PWR_ON();
            }
            break;

        case XO_SWITCH_OFF_1:
            if( IS_MAIN_OFF_AND_SUS_OFF /* host acknowleged power down */ )
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
        case XO_MAIN_OR_SUS_ERROR:
            SWITCH_DCON_EN_OFF();
            SWITCH_WLAN_OFF(); /* ... */
            SWITCH_VR_ONn_OFF();
// SWITCH_PWR_BUT_OFF;
// SWITCH_ECPWRRQST_OFF;
// SWITCH_SWI_OFF;

            LED_PWR_OFF();
            power_private.state = XO_OFF;
            break;
    }

    STATES_UPDATE(power, power_private.state);
}
#endif
