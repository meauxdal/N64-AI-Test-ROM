BUILD_DIR = build
include $(N64_INST)/include/n64.mk

N64_CFLAGS += -O2 -Wall
N64_LDFLAGS += -ldragon

OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/audio_tests.o

all: audio_test.z64

$(BUILD_DIR)/audio_test.dfs:
$(BUILD_DIR)/audio_test.elf: $(OBJS)

audio_test.z64: N64_ROM_TITLE="AI Test Menu"

clean:
	rm -rf $(BUILD_DIR) *.z64

.PHONY: all clean
