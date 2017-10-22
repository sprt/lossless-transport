CC = gcc
CFLAGS += -g
CFLAGS += -std=gnu99
CFLAGS += -O0 # useful when using a debugger
CFLAGS += -Werror
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpedantic
CFLAGS += -pedantic-errors
CFLAGS += -Wshadow
CFLAGS += -Wformat=2
LDFLAGS = -lz

.PHONY: default receiver sender tests

default: SRCS += src/packet_implem.c
default: SRCS += src/util.c
default: SRCS += src/window.c
default: sender receiver

tests: IFLAGS += -Ilib/CUnit-2.1-3/include
tests: LDFLAGS += lib/CUnit-2.1-3/lib/libcunit.a
tests: SRCS += src/packet_implem.c
tests: SRCS += src/window.c
tests: SRCS += tests/main.c
tests:
	@rm -f test
	$(CC) -o test $(IFLAGS) $(SRCS) $(CFLAGS) $(LDFLAGS)
	valgrind --leak-check=full --show-leak-kinds=all ./test

sender: SRCS += src/sender.c
sender:
	@rm -f sender
	$(CC) -o sender $(SRCS) $(CFLAGS) $(LDFLAGS)

receiver: SRCS += src/receiver.c
receiver:
	@rm -f receiver
	$(CC) -o receiver $(SRCS) $(CFLAGS) $(LDFLAGS)
