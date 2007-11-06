/*-------------------------------------------------------------------------
   one_wire.c - one wire protocol for the EC

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

/*! \file one_wire.c
    \image latex ow_reset.png "DQ (CH2) and SPICS# (CH1) during one-wire reset" width=0.8\textwidth
    \image html  ow_reset.png "DQ (CH2) and SPICS# (CH1) during one-wire reset"

    Trace 2 of the figure shows first the default high level, then low level
    during reset duration, then high level as the EC releases the bus,
    then the response from the one-wire device (note the slightly higher 
    voltage for low level) then the device  releasing the bus again and
    finally the first bit being written to the device.
    Trace 1 shows the SPI chip select which shows activity or low level
    when the EC fetches new instructions.

    \image latex ow_write_bit.png "DQ (CH2) and SPICS# (CH1) during one-wire write" width=0.8\textwidth
    \image html  ow_write_bit.png "DQ (CH2) and SPICS# (CH1) during one-wire write"

    This figure is split in an upper and a lower half with the lower half
    being a zoom of the upper one.
    The upper left quadrant shows the same situation as the previous figure for
    one-wire reset. In the upper right quadrant the 2nd and 3rd bit are selected
    in a box which is then shown zoomed below.
    There is a long and a short and another short low intervall shown (for writing
    a 0 then a 1 then another 1)

    \image latex ow_read_bit.png "DQ (CH2) and SPI chip select (CH1) during one-wire read" width=0.8\textwidth
    \image html  ow_read_bit.png "DQ (CH2) and SPI chip select (CH1) during one-wire read"

    This figure uses the same display mode as the previous figure but shows a
    zoom of a 1 and a 0 bit being read. Like in the first figure (one-wire reset)
    a slightly higher zero level can be seen when the DS2756 (and not the kb3700)
    pulls down DQ.
    The activity on the SPI CS line can be used as an indicator when the
    low/high bit is sampled by the EC.
    The high level on SPICS# of about 23us corresponds to the EC being in
    sleep mode within the main loop.
    This means that the one-wire routine does not occupy the EC during reading
    of bits completely and that about 1/3rd of the CPU power is left for other
    things. For writing bits (previous diagram) only about 1/10th of the CPU
    power is available (which is nice to have but not sufficient to serve the
    11520 interrupts per second the UART RX or TX can generate).
 */

#include <stdbool.h>
#include "kb3700.h"
#include "ds2756.h"
#include "idle.h"
#include "one_wire.h"
#include "states.h"
#include "timer.h"
#include "uart.h"

/* see also: http://www.ibutton.com/
 */

#define FLAG_BUSY  (0x80) /* defining flags */
#define FLAG_RESET (0x40)
#define TRANSFER_BUSY (transfer_state & FLAG_BUSY)  /* maybe also: (TR1) */

static volatile enum
{
    T_STATE_INIT = FLAG_BUSY | FLAG_RESET | 0x02,
    T_STATE_RW   = FLAG_BUSY              | 0x08,
    T_STATE_IDLE = 0x00
} transfer_state;

volatile unsigned char __pdata ow_transfer_buf[16]; /* <=16 allows use of nibbles in transfer_cnt */
static volatile unsigned char __pdata *transfer_ptr;
static volatile unsigned char __pdata transfer_cnt; /* upper nibble for TX, lower nibble for RX */

// Timings for DS2756 from data sheet (050806) Pag 3 of 26

// TBD check timing against: http://www.maxim-ic.com/appnotes.cfm/appnote_number/126

#define T_REC_USED   (2)

//! bit mask for the 1-wire line (DQ)
#define DQ (0x04)

//! \warning protect accesses to GPIOEOE0
/*! see http://en.wikipedia.org/wiki/Atomic_operation */
#define DQ_HIGH() do {GPIOEOE0 &= ~DQ;} while(0)
#define DQ_LOW()  do {GPIOEOE0 |=  DQ;} while(0)
#define DQ_IS_HIGH   (GPIOEIN0 & DQ)
#define DQ_IS_LOW    (!DQ_IS_HIGH)

//! us has to be >1
#define SET_TIMER1_NEXT_EVENT_US(us) do                                      \
    {                                                                        \
        /* would have expected to /12 instead /24 seems to be ok */          \
        TMR1 = -(unsigned int)((((SYSCLOCK/1000u)/24u)*(us)+500)/1000u);     \
        TF1 = 0;                                                             \
    }                                                                        \
    while(0)


bool ow_busy()
{
    return TRANSFER_BUSY;
}


void ow_dump()
{
    putspace();
    puthex( transfer_state );
    putspace();
    puthex( transfer_cnt );
}

//! Prepare One Wire BUS transfer
/*! using transfer_buf[] both as output and input buffer
    \see TRANSFER_BUSY (but do not busy wait on it)
 */
