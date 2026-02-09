N64_INST ?= /n64_toolchain
PROG_NAME = kb_test

SRCS = main.c
OBJS = $(SRCS:.c=.o)

kb_test_OBJS = $(OBJS)

include $(N64_INST)/include/n64.mk
