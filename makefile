GCC_DEVICE  = attiny84
DUDE_DEVICE = m328p
CLOCK       = 8000000            # 8Mhz (just used for code that needs to know about the CPU)
PROGRAMMER  = -c usbtiny -P usb  # For using Sparkfun Pocket AVR Programmer
FUSES       = -U lfuse:w:0xe2:m -U hfuse:w:0xde:m -U efuse:w:0xff:m # CKDIV8 is off
SRCS        = $(wildcard src/*.c)
DEPS        = $(wildcard src/*.h)
OBJS        = $(SRCS:src/%.c=build/%.o)

AVRDUDE = avrdude $(PROGRAMMER) -p $(DUDE_DEVICE) -e
COMPILE = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(GCC_DEVICE)

all: main.hex

setup:
	mkdir -p -- build

clean:
	rm build/*

build/%.o : src/%.c $(DEPS)
	$(COMPILE) -c $< -o $@

build/%.o : src/%.S
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

main.elf: $(OBJS)
	$(COMPILE) -o build/main.elf $(OBJS)

main.hex: setup main.elf
	avr-objcopy -j .text -j .data -O ihex build/main.elf build/main.hex
	avr-size --format=avr --mcu=$(GCC_DEVICE) build/main.elf

disasm: main.elf
	avr-objdump -d build/main.elf

upload: main.hex
	$(AVRDUDE) -U flash:w:build/main.hex:i

fuse:
	$(AVRDUDE) $(FUSES)

install: upload
