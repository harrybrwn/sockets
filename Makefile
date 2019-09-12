CC=gcc
CFLAGS=-Wall -Wextra -g

test: test.c sockets.c sockets.h
	$(CC) -I. $(CFLAGS) $< -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o test

.PHONY: clean