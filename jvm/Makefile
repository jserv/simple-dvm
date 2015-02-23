PROJECT = jvm

# Toolchain configurations
CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc

SUFFIX ?=
EXECUTABLE = $(PROJECT)$(SUFFIX)

# Basic configurations
CFLAGS += -g -std=c99 -Wall -Wno-unused-result
LDFLAGS =

# Optimizations
CFLAGS += -Os

# project starts here
CFLAGS += -I.
OBJS = \
	bytecodes.o \
	field_pool_parser.o \
	java_lib.o \
	stack.o \
	class_parser.o \
	free_pool.o \
	main.o \
	constant_pool_parser.o \
	interface_pool_parser.o \
	method_pool_parser.o

$(EXECUTABLE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXECUTABLE)
	rm -f $(OBJS)
.PHONY: clean
