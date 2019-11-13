#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define HELP_MESSAGE "Params :\n\t-r (number)\t-rank (number)\t\tIndicate the rank of the suite.\n\t-js\t\t--json\t\t\tUse json format.\n"

unsigned char isInteger(double value) {
	if (round(value) == value) {
		return 1;
	}
	return 0;
}

long unsigned int *primeNumbers(long unsigned int rank) {
	long unsigned int *primeList = (long int*)malloc(sizeof(long int) * rank);

	*(primeList) = 2;
	*(primeList + 1) = 3;

	long unsigned int iterator = 2;
	unsigned char isPrime;

	for (register long unsigned int i = 5; iterator < rank; i = i + 2) {
		isPrime = 1;
		for (register long unsigned int j = 2; j < iterator && *(primeList + j) < i / 2; j++) {
			if (isInteger(i * 1.0 / *(primeList + j))) {
				isPrime = 0;
				break;
			}
		}

		if (isPrime) {
			*(primeList + iterator) = i;
			iterator++;
		}
	}

	return primeList;
}

int main(int argc, char* argv[]) {
	long unsigned int rank = 0;
	unsigned char useJson = 0;

	for (register unsigned int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--rank") == 0 || strcmp(argv[i], "-r") == 0) {
			if (i + 1 < argc) {
				rank = atoi(argv[i + 1]);
			} else {
				printf(HELP_MESSAGE);
				return -1;
			}
		} else if (strcmp(argv[i], "-js") == 0 || strcmp(argv[i], "--json") == 0) {
			useJson = 1;
		}
	}

	if (rank == 0) {
		printf(HELP_MESSAGE);
		return -1;
	}

	long unsigned int *primeList = primeNumbers(rank);

	if (useJson) {
		printf("[");
	}

	for (register unsigned int i = 0; i<rank; i++) {
		if (useJson) {
			printf("%d", *(primeList + i));
			if (i + 1 != rank) {
				printf(",");
			}
		}
		else {
			printf("%d\n", *(primeList + i));
		}
	}

	if (useJson) {
		printf("]\n");
	}
	return 0;
}
