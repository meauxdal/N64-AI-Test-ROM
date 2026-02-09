N64_INST ?= /n64_toolchain
PROG_NAME = kb_test

kb_test_OBJS = main.o

all: $(PROG_NAME).z64

include $(N64_INST)/include/n64.mk
