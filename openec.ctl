;
; Header file for d52
;
; http://home.pacbell.net/theposts/
;
; derived from:
; KB3700 Keyboard Controller Datasheet Revision 0.1
;

Z 0-F3FF
T FF00-FFFC
B FFFC-FFFF

# f400
# f400 Embedded SRAM 0xf400..0xfbff (xdata memory)
# f400

# fc00
# fc00 Special Function Registers 0xfc00..0xffff (xdata memory)
# fc00

# ff00
# ff00 Build Strings (code memory)
# ff00

# fffc
# fffc Checksum (code memory)
# fffc

;
; directly addressable Special Function Registers
;
f 80 P0IE
f 86 PCON2
f 87 PCON
f 90 P1IE
f 9a SCON2
f 9b SCON3
f b0 P3IE
f d8 P0IF
f e8 P1IF
f f8 P3IF

;
; xdata Special Function Registers
;
s fc00 GPIOFS00
s fc01 GPIOFS08
s fc02 GPIOFS10
s fc03 GPIOFS18

s fc10 GPIOOE00
s fc11 GPIOOE08
s fc12 GPIOOE10
s fc13 GPIOOE18
s fc14 GPIOE0E0
s fc15 GPIOE0E8

s fc20 GPIOD00
s fc21 GPIOD08
s fc22 GPIOD10
s fc23 GPIOD18
s fc24 GPIOED0
s fc25 GPIOED8

s fc30 GPIOIN00
s fc31 GPIOIN08
s fc32 GPIOIN10
s fc33 GPIOIN18
s fc34 GPIOEIN0
s fc35 GPIOEIN8
s fc36 GPIADIN

s fc40 GPIOPU00
s fc41 GPIOPU08
s fc42 GPIOPU10
s fc43 GPIOPU18
s fc44 GPIOEPU0
s fc45 GPIOEPU8

s fc50 GPIOOD00
s fc51 GPIOOD08
s fc52 GPIOOD10
s fc53 GPIOOD18

s fc60 GPIOIE00
s fc61 GPIOIE08
s fc62 GPIOIE10
s fc63 GPIOIE18
; name clash within table 4.2.1:
s fc64 GPIOEIN0_ERR_FC64
s fc65 GPIOEIN8_ERR_FC65
s fc66 GPIAD0

s fc70 GPIOMISC

s fc80 KBCCB
s fc81 KBCCFG
s fc82 KBCCIF
s fc83 KBCHWEN
s fc84 KBCCMD
s fc85 KBCDAT
s fc86 KBCSTS

s fe00 PWMCFG
s fe01 PWMHIGH0
s fe02 PWMCYCL0
s fe03 PWMHIGH1
s fe04 PWMCYCL1
s fe05 PWMCFG2
s fe06 PWMCFG3
s fe07 PWMCFG4
s fe08 PWMHIGH2
s fe09 PWMHIGH3
s fe0a PWMHIGH4
s fe0b PWMCYC2
s fe0c PWMCYC3
s fe0d PWMCYC4


s fe50 GPTCFG
s fe51 GPTPF
s fe53 GPT0
s fe55 GPT1
s fe56 GPT2H
s fe57 GPT2L
s fe58 GPT3H
s fe59 GPT3L


s fe80 WDTCFG
s fe81 WDTPF
s fe82 WDTCNT
s fe83 WDT19_12
s fe84 WDT11_04
s fe85 WDT03_00


s fe90 LPCSTAT
s fe91 LPCSIRQ
s fe92 LPCBAH
s fe93 LPCBAL
s fe94 LPCFWH
s fe95 LPCCFG
s fe96 LPCXBAH
s fe97 LPCXBAL
s fe98 LPCEBAH
s fe99 LPCEBAL
s fe9a LPC_2EF
s fe9b LPC_RSV_fe9b
s fe9c LPC_2F_DATA
s fe9d LPC68CFG
s fe9e LPC68CSR
s fe9f LPC68DAT


s fea0 XBISEG0
s fea1 XBISEG1
s fea4 XBIXIOEN
s fea5 XBICFG
s fea6 XBICS
s fea7 XBIWE
s fea8 SPIA0
s fea9 SPIA1
s feaa SPIA2
s feab SPIDAT
s feac SPICMD
s fead SPICFG
s feae SPIDATR
s feaf SPICFG2


s fee0 PS2CFG
s fee1 PS2PF
s fee2 PS2CTRL
s fee3 PS2DATA
s fee4 PS2CFG2
s fee5 PS2PINS
s fee6 PS2PINO


s ff00 ECHV
s ff01 ECFV
s ff02 ECHA
s ff03 ESCICFG
s ff04 ECCFG
s ff05 SCIE0
s ff06 SCIE1
s ff07 SCIE2
s ff08 SCIF0
s ff0b SCID
s ff0c PMUCFG
s ff0d CLKCFG
s ff0e EXTIO
s ff0f PLLCFG
;
s ff11 RSV_0xff11
s ff12 CLKCFG2
s ff13 PLLCFG2
s ff14 PXCFG
s ff15 ADDAEN
s ff16 PLLFRH
s ff17 PLLFRL
s ff18 ADCTRL
s ff19 ADCDAT
s ff1a ECIF
s ff1b ECDAT
s ff1c ECCMD
s ff1d ECSTS
s ff1e PLLVAL_A
s ff1f PLLVAL_B


s ff30 GPWUEN00
s ff31 GPWUEN08
s ff32 GPWUEN10
s ff33 GPWUEN18

s ff40 GPWUPF00
s ff41 GPWUPF08
s ff42 GPWUPF10
s ff43 GPWUPF18

s ff50 GPWUPS00
s ff51 GPWUPS08
s ff52 GPWUPS10
s ff53 GPWUPS18

s ff60 GPWUEL00
s ff61 GPWUEL08
s ff62 GPWUEL10
s ff63 GPWUEL18


;
; Interrupt vectors
;
l 00 reset
l 03 irq_ext0
l 0b irq_tf0
l 13 irq_ext1
l 1b irq_tf1
l 23 irq_ser
l 2b irq_na_2b
l 33 irq_na_33
l 3b irq_na_3b
l 43 irq_wdt
l 4b irq_na_4b
l 53 irq_ps2
l 5b irq_kbc_host
l 63 irq_rsv63
l 6b irq_lpc
l 73 irq_ec_host
l 7b irq_na_7b
l 83 irq_rsv_83
l 8b irq_rsv_8b
l 93 irq_rsv_93
l 9b irq_na_9b
l a3 irq_gpt0
l ab irq_gpt1
l b3 irq_gpt2
l bb irq_gpt3
l c3 irq_extwio_port80
l cb irq_gpio00_0f
l d3 irq_gpio10_1b
l db irq_rsvdb
l e3 irq_rsv_e3
l eb irq_rsv_eb
l f3 irq_rsv_f3
l fb irq_adc
