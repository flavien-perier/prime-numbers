CC=gcc
CFLAGS=
LDFLAGS=-lpthread -lm
EXEC=primeNumbers clean test
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

primeNumbers: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean test

clean:
	rm -rf *.o

test:
	./test.sh