CC=gcc
CFLAGS=
LDFLAGS=-lpthread -lm $(shell pkg-config --cflags --libs gmp)
EXEC=cleanBefore primeNumbers cleanAfter test
SRC=$(shell find ./src -type f -name *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

primeNumbers: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean test

cleanBefore:
	rm -f ./primeNumbers
	rm -rf ./src/*.o

cleanAfter:
	rm -rf ./src/*.o

test:
	./test/test.sh