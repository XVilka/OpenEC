/*-------------------------------------------------------------------------
   port_0x6c.c - Host communication routines for the EC of the OLPC

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
#include "battery.h"
#include "ds2756.h"
#include "matrix_3x3.h"
#include "port_0x6c.h"
#include "states.h"
#include "timer.h"

#define IBF 0x02
#define OBF 0x01

//! Transfer pointer. Changed within IRQ
/*! The pointer itself is within __data memory.
    It points to a value in __xdata (__pdata) memory.
 */
unsigned char __xdata * volatile __data transfer_ptr;

//! number and direction of bytes (still) to transfer to or from host
/*! if uppermost bit is set the transfer if from Host to EC
    \see FLAG_TRANSFER_FROM_HOST
 */
static volatile unsigned char __data transfer_countdown;

static unsigned char __xdata six_zero_bytes[6];

static unsigned char __pdata command;


#if defined(SDCC)
# pragma disable_warning 126
#endif

//! src or dest may be in __xdata or __pdata memory
#define TRANSFER_TO_HOST_INIT(src, len) \
    do \
    { \
        LPC68DAT = (unsigned char)*(src); \
        if( (len) == 1) \
        {   /* E5a: only a single byte? No more OBF interrupts. */ \
            LPC68CFG &= ~OBF; \
        } \
        else \
        { \
            /* E5b */ \
            /* prepare for more bytes */ \
            LPC68CFG |= OBF; \
            transfer_ptr = (unsigned char __xdata *)(src)+1; \
            transfer_countdown = (len)-1; \
        } \
    } while(0)

//! src or dest may be in __xdata or __pdata memory
#define TRANSFER_FROM_HOST_INIT(dest, len) \
    do \
    {\
        /* E5c */ \
        LPC68CFG &= ~OBF; \
        transfer_ptr = (unsigned char __xdata *)(dest); \
        transfer_countdown = (len) | FLAG_TRANSFER_FROM_HOST; \
    } while(0)

#define TRANSFER_END() \
     /* E5d */ \
     do \
        LPC68CFG &= ~OBF; \
     while(0)

#define FLAG_TRANSFER_FROM_HOST 0x80

//! init hardware and set variables to default state
void host_interface_init(void)
{
    LPC68CFG |= (0x80 | IBF);
    LPC68CFG &= ~OBF;

    HOST_INTERFACE_INTERRUPT_ENABLE;
}


//! port_0x6c commands. Handle most directly, defer others to main loop
/*! The comments starting with En: or Cn: and those describing the
    switch cases are a copy and paste from:
    http://wiki.laptop.org/go/Revised_EC_Port_6C_Command_Protocol
    (try to keep in sync)

    The code on Linux that communicates with this routine is
    kept at: <insert link here>

    Don't call subroutines here.

    Stack footprint:
    2 for return address
    8 for registers R0, R2, R3, R4, DPH, DPL, ACC, PSW
    2 for the computed jump within the switch statement
 */
