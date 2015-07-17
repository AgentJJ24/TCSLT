TARGET_DEV = atmega328p
COMPILER = avr-

AS =            $(COMPILER)as
LD =            $(COMPILER)ld
CC =            $(COMPILER)gcc
CPP =           $(CC) -E
AR =            $(COMPILER)ar
NM =            $(COMPILER)nm
STRIP =         $(COMPILER)strip
OBJCOPY =       $(COMPILER)objcopy
OBJDUMP =       $(COMPILER)objdump

CFLAGS =        -mmcu=$(TARGET_DEV) -Wall -Os -o
ASFLAGS =       -mmcu=$(TARGET_DEV) -Wall -Os
LDFLAGS =       -T main.lds
OBJCPFLAGS = 	-j .text -j .data -O ihex

all: TCSLT.hex
	sudo avrdude -c usbasp -p m328p -U flash:w:TCSLT.hex:i

TCSLT.hex: TCSLT.elf
	$(OBJCOPY) $(OBJCPFLAGS) $< $@

TCSLT.elf: LTC.c LTC_READER_v1.0.c
	$(CC) $(CFLAGS) $@ $<

LTC_READER_v1.0.c:

LTC.c:

clean:
	rm *.hex && rm *.elf
