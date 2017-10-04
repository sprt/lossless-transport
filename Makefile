.PHONY: all clean tests

all: sender

tests:
	gcc -std=c99 -Werror -Wall -Wextra -Wpedantic -pedantic-errors \
	-Wshadow -Wduplicated-cond -Wduplicated-branches -Wlogical-op \
	-Wnull-dereference -Wformat=2 \
	src/packet_implem.c tests/test_packet.c -lz -lcunit -o test
	./test

sender:
	gcc -std=c99 -Werror -Wall -Wextra -Wpedantic -pedantic-errors \
	-Wshadow -Wduplicated-cond -Wduplicated-branches -Wlogical-op \
	-Wnull-dereference -Wformat=2 \
	src/packet_implem.c src/sender.c -lz -o sender

clean:
	rm -f sender test
