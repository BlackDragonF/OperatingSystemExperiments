CC = gcc
CFLAGS = -Wall -Werror
LFLAGS = -lm

fake-ls: fake-ls.c
		$(CC) $(CFLAGS) $< $(LFLAGS) -o fake-ls

.PHONY: clean

clean:
		rm -f fake-ls
