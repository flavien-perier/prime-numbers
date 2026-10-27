CC=gcc
CFLAGS=-O2 -Wall -Wextra -std=c99
LDFLAGS=-lpthread -lm -lgmp
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

all: test

primeNumbers: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

test: primeNumbers
	./test/test.sh

analyze:
	@for src in $(SRC); do \
		echo "analyze $$src"; \
		$(CC) -fanalyzer $(CFLAGS) -c $$src -o /dev/null || exit 1; \
	done

clean:
	rm -f primeNumbers $(OBJ)

.PHONY: all test analyze clean