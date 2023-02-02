CC=gcc
CFLAGS=
LDFLAGS=-lpthread -lm -lgmp
EXEC=primeNumbers clean test
SRC=$(shell find ./src -type f -name *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

primeNumbers: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean test

clean:
	rm -rf ./src/*.o

test:
	./test/test.sh