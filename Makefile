CC        = sdcc
DOC       = doxygen
SREC_CAT  = srec_cat
CFLAGS    = --main-return
LFLAGS    = --xram-size 2048 --iram-size 128
OBJS      = $(SOURCES:.c=.o)
LSTS      = $(SOURCES:.c=.lst)
RELS      = $(SOURCES:.c=.rel)
ASMS      = $(SOURCES:.c=.asm)
SYMS      = $(SOURCES:.c=.sym)
RSTS      = $(SOURCES:.c=.rst)
PROJECT   = open-ec
SOURCES   = main.c timer.c battery.c ds2756.c matrix_3x3.c states.c \
            build.c

.SUFFIXES: .rel

$(PROJECT).ihx : $(RELS)
	@echo "Linking"
	$(CC) -o $@ $(LFLAGS) $(RELS)
	$(SREC_CAT) -disable_sequence_warnings \
	             $(PROJECT).ihx -intel \
	             -fill 0xff 0x0000 0xff00 \
	             -fill 0x00 0xff00 0xfffe \
	             -little_endian_checksum_negative 0xfffe 2 2 \
	             -o $(PROJECT).bin -binary
	mv $(PROJECT).bin $(PROJECT).do_not_use.bin

.c.rel :
	@echo "Compiling $<"
	touch build.c      
	$(CC) $(CFLAGS) -o $@ -c $<

docs :
	$(DOC) Doxyfile

clean :
	rm -f $(ASMS) $(LSTS) $(RELS) $(SYMS) $(OBJS) $(RSTS)
	rm -f $(PROJECT).mem $(PROJECT).map $(PROJECT).lnk  \
	      $(PROJECT).ihx $(PROJECT).hex $(PROJECT).bin  \
	      $(PROJECT).do_not_use.bin