void host_interface_interrupt(void) __interrupt(0x0e)
{

    /* reset IRQ pending flag */
    P0IF &= ~0x20;

    /* new command received? */
    if(LPC68CSR & 0x40)
    {
        /* E1: EC receives IBF interrupt, checks register 0xfe9e bit 6 and observes that it
           was 1, hence a new command.
           E2: EC cancels any previous commands by setting its state variables
           to "new command" state.
         */
        transfer_countdown = 0;

         /* E3: If OBF=1, EC clears it by writing 0x01 to reg 0xfe9e.  This is the
            crucial interlock that makes the protocol predictable - this must
            be done prior to clearing IBF in a later step.  This step discards
            any residue from incomplete earlier commands, and step E2 cancels
            any tendency for earlier command to continue generating data.
          */
        if(LPC68CSR & 0x40)
            LPC68CSR = 0x01;

        /* E4: EC reads the command byte from reg 0xfe9f, clears IBF by writing 2
           to reg 0xfe9e,  decodes the command, and sets a state variable accordingly.
         */
        command = LPC68DAT;
        LPC68CSR = 0x02; /* does the host notice this or the reading of LPC68DAT? */

        /* write new input to the debugging area (if enabled) */
        STATES_UPDATE(command, command);

        switch( command )
        {
            case 0x03:/* 0x03 SPI write protect */
                TRANSFER_END();
                break;
            case 0x10: /* Read voltage (2 bytes) */
                TRANSFER_TO_HOST_INIT(&six_zero_bytes, 2); // dummy
                break;
            case 0x11: /* Read current (2 bytes) */
                TRANSFER_TO_HOST_INIT(&six_zero_bytes, 2); // dummy
                break;
            case 0x12: /* Read ACR (2 bytes) */
                TRANSFER_TO_HOST_INIT(&six_zero_bytes, 2); // dummy
                break;
            case 0x13: /* Read battery temperature (2 bytes) */
                TRANSFER_TO_HOST_INIT(&six_zero_bytes, 2); // dummy
                break;
            case 0x14: /* Read ambient temperature (2 bytes) */
                TRANSFER_TO_HOST_INIT(&six_zero_bytes, 2); // dummy
                break;
            case 0x15:
                /* Read battery status (1 byte)
                   o Bit 0: 1: battery exists
                   o Bit 1: 1: battery full charged
                   o Bit 2: 1: battery low
                   o Bit 3: 1: battery destroyed
                   o Bit 4: 1: AC in
                   o Bits 5-7 Not defined
                   */
                TRANSFER_TO_HOST_INIT(&battery.status_0x15.b, 1);
                break;
            case 0x16: /* Read Battery State of Charge (SOC) (1 byte) */
                // TBD: cooperate with command 0x1a
                TRANSFER_TO_HOST_INIT(&battery.soc_0x16, 1);
                break;
            case 0x17: /* Read Battery gas gauge chip serial number (6 bytes) */
                /* assuming the EC reads it by itself. */
                if( data_ds2756.serial_number_valid )
                    TRANSFER_TO_HOST_INIT(&data_ds2756.serial_number[0], 6);
                else
                    /* 0x000000000000 denotes no serial number available */
                    TRANSFER_TO_HOST_INIT(&six_zero_bytes, 6);
                break;
            case 0x18:
                /* Read Battery gas gauge EEPROM databyte
                   o Cmd data is the EEPROM address of the byte requested
                   o Result is 1 byte of EEPROM data
                 */
                TRANSFER_FROM_HOST_INIT(data_ds2756.eeprom_address_from_host, 1);
                break;
            case 0x19: /* Read board id (1 byte) */
                TRANSFER_TO_HOST_INIT(&six_zero_bytes, 1); // dummy
                break;
            case 0x1a:
                /* Read SCI source (1 byte)
                   o 0x01 Game button
                   o 0x02 Battery Status Change. Generated for any of:
                         + AC plugged/unplugged
                         + Battery inserted/removed
                         + Battery Low
                         + Battery full
                         + Battery destroyed
                   o 0x04 Battery SOC Change
                   o 0x08 Battery subsystem error
                   o 0x10 Ebook mode change
                   o 0x20 Wake up from Wlan
                 */
                TRANSFER_TO_HOST_INIT(&six_zero_bytes, 1); // dummy
                break;
            case 0x1b: /* Write SCI mask (1 byte) */
                // would this be SCIE0,SCIE1,SCIE3?
                TRANSFER_TO_HOST_INIT(&six_zero_bytes, 1); // dummy
                break;
            case 0x1c: /* Read SCI mask (1 byte) */
                TRANSFER_TO_HOST_INIT(&six_zero_bytes, 1); // dummy
                break;
            case 0x1d: 
                 /* Game key status (2 bytes) 9 bits of key status. 1 indicates key is depressed.
                   o Bit 1: KEY_LR_R
                   o Bit 2: KEY_RT_R
                   o Bit 3: KEY_UP_L
                   o Bit 4: KEY_DN_L
                   o Bit 5: KEY_LF_L
                   o Bit 6: KEY_RT_L
                   o Bit 7: KEY_COLOR/MONO
                   o Bit 8: KEY_UP_R
                   o Bit 9: KEY_DN_R
                  */
                  TRANSFER_TO_HOST_INIT(&cursors.game_key_status, 2);
                break;
            case 0x1e: /* Set date (day/mon/year)
                   o Need details for using this.
                   */
                //TRANSFER_FROM_HOST_INIT(buffer, 4); // dummy
                break;
            case 0x1f:
                 /* Read battery subsystem error code (1 byte)
                   o 0x02 Pack info fail (LiFePO4 & NiMH)
                   o 0x04 Over voltage checking fail (LiFePO4)
                   o 0x05 Over temperature (58C) (LiFePO4)
                   o 0x06 Gauge stop or sensor break (LiFePO4 & NiMH)
                   o 0x07 Sensor out of control (NiMH)
                   o 0x09 Battery ID fail & temperature > 52C
                   o 0x10 ACR fail (NiMH)
                  */
                TRANSFER_TO_HOST_INIT(&battery.errorcode_0x1f, 1);
                break;
            case 0x20: /* Init NiMH Battery */
                // TBD: signal to battery.c
                TRANSFER_END();
                break;
            case 0x21: /* Init LiFePO4 Battery */
                // TBD: signal to battery.c
                TRANSFER_END();
                break;
            case 0x23: /* Set WLAN Power on/off */
                // is an additional byte following?
                break;
            case 0x24: /* Wake up WLAN */
                // how to?
                // atomic! see http://en.wikipedia.org/wiki/Atomic_operation
                TRANSFER_END();
                break;
            case 0x25: /* WLAN reset */
                // how to?
                TRANSFER_END();
                break;
            case 0x26: /* DCON power enable/disable */
                // is an additional byte following?
                break;
            case 0x2a:
                /* Read EBOOK mode
                   Bit 0 EBOOK sensor status
                 */
                {
                    // TBD: cooperate with command 0x1a
                    static unsigned char __xdata ebook_mode;
                    ebook_mode = (GPIOIN18&0x02)?0x01:0x00; /* OK? */
                    TRANSFER_TO_HOST_INIT(&ebook_mode, 1);
                }
                break;
            case 0x2b:
                /* Read power rail status
                   o Bit 0 WLAN status
                   o Bit 1 DCON status
                 */
                {
                    static unsigned char __xdata power_rail_status;
                    power_rail_status  = (GPIOIN00&0x02)?0x01:0x00; /* OGPIOIN00 or OGPIOD00? */
                    power_rail_status |= (GPIOIN08&0x20)?0x02:0x00;
                    TRANSFER_TO_HOST_INIT(&power_rail_status, 1);
                }
                break;
        }
    }
    else /* new data received! */
    {
        /* E8: When the CPU writes the byte to the data register, the EC will
           receive the IBF interrupt and notice (via reg 0xfe9e bit 6 = 0) that
           the write was to the data register.  The EC then reads the byte from
           reg 0xfe9f, clears IBF by writing 2 to reg 0xfe9e, handles the byte
           according to the command sematics, and goes to step E5 to continue
           processing the command.
         */

        if( transfer_countdown & FLAG_TRANSFER_FROM_HOST ) /* uppermost bit codes direction */
           *transfer_ptr++ = LPC68DAT;
        else
           LPC68DAT = *transfer_ptr++;

        if(((unsigned char)~FLAG_TRANSFER_FROM_HOST) & --transfer_countdown)
        {
             /* transfer not yet completed - more data please */
             LPC68CSR = 0x02;

             /* return quickly */
             return;
        }


        if( !(FLAG_TRANSFER_FROM_HOST & transfer_countdown) )
        {
             /* if a transfer to host has completed there is
                nothing more to be done. */

             /* return quickly */
             return;
        }

        /* Now we have completely received the data from
           the host. What to do with the it? */
        {
            unsigned char tc = command;

            switch( tc )
            {
                case 0x18:
                    /* Read Battery gas gauge EEPROM databyte
                       o Cmd data is the EEPROM address of the byte requested
                       o Result is 1 byte of EEPROM data
                     */

                     /* now it is known which eeprom byte should be read */
                    data_ds2756.eeprom_host_has_a_request = 1;

                     /* the story does not end here though:
                        The byte has to be read and then transferred
                        back to the host. How to handle this?
                      */
                    break;
                case 0x1e:
                    /* Set date (day/mon/year)
                       o Need details for using this.
                       */
                    //TRANSFER_FROM_HOST_INIT(buffer, 4); // dummy
                    // do the equivalent of set_time(*buffer);
                    // but do not call the subroutine
                    break;
                case 0x23: /* Set WLAN Power on/off */
                    break;
                case 0x26: /* DCON power enable/disable */
                    break;
            }
        }
    }
}


