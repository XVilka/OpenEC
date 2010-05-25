/*-------------------------------------------------------------------------
   sfr_dump.c - dump register settings

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

/*
   The code and data here uses about 3 kByte.
   Most likely it will not be part of the final firmware.
 */


#include <stdbool.h>
#include "chip.h"
#include "adc.h"
#include "uart.h"
#include "sfr_rw.h"

typedef struct
{
    unsigned char __code *name;
    unsigned char __xdata *address;
    unsigned char len;
} ec_range_type;


static ec_range_type __code ec_range[] =
{
    { "GPIOO",  (unsigned char __xdata *)0xfc00, 0x04 },
    { "GPIOE",  (unsigned char __xdata *)0xfc10, 0x06 },
    { "GPIOD",  (unsigned char __xdata *)0xfc20, 0x06 },
    { "GPIOIN", (unsigned char __xdata *)0xfc30, 0x07 },
    { "GPIOPU", (unsigned char __xdata *)0xfc40, 0x06 },
    { "GPIOOD", (unsigned char __xdata *)0xfc50, 0x04 },
    { "GPIOIE", (unsigned char __xdata *)0xfc60, 0x07 },
    { "GPIOM",  (unsigned char __xdata *)0xfc70, 0x01 },
    { "KBC",    (unsigned char __xdata *)0xfc80, 0x07 },
    { "PWM",    (unsigned char __xdata *)0xfe00, 0x0e },
    { "GPT",    (unsigned char __xdata *)0xfe50, 0x0a },
    { "WDT",    (unsigned char __xdata *)0xfe80, 0x06 },
    { "LPC",    (unsigned char __xdata *)0xfe90, 0x10 },
    { "SPI",    (unsigned char __xdata *)0xfea0, 0x10 },
    { "PS2",    (unsigned char __xdata *)0xfee0, 0x07 },
    { "EC",     (unsigned char __xdata *)0xff00, 0x20 },
    { "GPWUEN", (unsigned char __xdata *)0xff30, 0x04 },
    { "GPWUPF", (unsigned char __xdata *)0xff40, 0x04 },
    { "GPWUPS", (unsigned char __xdata *)0xff50, 0x04 },
    { "GPWUEL", (unsigned char __xdata *)0xff60, 0x04 },
};


typedef struct
{
    unsigned char number;
    unsigned char pin;
    unsigned char __code *ec_name;
    unsigned char __code *alt_out_name;
    unsigned char __code *alt_in_name;
} ec_gpio_type;


