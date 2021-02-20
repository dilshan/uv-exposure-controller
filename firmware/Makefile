MCU = attiny2313
CLOCK = 8000000

OBJS = main.o

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(MCU)
AVRDUDE = avrdude -c usbasp -p $(MCU)

all:	exposurecntrl.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

exposurecntrl.elf: $(OBJS)
	$(COMPILE) -o exposurecntrl.elf $(OBJS)

exposurecntrl.hex: exposurecntrl.elf
	rm -f exposurecntrl.hex
	avr-objcopy -j .text -j .data -O ihex exposurecntrl.elf exposurecntrl.hex

disasm:	exposurecntrl.elf
	avr-objdump -d exposurecntrl.elf

clean:
	rm -f exposurecntrl.hex exposurecntrl.elf $(OBJS)

flash:	all
	$(AVRDUDE) -U flash:w:exposurecntrl.hex:i

fuse:
	$(AVRDUDE) -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m