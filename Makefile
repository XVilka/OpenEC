CC        = sdcc
DOC       = doxygen
SREC_CAT  = srec_cat
D52       = d52
CFLAGS    = --main-return
LFLAGS    = --xram-loc 0xf400 --xram-size 2048 --iram-size 128 --code-size 0xf400
OBJS      = $(SOURCES:.c=.o)
LSTS      = $(SOURCES:.c=.lst)
RELS      = $(SOURCES:.c=.rel)
ASMS      = $(SOURCES:.c=.asm)
SYMS      = $(SOURCES:.c=.sym)
RSTS      = $(SOURCES:.c=.rst)
ADBS      = $(SOURCES:.c=.adb)
PROJECT   = openec
SOURCES   = main.c fs_entry.c battery.c ds2756.c matrix_3x3.c port_0x6c.c \
            power.c sfr_dump.c sfr_rw.c states.c timer.c uart.c unused_irq.c \
            watchdog.c build.c

.SUFFIXES: .rel

$(PROJECT).ihx : $(RELS)
	@echo "Linking"
	$(CC) -o $@ $(LFLAGS) $(RELS)
	$(SREC_CAT) -disable_sequence_warnings \
	             $(PROJECT).ihx -intel \
	             -fill 0xff 0x0000 0xf300 \
	             -fill 0x00 0xf300 0xfffc \
	             -little_endian_checksum_negative 0xfffc 4 4 \
	             -o $(PROJECT).bin -binary
	if test "x`which $(D52) 2>/dev/null`" != "x" ; then $(D52) -p -n -d -b $(PROJECT).bin ; fi;
	mv $(PROJECT).bin $(PROJECT).do_not_use.bin

.c.rel :
	@echo "Compiling $<"
	touch build.c      
	$(CC) $(CFLAGS) -o $@ -c $<

docs :
	$(DOC) Doxyfile

clean :
	rm -f $(ASMS) $(LSTS) $(RELS) $(SYMS) $(OBJS) $(RSTS) $(ADBS)
	rm -f $(PROJECT).mem $(PROJECT).map $(PROJECT).lnk $(PROJECT).cdb \
	      $(PROJECT).ihx $(PROJECT).hex $(PROJECT).bin $(PROJECT).d52 \
	      $(PROJECT).do_not_use.bin
