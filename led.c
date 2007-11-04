/*-------------------------------------------------------------------------
   led.c - handles LEDs

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
#include "led.h"
#include "power.h"
#include "timer.h"


//! battery LEDs change colour as long as this countdown timer is nonzero
unsigned char __xdata batt_led_jingle = HZ;

//! bit 0 green, bit 1 red. bit 7 used for blinking mode
unsigned char __xdata batt_led_colour;


//! handle LEDs according to battery state, power_mode, blinking_mode and jingle
void handle_leds(void)
{
    static unsigned char __pdata my_tick;

    if( my_tick == (unsigned char) tick )
        return;

    my_tick = (unsigned char) tick;

    if( batt_led_jingle )
    {
        batt_led_jingle--;
        switch ( (batt_led_jingle >> 3) & 0x01 )
        {
            case 0: LED_CHG_R_ON();
                    LED_CHG_G_OFF();
                    break;
            case 1: LED_CHG_G_ON();
                    LED_CHG_R_OFF();
                    break;
        }
    }
    else
    {
        if( /* off if XO is off and no external supply */
            (!IS_AC_IN_ON  && !LED_PWR_IS_ON)  ||
            /* in suspend mode mostly off */
            ((XO_suspended && ((unsigned char)my_tick & 0x7e))) ||
            /* blinking mode? */
            ((batt_led_colour & 0x80) && ((unsigned char)my_tick & 0x40)) )
        {
            LED_CHG_G_OFF();
            LED_CHG_R_OFF();
        }
        else
        {
            switch ( batt_led_colour & 0x03 )
            {
                case 0:
                    LED_CHG_G_OFF();
                    LED_CHG_R_OFF();
                    break;
                case 1:
                    LED_CHG_G_ON();
                    LED_CHG_R_OFF();
                    break;
                case 2:
                    LED_CHG_G_OFF();
                    LED_CHG_R_ON();
                    break;
                case 3:
                    LED_CHG_G_ON();
                    LED_CHG_R_ON();
                    break;
            }
        }
    }
}
