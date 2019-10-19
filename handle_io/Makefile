#!/usr/bin/make -f

NAME = mcu_handle_input
SRC	 = mcu_handle_input.c

MCU_FREQ	= 16000000UL
MCU_TARGET	= atmega1284p
PROGRAMMER_MCU	= m1284p
PROGRAMMER	= avrisp2
UART_BAUD	= 9600UL

CC	= avr-gcc
OBJCOPY	= avr-objcopy

CFLAGS	+= -mmcu=$(MCU_TARGET) -Os -Wall -DF_CPU=$(MCU_FREQ) -DBAUD=$(UART_BAUD)
LDFLAGS	+= -mmcu=$(MCU_TARGET)
LIBS	+=
OBJ	:= $(patsubst %.c,%.o,$(SRC))

all: $(NAME).hex

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(NAME).elf: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

clean:
	rm -f $(OBJ)
	rm -f $(NAME).elf
	rm -f $(NAME).hex

flash: $(NAME).hex
	avrdude -c $(PROGRAMMER) -p $(PROGRAMMER_MCU) -U flash:w:$(NAME).hex

fuse:
	avrdude -c $(PROGRAMMER) -p $(PROGRAMMER_MCU) -U lfuse:w:0xE0:m
	avrdude -c $(PROGRAMMER) -p $(PROGRAMMER_MCU) -U hfuse:w:0xD9:m
