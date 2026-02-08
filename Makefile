N64_INST = /n64_toolchain
PROG_NAME = audio_test
$(PROG_NAME)_OBJS = main.o audio_tests.o

all: $(PROG_NAME).z64

include $(N64_INST)/include/n64.mk

clean:
	rm -rf build $(PROG_NAME).z64

.PHONY: all clean