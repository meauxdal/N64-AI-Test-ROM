BUILD_DIR = build
N64_INST = /n64_toolchain

include $(N64_INST)/include/libdragon.mk

CFLAGS += -O2 -Wall
LDFLAGS += 

OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/audio_tests.o

audio_test.z64: N64_ROM_TITLE="AI Test Menu"
audio_test.z64: $(BUILD_DIR)/audio_test.dfs

$(BUILD_DIR)/audio_test.dfs:
$(BUILD_DIR)/audio_test.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64

.PHONY: all clean

-include $(wildcard $(BUILD_DIR)/*.d)