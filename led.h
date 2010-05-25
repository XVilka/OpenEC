/*-------------------------------------------------------------------------
   led.h - handles LEDs

   Written By - Frieder Ferlemann <Frieder.Ferlemann AT web.de>

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
#include "compiler.h"
#include "chip.h"

//! low level defines for LED colour
#define LED_CHG_G_OFF() do{GPIOED0 |=  0x08;}while(0)
#define LED_CHG_G_ON()  do{GPIOED0 &= ~0x08;}while(0)
#define LED_CHG_R_OFF() do{GPIOD08 |=  0x04;}while(0)
#define LED_CHG_R_ON()  do{GPIOD08 &= ~0x04;}while(0)

//! defines for LED colour that should "normally" be used
#define BATT_LED_OFF()  do{batt_led_colour = 0x00;}while(0)
#define BATT_LED_GREEN()do{batt_led_colour = 0x01;}while(0)
#define BATT_LED_RED()    do{batt_led_colour = 0x02;}while(0)
#define BATT_LED_ORANGE() do{batt_led_colour = 0x03;}while(0)
#define BATT_LED_GREEN_BLINKING()  do{batt_led_colour = 0x81;}while(0)
#define BATT_LED_RED_BLINKING()    do{batt_led_colour = 0x82;}while(0)
#define BATT_LED_ORANGE_BLINKING() do{batt_led_colour = 0x83;}while(0)
#define BATT_LED_JINGLE_1s()       do{batt_led_jingle = HZ;  }while(0)
#define BATT_LED_JINGLE_500ms()    do{batt_led_jingle = HZ/2;}while(0)

extern unsigned char __xdata batt_led_jingle;
extern unsigned char __xdata batt_led_colour;

void handle_leds(void);
