CC=gcc
CFLAGS=
LDFLAGS=-lpthread -lm -lgmp
EXEC=cleanBefore primeNumbers cleanAfter test
SRC=$(shell find ./src -type f -name *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

primeNumbers: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean test

cleanBefore:
	rm -f ./primeNumbers
	rm -rf ./src/*.o

cleanAfter:
	rm -rf ./src/*.o

test:
	./test/test.sh