static ec_gpio_type __code ec_gpio[] =
{
   /* num  pin  ec_name                 */
    { 0x00,  6, "VR_ON#",               "GA20",           "?" },
    { 0x01,  9, "WLAN_EN",              "KBRST#",         "?" },
    { 0x02, 10, "SWI#",                 "?",              "?" },
    { 0x03, 16, "CLK_EC_LPC",           "?",              "PCICLK" },
    { 0x04, 17, "PCI_RST#",             "?",              "PCIRST#" },
    { 0x05, 22, "SCI#",                 "SCI#",           "?" },
    { 0x06, 23, "TX",                   "PWM0/E51_TXD",   "?" },
    { 0x07, 24, "RX/BAT_L0",            "PWM1/E51_CLK",   "E51_RXD" },

    { 0x08, 25, "EC_EAPD",              "PWM2",           "?" },
    { 0x09, 26, "LED_PWR#",             "PWM3",           "?" },
    { 0x0a, 27, "LED_CHG_R#",           "PWM4",           "?" },
    { 0x0b, 28, "WAKEUP",               "?",              "E51_TMR0" },
    { 0x0c, 29, "PWR_BUT#",             "?",              "E51_TMR1" },
    { 0x0d, 30, "ECRST#",               "E51_TXD",        "ECRST#" },
    { 0x0e, 31, "CHG",                  "PLLCLK32",       "E51_INT0" },
    { 0x0f, 32, "CC0",                  "POR",            "E51_INT1" },

    { 0x10, 37, "WLAN WAKEUP",          "?",              "?" },
    { 0x11, 43, "KBSCLK",               "PSCLK1",         "PSCLK1" },
    { 0x12, 44, "KBSDAT",               "PSDAT1",         "PSDAT1" },
    { 0x13, 45, "TPCLK",                "PSCLK2",         "PSCLK2" },
    { 0x14, 46, "TPDAT",                "PSDAT2",         "PSDAT2" },
    { 0x15, 47, "KEY_OUT_3/ISP_CLK",    "?",              "TEST_CLK" },
    { 0x16, 48, "KEY_OUT_1/ISP_EN#",    "?",              "TP_CLK_TEST" },
    { 0x17, 53, "KEY_OUT_2",            "CLK",            "TP_PLL_TEST" },

    { 0x18, 54, "MIC_AC/ISP_EN2#",      "CLK32MHz(8051)", "TP_ISP_MODE" },
    { 0x19, 55, "EB_MODE",              "CLK16MHz(peri)", "TP_IO_TEST" },
    { 0x1a, 56, "MAIN_ON",              "CLK32MHz(WDT)",  "?" },
    { 0x1b, 57, "SUS_ON",               "?",              "?" },
    { 0x1c,  0, ""      ,               "?",              "?" },
    { 0x1d,  0, "",                     "?",              "?" },
    { 0x1e,  0, ""      ,               "?",              "?" },
    { 0x1f,  0, "",                     "?",              "?" },

    { 0x20,  1, "EC_WP#",               "?",              "?" },
    { 0x21,  2, "CV_SET",               "?",              "?" },
    { 0x22,  3, "DQ",                   "?",              "?" },
    { 0x23,  4, "LED_CHG_G#",           "?",              "?" },
    { 0x24, 17, "KEY_IN_1",             "?",              "?" },
    { 0x25, 18, "KEY_IN_2",             "?",              "?" },
    { 0x26, 19, "KEY_IN_3",             "?",              "?" },
    { 0x27, 20, "POWER_BUTTON#",        "?",              "?" },

    { 0x28, 33, "BAT_L1",               "?",              "?" },
    { 0x29,  0, "",                     "?",              "?" },
    { 0x2a,  0, "",                     "?",              "?" },
    { 0x2b,  0, "",                     "?",              "?" },
    { 0x2c, 49, "SPIWP#",               "?",              "?" },
    { 0x2d, 50, "DCON_EN",              "?",              "?" },
    { 0x2e, 51, "T188(B1,WLAN,41)",     "?",              "?" },
    { 0x2f, 52, "ECPWRRQST",            "?",              "?" },

    { 0x30, 38, "TEMP",                 "?",              "?" },
#if 0
    { 0x31, 39, "WLAN_ENLED#",          "?",              "?" },
#else
    { 0x31, 39, "EC_ID",                "?",              "?" },
#endif
    { 0x32, 40, "ACIN",                 "?",              "?" },
    { 0x33, 34, "ISYS",                 "?",              "?" },
    { 0x34, 35, "CVM",                  "?",              "?" },
    { 0x35, 36, "WAKEUP_EC",            "?",              "?" },

};


//! dumps SFR registers and data memory 
void dump_mcs51( void )
{
    unsigned char i = 0x00;

    do
    {
        if( (i & 0x07) == 0)
        {
            putstring( i & 0x80 ? 
                       "\r\nSFR " :
                       "\r\nDATA " );
            puthex( i );
            putchar(':');
        }
        putspace();
#if defined(SDCC)
        puthex( i & 0x80 ? 
                read_mcs51_sfr( i ) : 
                *(unsigned char __idata *) i );
#else
        puthex(0xff);
#endif
        i++;
    } while( i );

}




void dump_xdata_sfr( void )
{
    unsigned char i,k;

    for( i=0; i != sizeof ec_range / sizeof ec_range[0]; i++ )
    {
        putstring( "\r\n" );
        k = putstring( ec_range[i].name );
        while( k++ < 7 )
            putspace();
        puthex_u16( (unsigned int)ec_range[i].address );
        putchar(':');
        for( k=0; k<ec_range[i].len; k++ )
        {
            if( k && !(k % 8))
                putspace();
            putspace();
#if defined(SDCC)
            puthex( *(ec_range[i].address + k) );
#else
            puthex( 0xff );
#endif
        }
    }
}


__bit get_bit(volatile unsigned char __xdata *address, unsigned char bitnum)
{
    return (__bit)(*((bitnum/8) + address) & (unsigned char)(1<<(bitnum%8)));
}


