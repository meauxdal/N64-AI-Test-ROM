N64_INST ?= /n64_toolchain
PROG_NAME = kb_test

OBJS = main.o

kb_test_OBJS = $(OBJS)

all: $(PROG_NAME).z64

include $(N64_INST)/include/n64.mk
