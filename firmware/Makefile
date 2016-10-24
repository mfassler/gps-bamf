
######
######   What are we building?
######

# Target filename (without extension):
TARGET = gpsBamf

# Objects that must be built in order to link
OBJECTS += main.o
OBJECTS += system_timer.o
OBJECTS += usart.o
OBJECTS += bamf_twi.o
OBJECTS += accelerometer.o
OBJECTS += magnetometer.o
OBJECTS += presstemp.o
OBJECTS += tach.o

# Processor model, frequency:
MCU = atmega328p
F_CPU = 8000000


######
######   Binaries
######

CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude -p $(MCU) -c avrisp2 -P usb
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex


#####
#####   Flags
#####

# We must, must, must use unsigned chars:
CFLAGS = -D AVRGCC -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL
CFLAGS += -I. -funsigned-char
CFLAGS += -Wall -Os

LDFLAGS = -mmcu=$(MCU) -Wl,-Map=$(PROJECT).map,--cref


######
######   Targets and Rules
######

# Default target:
.PHONY: all
all: $(TARGET).hex

## This rule should be implicit to Make:
#.c.o:
#   $(CC) -c $(CFLAGS) -o $@ $<


%.elf: $(OBJECTS)
	@echo
	@echo "   LINKING to .elf:"
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.hex: %.elf
	@echo
	@echo "   CONVERTING from .elf to .hex:"
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature $< $@

%.lss: %.elf
	avr-objdump -h -S $< > $@


.PHONY: install
install: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)

.PHONY: size
size: ${TARGET}.hex
	@echo
	avr-size ${TARGET}.hex


.PHONY: clean
clean:
	rm -f $(OBJECTS) dep/* $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).map $(TARGET).lss
	@echo

# Don't automatically delete intermdiate files (*.o and *.elf)
.SECONDARY:


## Other dependencies
#-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

# Manually-defined dependencies:
#hid_task.o: usb_descriptors.h
#mouse_task.o: usb_descriptors.h
#storage_task.o: usb_descriptors.h
#usb_descriptors.o: usb_descriptors.h
#usb_specific_request.o: usb_descriptors.h


