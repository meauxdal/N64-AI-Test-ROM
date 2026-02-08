N64_INST = /n64_toolchain
PROG_NAME = audio_test

#produce .z64
all: $(PROG_NAME).z64

#n64.mk needs these to populate linker
OBJS = main.o audio_tests.o

include $(N64_INST)/include/n64.mk

#maps Z64 target to ELF output
$(PROG_NAME).z64: $(PROG_NAME).elf

clean:
	rm -f *.o *.elf *.z64

.PHONY: all clean