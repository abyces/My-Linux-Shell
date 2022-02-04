CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: nyush

nyush: nyush.o parser.o buildin.o process.o

nyush.o: nyush.c parser.h buildin.h process.h

parser.o: parser.c parser.h

buildin.o: buildin.c buildin.h process.h parser.h

process.o: process.c process.h

.PHONY: clean
clean:
	rm -f *.o nyush