//! routine for nonIRQ code to respond with data to a request from within IRQ
void respond_to_host(unsigned char my_command,
                     unsigned char __xdata * my_transfer_ptr,
                     unsigned char my_len)
{
    HOST_INTERFACE_INTERRUPT_DISABLE;

    /* only if host has not issued a new command */
    if( my_command == command )
    {
        /* Is this enough to avoid a race condition?
           A new command may have been written to 0x6c
           immediately after we tested it.
           (Should host explicitly send a break command
           before issuing a new one?) */
        TRANSFER_TO_HOST_INIT( my_transfer_ptr, my_len);
    }
    else
    {
        /* ignore the outdated request to host.
           set a flag "host was to impatient" or
           "sorry we were too slow" */
    }

    HOST_INTERFACE_INTERRUPT_ENABLE;
}



#if 0
 Cut and paste from:
 http://wiki.laptop.org/go/Revised_EC_Port_6C_Command_Protocol

 C1: CPU waits until IBF=0, then sends the command by writing it to port 0x6c.

 E1: EC receives IBF interrupt, checks register 0xfe9e bit 6 and observes that it
 was 1, hence a new command.

 E2: EC cancels any previous commands by setting its state variables
 to "new command" state.

 E3: If OBF=1, EC clears it by writing 0x01 to reg 0xfe9e.  This is the
 crucial interlock that makes the protocol predictable - this must
 be done prior to clearing IBF in a later step.  This step discards
 any residue from incomplete earlier commands, and step E2 cancels
 any tendency for earlier command to continue generating data.

 E4: EC reads the command byte from reg 0xfe9f, clears IBF by writing 2
 to reg 0xfe9e,  decodes the command, and sets a state variable accordingly.


 E5a: If the next step of the command is to send data to the CPU and
 there are no more steps after that, EC disables the OBF interrupt,
 puts the data byte in the data register, sets state to "idle",
 and returns from the protocol handler.

 E5b: If the next step of the command is to send data to the CPU and
 there are more steps after that, EC enables the OBF interrupt, puts
 the first data byte in the data register, and returns from the
 protocol handler.  (When the CPU later reads the byte, the EC will
 receive the OBF interrupt as described in E7.)

 E5c: If the next step of the command is to receive data from the
 CPU, EC disables the OBF interrupt, and returns from the protocol
 handler. (When the CPU later writes the byte to the data register,
 the EC will perform step E8.)

 E5d: If there is no more data to be sent or received, the EC
 disables OBF interrupt, sets state to idle, and returns from the
 protocol handler.

