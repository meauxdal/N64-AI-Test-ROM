N64_INST = /n64_toolchain
PROG_NAME = audio_test

# We are putting the objects in the root folder to eliminate pathing issues
audio_test_OBJS = main.o audio_tests.o

all: $(PROG_NAME).z64

include $(N64_INST)/include/n64.mk

clean:
	rm -f *.o *.elf *.z64

.PHONY: all clean