N64_INST = /n64_toolchain
PROG_NAME = audio_test
OBJS = main.o audio_tests.o

all: $(PROG_NAME).z64

include $(N64_INST)/mips64-elf/lib/libdragon.mk

clean:
	rm -f *.o *.z64 *.elf *.dfs

.PHONY: all clean