C2: If the next stage of the command is an EC-to-CPU data byte, the CPU polls port 0x6c until IBF,OBF=0,1, then reads the data byte from port 0x68. IBF must be included in the test, since it makes the E3 interlock work. If there is leftover data that causes OBF to start out as 1, step E3 will drive OBF to 0 while IBF is still 1, so the IBF,OBF=0,1 test will not succeed until the EC reaches step E5a or E5b.

 E6: If that byte was the last thing in the command sequence, the EC
 will not receive an OBF interrupt because step E5a disabled the
 interrupt.  The EC is already back in idle state, so it does not
 need notification that the CPU read the byte.

 E7: If there are more bytes after this to send or receive, the EC
 will receive an OBF interrupt (because of step E5b) and go to step
 E5 to continue processing the command.

C3: If the next stage of the command is a CPU-to-EC data byte, the CPU polls port 6c until IBF=0, then writes the data byte to port 0x68.

 E8: When the CPU writes the byte to the data register, the EC will
 receive the IBF interrupt and notice (via reg 0xfe9e bit 6 = 0) that
 the write was to the data register.  The EC then reads the byte from
 reg 0xfe9f, clears IBF by writing 2 to reg 0xfe9e, handles the byte
 according to the command sematics, and goes to step E5 to continue
 processing the command.

#endif
