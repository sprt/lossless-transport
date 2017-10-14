CC = gcc
CFLAGS += -g
CFLAGS += -std=gnu99
CFLAGS += -O1
CFLAGS += -Werror
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpedantic
CFLAGS += -pedantic-errors
CFLAGS += -Wshadow
CFLAGS += -Wformat=2
LDFLAGS = -lz

.PHONY: default receiver sender tests

default: sender receiver

tests: LDFLAGS += -lcunit
tests: SRCS += src/packet_implem.c
tests: SRCS += src/minqueue.c
tests: SRCS += tests/main.c
tests:
	@rm -f test
	$(CC) -o test $(SRCS) $(CFLAGS) $(LDFLAGS)
	valgrind --leak-check=full --show-leak-kinds=all ./test

sender:
	@rm -f sender
	$(CC) -o sender src/packet_implem.c src/util.c src/sender.c $(CFLAGS) $(LDFLAGS)

receiver:
	@rm -f receiver
	$(CC) -o receiver src/packet_implem.c src/util.c src/receiver.c $(CFLAGS) $(LDFLAGS)
