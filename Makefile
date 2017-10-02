all: sender

sender:
	gcc -pedantic -std=c99 -Werror -Wall -Wextra -Wpedantic \
	-pedantic-errors -Wshadow -Wduplicated-cond -Wduplicated-branches \
	-Wlogical-op -Wrestrict -Wnull-dereference -Wformat=2 \
	src/sender.c -o sender