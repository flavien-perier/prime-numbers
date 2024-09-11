CC=gcc
CFLAGS=-Wall -std=c99
LDFLAGS=-lpthread -lm $(shell pkg-config --cflags --libs gmp)
SRC=$(shell find ./src -type f -name *.c)
OBJ=$(SRC:.c=.o)

all: cleanBefore primeNumbers cleanAfter test
cleanAfter: primeNumbers
test: cleanAfter

primeNumbers: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: cleanBefore cleanAfter test

cleanBefore:
	rm -f ./primeNumbers
	find . -name "*.o" -delete

cleanAfter:
	find . -name "*.o" -delete

test:
	./test/test.sh