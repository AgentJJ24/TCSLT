PRG = TCSLT
OBJ = LTC.o LTC_READER_v1.0.o

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

LIBS=
CFLAGS =        -mmcu=$(TARGET_DEV) -Wall -Os
ASFLAGS =       -mmcu=$(TARGET_DEV) -Wall -Os
LDFLAGS =       -T main.lds
OBJCPFLAGS = 	-j .text -j .data -O ihex

all: $(PRG).hex $(PRG).elf
	sudo avrdude -c usbasp -p m328p -U flash:w:$(PRG).hex:i

$(PRG).hex: $(PRG).elf
	$(OBJCOPY) $(OBJCPFLAGS) $< $@

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm *.hex && rm *.elf
