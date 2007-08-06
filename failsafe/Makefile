CC        = sdcc
DOC       = doxygen
SREC_CAT  = srec_cat
D52       = d52
CFLAGS    = --main-return --no-xinit-opt
LFLAGS    = --xram-loc 0xf400 --xram-size 2048 --iram-size 128 --code-size 0x4000
OBJS      = $(SOURCES:.c=.o)
LSTS      = $(SOURCES:.c=.lst)
RELS      = $(SOURCES:.c=.rel)
ASMS      = $(SOURCES:.c=.asm)
SYMS      = $(SOURCES:.c=.sym)
RSTS      = $(SOURCES:.c=.rst)
ADBS      = $(SOURCES:.c=.adb)
PROJECT   = notfailsafe
SOURCES   = failsafe.c

.SUFFIXES: .rel

$(PROJECT).bin : $(RELS)
	@echo "Linking"
	$(CC) -o $(PROJECT).ihx $(LFLAGS) $(RELS)
	$(SREC_CAT) -disable_sequence_warnings \
	             $(PROJECT).ihx -intel \
	             -fill 0xff 0x0000 0x3ffc \
	             -little_endian_checksum_negative 0x3ffc 4 4 \
	             -o $(PROJECT).bin -binary
	if test "x`which $(D52) 2>/dev/null`" != "x" ; then $(D52) -P -N -D -b $(PROJECT).bin ; fi;

.c.rel :
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -o $@ -c $<


clean :
	rm -f $(ASMS) $(LSTS) $(RELS) $(SYMS) $(OBJS) $(RSTS) $(ADBS)
	rm -f $(PROJECT).mem $(PROJECT).map $(PROJECT).lnk $(PROJECT).cdb \
	      $(PROJECT).ihx $(PROJECT).hex $(PROJECT).bin $(PROJECT).d52
