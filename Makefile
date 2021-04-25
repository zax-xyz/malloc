CC=gcc
CFLAGS=-fsanitize=address -Wall -Werror -std=gnu11 -g -lm

tests: tests.c virtual_alloc.c
	$(CC) $(CFLAGS) -DDEBUG $^ -o $@

debug: tests.c virtual_alloc.c
	$(CC) $(CFLAGS) -DDEBUG $^ -o $@