void ow_transfer_init( unsigned char num_tx, unsigned char num_rx )
{
    if( /* basic check if parameter is OK */
        num_rx > sizeof ow_transfer_buf ||
        num_tx > sizeof ow_transfer_buf ||

        /* Do not yet allow some commands, this is not really secure: */
        ow_transfer_buf[0] == 0x6a || ow_transfer_buf[0] == 0x6c || ow_transfer_buf[0] == 0x48 ||
        ow_transfer_buf[1] == 0x6a || ow_transfer_buf[1] == 0x6c || ow_transfer_buf[1] == 0x48 ||

        ow_busy()


        /* Host should have noticed _any_ previous error.
           If the host has seen (and cleared) the flags
           he may proceed on his own resposibility: */
//        data_ds2756.error.c

//     /* you want to write and do not know whom you are writing? */
//     (!data_ds2756.serial_number_valid && (WRITE LOCK COPY))

      )
    {
        data_ds2756.error.internal_error = 1;
        return;
    }

    /* setting up info for the IRQ */
    transfer_state = T_STATE_INIT;
    transfer_ptr = &ow_transfer_buf[0];
    transfer_cnt = (num_tx<<4) | num_rx;

    /* preparing IRQ */
    DQ_HIGH();

    SET_TIMER1_NEXT_EVENT_US(T_REC_USED);
    TIMER1_IRQ_ENABLE();
    TR1 = 1;

//while(ow_busy())
//    { putcrlf(); ow_dump();};
}


#if 0
struct ow_transfer_type __xdata * __pdata owt_active;

// it would be nicer to directly work on the struct
// but within the IRQ the resources are not there.
void ow_transfer_init_nice_but_too_bulky( struct ow_transfer_type __xdata *owt )
{
    unsigned char i;
    unsigned char tx_len = owt->TX_len;

    for(i=0; i<tx_len; i++)
        ow_transfer_buf[i] = owt->buf[i];

    owt->error.c = data_ds2756.error.c;
    owt_active = owt;
    ow_transfer_init( owt->TX_len, owt->TX_len );
}

//! transfer the (quickly accessable) data used in IRQ to the xdata memory
/*! (this could be done within IRQ but implemented in C uses too much resources)
 */
void ow_finish()
{
    unsigned char rx_len = owt_active->RX_len;
    unsigned char __xdata *ptr = owt_active->buf;
    unsigned char i;

    for(i=0; (unsigned char)i < rx_len; i++)
        *ptr++ = ow_transfer_buf[i] ;
}
#endif


void ow_init()
{
    /* setting data to be output to low! (switching is done by enabling/disabling drivers) */
    GPIOED0  &= ~DQ;
    GPIOEIN0_0xfc64 |=  DQ;

    DQ_HIGH();
}


void timer1_init()
{
    TR1 = 0;
    TH1 = 0;
    TL1 = 0;
    TMOD &= 0x0f;
    TMOD |= 0x10; /* 16 bit timer */
    TF1 = 0;
    PT1 = 1; /* high priority interrupt!! */
}


//#define DEBUG_TOGGLE do{GPIOD10 ^= 0x40;} while(0)      /* a boguous hack... */
#define DEBUG_TOGGLE do{} while(0)


/*! timer IRQ that handles One-Wire communication

    This routine is ugly. It does busy wait during the interrupt.
    It does not allow IRQ to be disabled for longer than x us!

    Stack footprint on top (!!!) of other IRQ stack footprints:
    2 for return address
    x for registers DPH, DPL, ACC, PSW

    The resource usage for this one-wire routine is substantial 
    (in terms of data memory and of the time global IRQ may be disabled)
 */
