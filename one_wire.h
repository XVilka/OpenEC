/*-------------------------------------------------------------------------
   one_wire.h - one wire protocol for the EC

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
#ifndef ONE_WIRE_H
#define ONE_WIRE_H

#include <stdbool.h>
#include "kb3700.h"

#define GPIOEOE0_ATOMIC(x) do{TIMER1_IRQ_DISABLE();{ x };TIMER1_IRQ_ENABLE();}while(0)

#define TIMER1_IRQ_ENABLE()  do{ET1=1;}while(0)
#define TIMER1_IRQ_DISABLE() do{ET1=0;}while(0)


typedef struct ow_transfer_type
{
    unsigned char RX_len;
    unsigned char TX_len;
    unsigned char __xdata *buf;

    unsigned int request_new:1;
    unsigned int request_completed:1;

    union
    {
        // character as sent to host?
        unsigned char c;
        struct
        {
            unsigned int internal_error:1;
            unsigned int busy_wait:1;
            unsigned int illegal_write:1;
            unsigned int crc_fail:1;

            unsigned int no_device:1;
            unsigned int no_device_flag_is_invalid:1;
            unsigned int line_stuck:1;
        };
    } error;
};



extern volatile unsigned char __pdata ow_transfer_buf[16];

void ow_init();
void ow_transfer_init(unsigned char num_tx, unsigned char num_rx );
bool ow_busy();
/*
unsigned char ow_get_read_byte(void);
void ow_reset_and_presence_detect_init(void);
unsigned char ow_reset_and_presence_detect_read(void);
*/ 

void timer1_init();
void timer1_interrupt(void) __interrupt (3) __using (1);

#endif
