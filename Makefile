N64_INST = /n64_toolchain
PROG_NAME = audio_test

all: $(PROG_NAME).z64

$(PROG_NAME)_OBJS = main.o audio_tests.o

include $(N64_INST)/include/n64.mk

clean:
	rm -f *.o *.elf *.z64

.PHONY: all clean