N64_INST = /n64_toolchain
PROG_NAME = audio_test
BUILD_DIR = build

# Wiring Signal: These now correctly expand to build/main.o and build/audio_tests.o
audio_test_OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/audio_tests.o

all: $(PROG_NAME).z64

include $(N64_INST)/include/n64.mk

# Existence Signal: Ensures the build directory exists before any objects are compiled
$(audio_test_OBJS): | $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(PROG_NAME).z64

.PHONY: all clean