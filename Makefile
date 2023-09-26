CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: nyush

nyush: nyush.o list.o

nyush.o: nyush.c nyush.h

list.o: list.c list.h

.PHONY: clean
clean:
	rm -f *.o nyush
