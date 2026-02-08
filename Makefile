N64_INST = /n64_toolchain
PROG_NAME = audio_test

audio_test_OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/audio_tests.o

all: $(PROG_NAME).z64

include $(N64_INST)/include/n64.mk

BUILD_DIR = build

clean:
	rm -rf $(BUILD_DIR) $(PROG_NAME).z64

.PHONY: all clean