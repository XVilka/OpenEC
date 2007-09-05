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

/*
   Completely untested.
 */

#include <stdbool.h>
#include "kb3700.h"
#include "ds2756.h"
#include "states.h"
#include "timer.h"

/* see also: http://www.ibutton.com/
 */

#define FLAG_BUSY  (0x80) /*defining flags*/
#define FLAG_RESET (0x40)
#define FLAG_WRITE (0x20)
#define TRANSFER_BUSY (transfer_state & FLAG_BUSY)

static volatile
enum
{
    T_STATE_INIT_WRITE = 0xe0 | 7, /*masking*/
    T_STATE_INIT_READ  = 0xc0 | 7,
    T_STATE_READ       = 0x80 | 0x18 | 0x07,
    T_STATE_WRITE      = 0xa0 | 0x08 | 0x07,
    T_STATE_IDLE       = 0x00
} transfer_state;

static volatile unsigned char transfer_byte;

//! Timings for DS2756 from data sheet (050806) Pag 3 of 26
#define T_REC_MIN   (  1)
#define T_LOW0_MIN  ( 60)
#define T_LOW1_MIN  (  1)
#define T_RSTH_MIN  (480)
#define T_RSTL_MIN  (480)
#define T_PDH_MIN   ( 15)
#define T_PDL_MIN   ( 60)
#define T_PDL_MAX   (240)
#define T_SLOT_MIN  ( 60)
#define T_SLOT_MAX  (120)

//! The timings we actually use (why are they different?)
#define T_REC_USED   (T_REC_MIN  + 1)
#define T_LOW0_USED  (T_LOW0_MIN + 1)
#define T_LOW1_USED  (T_LOW1_MIN + 1)
#define T_HI_AFTER_LOW0_USED (T_TIME_SLOT_USED - T_LOW0_USED)
#define T_HI_AFTER_LOW1_USED (T_TIME_SLOT_USED - T_LOW1_USED)
#define T_RSTH_USED  (T_RSTH_MIN + 1)
#define T_RSTL_USED  (T_RSTL_MIN + 1)
#define T_PDH_USED   (T_PDH_MIN  + 1)
#define T_PDL_USED   (T_PDL_MIN  + 1)
#define T_PDL_BEFORE_SAMPLE_USED  (T_PDL_MIN  - 5)
#define T_PDL_AFTER_SAMPLE_USED   (T_PDL_MAX - T_PDL_BEFORE_SAMPLE_USED + 1)
#define T_TIME_SLOT_USED (100)

//! bit mask for the 1-wire line (DQ)
#define DQ (0x04)

//! \warning protect accesses to GPIOD00
/*! see http://en.wikipedia.org/wiki/Atomic_operation */
#define DQ_HIGH() do {GPIOED0 |=  DQ;} while(0) /*High DQ 
masking*/
#define DQ_LOW()  do {GPIOED0 &= ~DQ;} while(0) /*low DQ masking 
*/
#define DQ_IS_HIGH   (GPIOEIN0 & DQ) /*DQ is High Pin3 of 
KB3700*/
#define DQ_IS_LOW    (!DQ_IS_HIGH)

#define GPIOD00_ATOMIC_ON()  do{ TIMER1_IRQ_DISABLE(); } while (0)
#define GPIOD00_ATOMIC_OFF() do{ TIMER1_IRQ_ENABLE();  } while (0)

#define TIMER1_IRQ_ENABLE()  do {ET1=1;} while(0)
#define TIMER1_IRQ_DISABLE() do {ET1=0;} while(0)

#define SET_TIMER_NEXT_EVENT_US(us) do \
    { \
        TMR1 = -(unsigned int)(((SYSCLOCK/1000u)/12u)*(us)/1000u); \
        TF1 = 0; \
    } \
    while(0)


__bit ow_busy()
{
    return TRANSFER_BUSY;
}

//! Prepare single byte read and let the IRQ do the rest
/*! \see TRANSFER_BUSY (but do not busy wait on it)
 */
void ow_read_byte_init()
{
    /* well this byte obviously has several uses... */
    transfer_state = FLAG_BUSY | 0x18 | 0x07; // !!

    /* 0xff should be a good default.
       If the device would not respond for whatever
       reason we'd read 0xff anyway */
    transfer_byte = 0xff;

    SET_TIMER_NEXT_EVENT_US(T_REC_USED);
    TIMER1_IRQ_ENABLE();

}


//! Prepare single byte write and let the IRQ do the rest
/*! \see TRANSFER_BUSY (but do not busy wait on it)
 */
void ow_write_byte_init(unsigned char b)
{
    if( !data_ds2756.serial_number_valid )
    {
        /* you want to write and do not know whom you are writing? */
        data_ds2756.error.illegal_write = 1;

        /* please fetch a book from the New Age section. */
        return;
    }

    if( data_ds2756.error.c )
    {
        /* Host should have noticed _any_ previous error.
           If the host has seen (and cleared) the flags
           he may proceed on his own resposibility. */
        data_ds2756.error.illegal_write = 1;
        return;
    }

    /* setting up info for the IRQ.
       Obviously this byte has several uses. The uses
       might differ between READ/WRITE/RESET mode.
       Instead of reading some semi-maintained comment
       here, please only trust its usage within the IRQ */
    transfer_state = FLAG_BUSY | FLAG_WRITE | 0x08 | 0x07;
    transfer_byte = b;

    SET_TIMER_NEXT_EVENT_US(T_REC_USED);
    TIMER1_IRQ_ENABLE();
}



