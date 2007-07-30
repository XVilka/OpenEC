/*-------------------------------------------------------------------------
   unused_irq.h - safe IRQ stubs for unused IRQ

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

/* This file serves two purposes, it can
   a) either only provide the prototypes or
   b) define the functions.
 */

#if defined( UNUSED_IRQ_GENERATE_FUNCTIONS )
#   define UNUSED_IRQ(name, number) \
        /* unused IRQ, clear the corresponding IRQ enable */ \
        void name (void) __interrupt(number) \
        { \
            switch(number / 0x08) \
            { \
                 case 0: IE   &= ~(1 << (number - 1)); \
                         /* spurious_irq[0] |= (1 << (number - 1)); */ \
                         break; \
                 case 1: P0IE &= ~(1 << (number - 0x08)); \
                         /* spurious_irq[1] |= (1 << (number - 0x08)); */ \
                         break; \
                 case 2: P1IE &= ~(1 << (number - 0x10)); \
                         /* spurious_irq[2] |= (1 << (number - 0x10)); */ \
                         break; \
                 case 3: P3IE &= ~(1 << (number - 0x18)); \
                         /* spurious_irq[3] |= (1 << (number - 0x18)); */ \
                         break; \
            } \
        }

#   if defined( SDCC )
#       pragma disable_warning 116
#       pragma disable_warning 126
#   endif
#else
#   define UNUSED_IRQ(name, number) void name (void) __interrupt(number);
#endif

extern unsigned char __xdata spurious_irq[4];

UNUSED_IRQ(ext0_interrupt,      0x01)
UNUSED_IRQ(timer0_interrupt,    0x02)
UNUSED_IRQ(ext1_interrupt,      0x03)
UNUSED_IRQ(timer1_interrupt,    0x04)
UNUSED_IRQ(serial0_interrupt,   0x05)
UNUSED_IRQ(n_a_0x06_interrupt,  0x06)
UNUSED_IRQ(n_a_0x07_interrupt,  0x07)

//UNUSED_IRQ(watchdog_interrupt,0x08)
UNUSED_IRQ(n_a_0x09_interrupt,  0x09)
UNUSED_IRQ(ps2_interrupt,       0x0a)
UNUSED_IRQ(kbc_interrupt,       0x0b)
UNUSED_IRQ(rsv_0x0c_interrupt,  0x0c)
UNUSED_IRQ(lpc_interrupt,       0x0d)
//UNUSED_IRQ(ec_hi_interrupt,   0x0e)
UNUSED_IRQ(n_a_0x0f_interrupt,  0x0f)

UNUSED_IRQ(rsv_0x10_interrupt,  0x10)
UNUSED_IRQ(rsv_0x11_interrupt,  0x11)
UNUSED_IRQ(rsv_0x12_interrupt,  0x12)
UNUSED_IRQ(n_a_0x13_interrupt,  0x13)
UNUSED_IRQ(gpt0_interrupt,      0x14)
UNUSED_IRQ(gpt1_interrupt,      0x15)
UNUSED_IRQ(gpt2_interrupt,      0x16)
//UNUSED_IRQ(gpt3_interrupt,    0x17)

UNUSED_IRQ(extwio_port80_interrupt, 0x18)
UNUSED_IRQ(gpio00_0f_interrupt, 0x19)
UNUSED_IRQ(gpio10_1b_interrupt, 0x1a)
UNUSED_IRQ(rsv_0x1b_interrupt,  0x1b)
UNUSED_IRQ(rsv_0x1c_interrupt,  0x1c)
UNUSED_IRQ(rsv_0x1d_interrupt,  0x1d)
UNUSED_IRQ(rsv_0x1e_interrupt,  0x1e)
UNUSED_IRQ(adc_interrupt,       0x1f)
