# Builds pkstorage.gba with devkitARM
TARGET  := pkstorage
CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy

ARCH    := -mthumb -mthumb-interwork -mcpu=arm7tdmi -mtune=arm7tdmi
CFLAGS  := $(ARCH) -O2 -Wall -Wextra -fno-strict-aliasing -ffunction-sections
LDFLAGS := $(ARCH) -specs=gba.specs -Wl,-Map,$(TARGET).map

all: $(TARGET).gba

$(TARGET).elf: main.c hw.h storage.h ui.h font.h species.h
	$(CC) $(CFLAGS) main.c $(LDFLAGS) -o $@

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	gbafix $@ -tPKSTORAGE -cPKST -m01 -r0

clean:
	rm -f $(TARGET).elf $(TARGET).gba $(TARGET).map

.PHONY: all clean