unsigned char ow_get_read_byte(void)
{
    // we should _never_ need to enter the polling loop here!!!
    // Now how is this meant?
    // The higher level code (state-machine?)
    // should only call this routine
    // if it is known to succeed immediately.
    //
    // We rely on a quickly running main loop
    if( TRANSFER_BUSY)
    {
        /* set error flag for unclean call */
        data_ds2756.error.busy_wait = 1;

        /* enter polling loop */
        while( TRANSFER_BUSY )
            ;
    }
    return transfer_byte;
}

void ow_reset_and_presence_detect_init(void)
{
    transfer_state = FLAG_BUSY | FLAG_RESET | 6;
    /* would mean "not detected" */
    transfer_byte = 0x00;

    DQ_HIGH();
    SET_TIMER_NEXT_EVENT_US(T_RSTH_USED);
    TIMER1_IRQ_ENABLE();
}

unsigned char ow_reset_and_presence_detect_read(void)
{
     return ow_get_read_byte();
}


/*! timer IRQ that handles (one byte of) One-Wire communication

    This routine is ugly. It does busy wait during the interrupt.
    It does not allow other IRQ to last longer than xx us.

    (We could use high priority IRQ to avoid this
    but as post B4 XOs seem to have hardware support for One-Wire
    that probably is not worth it)

    Stack footprint:
    2 for return address
    5 for registers R2, DPH, DPL, ACC, PSW
    x for the computed jump within the switch statement
 */
void timer1_interrupt(void) __interrupt (0x04)
{

    switch( transfer_state>>4 )
    {
        case T_STATE_INIT_WRITE>>4:

            /* a reset before a read or write */
            switch( transfer_state & 0x07 )
            {
                case 6:
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
                        break;
                    }
                    /* beginning of reset */
                    DQ_LOW();
                    SET_TIMER_NEXT_EVENT_US( 480 + 1 );
                    transfer_state--;
                    break;

                case 5:
                    /* check for hardware fault part 2.
                       Note, the check here might not notice if the
                       Output FET has degraded due to ESD.
                       (Ignoring that failure mode and hoping CRC will notice)
                     */
                    if( DQ_IS_HIGH )
                    {
                        data_ds2756.error.line_stuck_high = 1;
                        /* giving up. Setting state for a quick exit */
                        transfer_state = T_STATE_IDLE;
                        break;
                    }
                    /* end of reset */
                    DQ_HIGH();
                    SET_TIMER_NEXT_EVENT_US( 480 + 1 );
                    transfer_state--;
                    break;

                case 4:
                    DQ_LOW();
                    SET_TIMER_NEXT_EVENT_US( 60 + 1 );
                    transfer_state--;
                    break;

                case 3:
                    DQ_HIGH();
                    SET_TIMER_NEXT_EVENT_US( 240 - 60 - 5 );
                    transfer_state--;
                    // busy wait during IRQ :(
                    while (!TF1)
                        ;
                    // intentionally no break;

                case 2:
                    if( DQ_IS_LOW )
                    {
                        transfer_byte++; /**< known to have been 0x00 before */
                        data_ds2756.error.no_device = 0;
                    }
                    else
                    {
                        data_ds2756.error.no_device = 1;
                        data_ds2756.serial_number_valid = 0; /**< being rude */
                    }
                    data_ds2756.error.no_device_flag_is_invalid = 0;
                    SET_TIMER_NEXT_EVENT_US( 60 + 5  + 1 );
                    transfer_state--;
                    break;

                case 0xff: /* dummy case to inhibit jumptable generation,
                              might save 2 byte stack space */

                case 1:
                    transfer_state = T_STATE_WRITE;
                    break;

                default:
                    transfer_state = T_STATE_IDLE;

            }
            break;


        case T_STATE_WRITE >> 4:

            DQ_LOW();

            SET_TIMER_NEXT_EVENT_US( 2 );
            while( !TF1 )
                ;
            if( transfer_byte & 0x01 )
                DQ_HIGH();

            /* roll byte - after 8 iterations it is there again */
            transfer_byte = (transfer_byte>>1) | (transfer_byte<<7);

            /* note: intentionally no attempt to be quicker in case a 1 was written */
            SET_TIMER_NEXT_EVENT_US( 60 - 2 );
            while( !TF1 )
                ;
            DQ_HIGH();

            /* this delay decides how much CPU power is left when
               one-wire writes are active. Take care that t(slot),max
               is in spec even if another IRQ hits in between */
            SET_TIMER_NEXT_EVENT_US( 20 );

            if( transfer_state & 0x07 )
            {
                transfer_state--;
            }
            else
            {
                while( !TF1 )
                    ;
                transfer_state =  T_STATE_IDLE;
                TIMER1_IRQ_DISABLE();
            }
            break;


        case T_STATE_READ >> 4:

            DQ_LOW();
            SET_TIMER_NEXT_EVENT_US( 2 );
            while( !TF1 )
                ;
            DQ_HIGH();

            SET_TIMER_NEXT_EVENT_US( 14 );
            while( !TF1 )
                ;

            transfer_byte <<= 1;
            if( !DQ_IS_LOW )
                transfer_byte++;

            SET_TIMER_NEXT_EVENT_US( 60 - 14 - 2 );

            if( transfer_state & 0x07 )
            {
                transfer_state--;
            }
            else
            {
                while( !TF1 )
                    ;
                transfer_state =  T_STATE_IDLE;
                TIMER1_IRQ_DISABLE();
            }

            break;
    }
}

