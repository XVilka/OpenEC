/*-------------------------------------------------------------------------
   uart.c - handle serial IO on the EC

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
#include "idle.h"
#include "timer.h"
#include "uart.h"

#define BITBANG (0)
#define BAUDRATE (115200uL)

#if BITBANG && defined(SDCC)

//! the delays in this bitbanging routine result in 115 kBaud with PLLCFG = 0x70
void putchar(unsigned char c)
{
    __asm
        mov     r2,dpl      ; argument
        mov     r3,#0x08    ; number of bits
        mov     dptr,#_GPIOD00

        ;
        clr     ea          ; disable IRQ


        movx    a,@dptr
        anl     a,#0xBF     ; TX_LOW;
        movx    @dptr,a

        mov     r4,#0x03
    STARTBITDELAY$:
        djnz    r4,STARTBITDELAY$
        ;

    BITLOOP$:
        mov     a,r2
        rrc     a           ; bit to send is in Carry now
        mov     r2,a
        ;

        movx    a,@dptr
        mov     _ACC_6,c    ; move carry into bit 6
        movx    @dptr,a
        ;
        mov     r4,#0x02
    BITDELAY$:
        djnz    r4,BITDELAY$
        nop
        ;

        djnz    r3,BITLOOP$
        ;

        mov     r4,#0x02
    BITDELAY2$:
        djnz    r4,BITDELAY2$
        ;

        movx    a,@dptr
        orl     a,#0x40     ; TX_HIGH;
        movx    @dptr,a

        ;
        setb    ea          ; enable IRQ
        ;

        mov     r4,#0x0a
    STOPBITDELAY:
        djnz    r4,STOPBITDELAY
        ;

    __endasm;
}


void uart_init()
{
    GPIOD00  |=  0x40; /**< set TX high */
    GPIOOE00 |=  0x40; /**< output enable for TX */
}

#elif !BITBANG && defined (SDCC)

static volatile unsigned char __pdata tx_buffer[64];
static volatile unsigned char __pdata rx_buffer[16];

static volatile unsigned char __pdata rx_head;
static          unsigned char __pdata rx_tail;
static          unsigned char __pdata tx_head;
static volatile unsigned char __pdata tx_tail;

static volatile bool tx_active;


void putchar(unsigned char c)
{
    unsigned char next_tx_head;

    next_tx_head = (unsigned char)(tx_head + 1) % sizeof tx_buffer;

    while( next_tx_head == tx_tail )
        ;

    ES = 0;

    tx_buffer[next_tx_head] = c;
    tx_head = next_tx_head;

    if( !tx_active)
    {
        tx_active = 1;
        TI = 1;      /**< start TX interrupt chain */
    }

    ES = 1;
}

//! unsigned? unsigned.
/*! please never poll here. Check buffer before calling.
 */
unsigned char getchar()
{
    unsigned char c;
    unsigned char rx_next_tail;

    rx_next_tail = (unsigned char)(rx_tail + 1) % sizeof rx_buffer;

    while( rx_tail == rx_head )
        ;

    c = rx_buffer[ rx_next_tail ];

    rx_tail = rx_next_tail; /* IRQ safe */

    return c;
}


void uart_interrupt(void) __interrupt(4)
{

    if( RI )
    {
        unsigned char next_rx_head;

        next_rx_head = (unsigned char)(rx_head + 1) % sizeof rx_buffer;
        if( next_rx_head != rx_tail )
        {
            rx_buffer[next_rx_head] = SBUF;
            rx_head = next_rx_head;
        }
        RI = 0;
        busy = 1;   /**< new data, do not sleep */
    }

    if( TI )
    {
        unsigned char next_tx_tail;

        TI = 0;
        next_tx_tail = tx_tail;
        if( next_tx_tail == tx_head )
            tx_active = 0;
        else
        {
            next_tx_tail = (unsigned char)(next_tx_tail + 1) % sizeof tx_buffer;
            SBUF = tx_buffer[next_tx_tail];
            tx_tail = next_tx_tail;
        }
    }
}

bool char_avail( void )
{
    return rx_tail ^ rx_head;
}

void uart_init()
{
    /* the IO part */
    GPIOFS00 |=  0x40; /**< switch to alternate output for TX */
    GPIOFS00 &= ~0x80; /**< no alternate output for RX */

    GPIOOE00 &= ~0x40; /**< no output enable for TX !!! GPIOFS seems to do it */
    GPIOOE00 &= ~0x80; /**< no output enable for RX */

    GPIOIE00 &= ~0x40; /**< TX is not input */
    GPIOIE00 |=  0x80; /**< RX is input */

    GPIOMISC |=  0x01; /**< GPIO06 is E51_TXD */
    GPIOMISC &= ~0x02; /**< GPIO07 is not E51_CLK */


    /* the UART part */
    SCON  = 0x50;      /**< not unusual */

    /* If baudrate does not match, please either adjust here or
       adjust PLLCFG in _sdcc_external_startup() */
    SCON2 = (unsigned char)((SYSCLOCK + BAUDRATE)/ (2 * BAUDRATE) >> 8);
    SCON3 = (unsigned char)((SYSCLOCK + BAUDRATE)/ (2 * BAUDRATE));

    SBUF = '*';

    ES = 0;
    rx_tail = rx_head;
    tx_head = tx_tail;
    ES = 1;
}

#else

/* gcc and others use their putchar() for now */
//void putchar(unsigned char c){}

/* dummy */
void uart_init()
{
}

bool char_avail( void )
{
    return 0;
}

#endif


//----8<----------------------------------------------------------------------


void puthex(unsigned char c)
{
    putchar("0123456789abcdef"[ c >> 4 ]);
    putchar("0123456789abcdef"[ c & 0x0f ]);
}

void puthex_u16(unsigned int i)
{
    puthex((unsigned char)(i>>8));
    puthex((unsigned char)i);
}

void putspace()
{
    putchar(' ');
}

void putcrlf()
{
    putchar('\r');
    putchar('\n');
}

unsigned char putstring(unsigned char __code *p)
{
    unsigned char c;
    unsigned char len = 0;

    while( (c = *p++) )
    {
        putchar(c);
        len++;
    }
    return len;
}

