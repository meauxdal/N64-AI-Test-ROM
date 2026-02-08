N64_INST = /n64_toolchain
PROG_NAME = audio_test

# Try the most generic object variable name
OBJS = main.o audio_tests.o
audio_test_OBJS = $(OBJS)

all: $(PROG_NAME).z64

include $(N64_INST)/include/n64.mk

# Explicit override: If make is being lazy, this forces it to realize 
# that audio_test.elf depends on your actual code.
$(PROG_NAME).elf: $(OBJS)

clean:
	rm -f *.o *.elf *.z64

.PHONY: all clean