void timer1_interrupt(void) __interrupt(3) __using(1)
{

#if 0
   /* if short on data memory use keyword "__naked" and do this
      (and the inverse at the exit of the IRQ).
      Or recode the complete IRQ in assembler */
   __asm
       push     psw
       mov      psw,#0x08    ; switching to register bank 1
       mov      r7,a         ; instead of pushing onto stack, store in unused register
       mov      r6,dph
       mov      r5,dpl
       ; r1 and r4 in bank 1 are eventually free to use for transfer_state et al.
   __endasm;
#endif

    if( !(transfer_state & FLAG_RESET) )
    {
        /* FLAG_RESET not set, should be a read or a write */
        if( transfer_state )
        {
            /* both TX and RX start with at least 1 us DQ low */
            DQ_LOW();

            if( transfer_cnt & 0xf0 ) /* is it a write? */
            {
                /* TX */

                unsigned char c;

                c = *transfer_ptr;
                if( c & 0x01 )
                    DQ_HIGH();

                DEBUG_TOGGLE;

                /* note: intentionally no attempt to be quicker in case a 1 was written */
                SET_TIMER1_NEXT_EVENT_US( 49 );

                /* roll byte - after 8 iterations it is there again */
                c = (c >> 1) | (c << 7);
                *transfer_ptr = c;

                /* all bits of the byte transferred? */
                if( !(--transfer_state & 0x0f) )
                {
                    /* decrement upper nibble */
                    transfer_cnt -= 0x10;
                    if( transfer_cnt & 0xf0 )
                    {
                        /* proceed to next byte to transmit */
                        transfer_ptr++;
                        transfer_state |= 0x08;
                    }
                    else
                    {
                        if( transfer_cnt )
                        {
                            /* proceed to the receiving part */
                            transfer_ptr = &ow_transfer_buf[0];
                            transfer_state |= 0x08;
                        }
                        else
                        {
                            /* seems to have no receiving part */
                            transfer_state = T_STATE_IDLE;
                            TIMER1_IRQ_DISABLE();
                            busy = 1; /* done. Do not sleep now */
                            while( !TF1 )
                                ;
                            TR1 = 0;
                        }
                    }
                }

                while( TH1 )
                    ;

                /* this delay decides how much CPU power is left when
                   one-wire writes are active. And how long IRQ may
                   be disabled. Take care of t(slot),max */
                SET_TIMER1_NEXT_EVENT_US( 20 );  // ooops... this is short!

                DQ_HIGH();
            }
            else
            {
                /* RX */

                unsigned char c;

                DQ_HIGH();

                SET_TIMER1_NEXT_EVENT_US( 2 );

                c = *transfer_ptr;
                c >>= 1;

                while( TH1 )
                    ;

                /* this delay decides how much CPU power is left when
                   one-wire reads are active. And how long IRQ may
                   be disabled. Take care of t(slot),max */
                SET_TIMER1_NEXT_EVENT_US( 40 );

                DEBUG_TOGGLE;

                if( !DQ_IS_LOW )
                    c |= 0x80;
                *transfer_ptr = c;

                /* all bits within the byte transferred ? */
                if( !(--transfer_state & 0x0f) )
                {
                    /* next byte */
                    transfer_cnt--;

                    if( transfer_cnt ) /* upper nibble known to be zero */
                    {
                        /* not yet done */
                        transfer_ptr++;
                        transfer_state |= 0x08;
                    }
                    else
                    {
                        /* done */
                        transfer_state = T_STATE_IDLE;
                        TIMER1_IRQ_DISABLE();
                        while( !TF1 )
                            ;
                        TR1 = 0;
                        /* new data completely read. Do not sleep now */
                        busy = 1;
                    }
                }
            }
        }
        else /* if( transfer_state ) */
        {
            /* transfer_state = T_STATE_IDLE;  redundant */
            TIMER1_IRQ_DISABLE();
            TR1 = 0;
        }
    }
    else /* if(!(transfer_state & FLAG_RESET)) */
    {
        /* this is the reset */
        switch( transfer_state & 0x03 )
        {
            case 2:
                /* check for hardware fault part 1.
                   Note, the check here might not notice a small
                   capacitor connected to DQ.
                   (Ignoring that failure mode and hoping CRC will notice)
                 */
                if( DQ_IS_LOW )
                {
                    data_ds2756.error.line_stuck_low = 1;
                    /* giving up. Setting state for a quick exit */
                    transfer_state = T_STATE_IDLE;
                }
                else
                {
                    /* beginning of reset */
                    DQ_LOW();
                    SET_TIMER1_NEXT_EVENT_US( 480 + 1 );
                    transfer_state--;
                }
                break;

            case 1:
                /* check for hardware fault part 2.
                   Note, the check here might not notice if the
                   Output FET of the one-wire device has degraded due to ESD.
                   (Ignoring that failure mode and hoping CRC will notice)
                 */
                if( DQ_IS_HIGH )
                {
                    data_ds2756.error.line_stuck_high = 1;
                    /* giving up. Setting state for a quick exit */
                    transfer_state = T_STATE_IDLE;
                }
                else
                {
                    /* end of reset */
                    DQ_HIGH();
                    SET_TIMER1_NEXT_EVENT_US( 60 + 1 );
                    transfer_state--;
                }
                break;

            case 0:
                if( DQ_IS_LOW )
                {
                    data_ds2756.error.no_device = 0;
                    transfer_state = T_STATE_RW;
                }
                else
                {
                    data_ds2756.error.no_device = 1;
                    data_ds2756.serial_number_valid = 0; /**< being rude */
                    transfer_state = T_STATE_IDLE;
                }

                data_ds2756.error.no_device_flag_is_invalid = 0;
                /* wait until one-wire device freed the bus again */
                SET_TIMER1_NEXT_EVENT_US( 240 + 1 );
                break;

//                case 0xff: /* dummy case to inhibit jumptable generation,
//                              might save 2 byte stack space */


            default:
                /* internal error */
                transfer_state = T_STATE_IDLE;

        }
    }

    DEBUG_TOGGLE;
}

