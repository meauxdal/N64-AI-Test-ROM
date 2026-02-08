N64_INST = /n64_toolchain
PROG_NAME = audio_test

# 1. THE GOAL (The missing piece from bw18)
all: $(PROG_NAME).z64

# 2. THE SIGNALS (Redundant to ensure one hits)
# We provide the generic and the specific to populate the linker.
OBJS = main.o audio_tests.o
audio_test_OBJS = main.o audio_tests.o

# 3. THE LIBRARY
include $(N64_INST)/include/n64.mk

# 4. THE FORCED LINK (The Hammer)
# If the library still tries to link a blank ELF, this forces 
# your objects into the command line.
$(PROG_NAME).elf: $(OBJS)

clean:
	rm -f *.o *.elf *.z64

.PHONY: all clean