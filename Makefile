N64_INST ?= /n64_toolchain
PROG_NAME = kb_test

kb_test_OBJS = main.o

include $(N64_INST)/include/n64.mk
