N64_INST = /n64_toolchain
PROG = audio_test

OBJS = main.o audio_tests.o

all: $(PROG).z64

include $(N64_INST)/include/n64.mk

clean:
	rm -f *.o *.elf *.z64

.PHONY: all clean