void dump_gpio( void )
{
    unsigned char i,k;

    putstring( "\r\nName  Pin  EC_name           I/O/P/D alt_input/alt_output" );

    for( i=0; i != sizeof ec_gpio / sizeof ec_gpio[0]; i++ )
    {
        if( !ec_gpio[i].pin )
            continue;

        putstring( "\r\nGPI" );
        if(i<0x30)
        {
           putchar( 'O' );
           puthex( i<0x20?i:i+0xc0 );
        }
        else
        {
           putchar( 'A' );
           puthex( i+0xa0 );
        }
        putchar('(');
        putchar('0' + ec_gpio[i].pin/10);
        putchar('0' + ec_gpio[i].pin%10);
        putchar(')');
        putspace();

        k = putstring( ec_gpio[i].ec_name );
        while( ++k < 19 )
            putspace();

        if( get_bit( &GPIOIE00, i ) )
            putchar( '0' + get_bit( &GPIOIN00, i) );
        else
            putchar( '-' );
        putspace();

        if( get_bit( &GPIOOE00, i ) )
            putchar( '0' + get_bit( &GPIOD00, i ) );
        else
            putchar( '-' );
        putspace();

        /* pullup enable? */
        if( get_bit( &GPIOPU00, i ) )
            putchar( '1' );
        else
            putchar( '-');
        putspace();

        /* open drain enable? */
        if( get_bit( &GPIOOD00, i ) && (i<0x20) )
            putchar( '1' );
        else
            putchar( '-');
        putspace();

        if( i<0x20 )
        {
            k = get_bit( &GPIOIE00, i );
            putchar( k ? ' ' : '(' );
            putstring( ec_gpio[i].alt_in_name );
            putchar( k ? ' ' : ')' );

            k = get_bit( &GPIOFS00, i );
            putchar( k ? ' ' : '(' );
            putstring( ec_gpio[i].alt_out_name );
            putchar( k ? ' ' : ')' );
        }

        if( i>=0x30 && i<=0x32 )
            puthex( adc_cache[ i&0x07] );
    }
}


//! this function checks if IO pins are inadvertedly set to output
/*! setting of the output enable SFRs is checked against the values
    from ec-dump.fth
    If a pin is set to output that is not expected to be an output
    this function halts.

    Note, if an alternate function is set (GPIOFSxx) then a pin
    probably is an output without output enable (GPIOOExx) being set.
    This is not yet handled.

    (Also this function cannot react instantly on pins wrongly set
    to output (and of potentially illegal output combinations
    (if these exist)))
 */
void gpio_check_IO_direction(void)
{
    #define EC_DUMP_0xFC10_Q2C25_B4 { 0x87, 0xd7, 0xfe, 0x01, 0x0f, 0xbd }
    #define EC_DUMP_0xFC10_Q2C23_B1 { 0x87, 0xd7, 0xfe, 0x01, 0x0b, 0xbd }

    const unsigned char __code ec_dump_0xfc10[6] = EC_DUMP_0xFC10_Q2C23_B1;

    if( (GPIOOE00 & (unsigned char)~ec_dump_0xfc10[0]) |
        (GPIOOE08 & (unsigned char)~ec_dump_0xfc10[1]) |
        (GPIOOE10 & (unsigned char)~ec_dump_0xfc10[2]) |
        (GPIOOE18 & (unsigned char)~ec_dump_0xfc10[3]) |
        (GPIOEOE0 & (unsigned char)~ec_dump_0xfc10[4]) |
        (GPIOEOE8 & (unsigned char)~ec_dump_0xfc10[5]) )
    {
        /* disabling outputs again. */
        GPIOOE00 = 0x00;
        GPIOOE08 = 0x00;
        GPIOOE10 = 0x00;
        GPIOOE18 = 0x00;
        GPIOEOE0 = 0x00;
        GPIOEOE8 = 0x00;

        /* disabling almost all special functions */
        GPIOFS00 = 0x40; /* leave UART TX as output */
        GPIOFS08 = 0x00;
        GPIOFS10 = 0x00;
        GPIOFS18 = 0x00;

        /* and no pull up either */
        GPIOPU00 = 0x00;
        GPIOPU08 = 0x00;
        GPIOPU10 = 0x00;
        GPIOPU18 = 0x00;
        GPIOEPU0 = 0x00;
        GPIOEPU8 = 0x00;

        putstring("\r\nunexpected IO dir - halting.");

        /* lock */
        while(1)
            ;
    }
}
