#include "stdafx.h"

#include <iostream>
#include <string.h>
#include <math.h>
#include <cstdlib>

using namespace std;

bool isInteger(double value) {
	if (round(value) == value) {
		return true;
	}
	return false;
}

long unsigned int *primeNumberUpToTheRank(long unsigned int rank) {
	long unsigned int *primeList = new long unsigned int[rank];

	*(primeList) = 2;
	*(primeList + 1) = 3;

	long unsigned int iterator(2);
	bool isPrime;

	for (double i = 5; iterator<rank; i = i + 2) {
		isPrime = true;
		for (long unsigned int j = 2; j<iterator && *(primeList + j) < i / 2; j++) {
			if (isInteger(i / *(primeList + j))) {
				isPrime = false;
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
	int rank = NULL;
	bool useJson = false;

	const string help = "Params :\n\t-r (number)\t-rank (number)\t\tIndicate the rank of the suite.\n\t-js\t\t--json\t\t\tUse json format.";

	for (int i = 1; i<argc; i++) {
		if (strcmp(argv[i], "--rank") == 0 || strcmp(argv[i], "-r") == 0) {
			if (i + 1 < argc) {
				rank = atoi(argv[i + 1]);
			}
			else {
				cout << help.c_str() << endl;
				return -1;
			}
		}
		else if (strcmp(argv[i], "-js") == 0 || strcmp(argv[i], "--json") == 0) {
			useJson = true;
		}
	}

	if (rank == NULL || rank == 0) {
		cout << help.c_str() << endl;
		return -1;
	}

	long unsigned int *primeList = primeNumberUpToTheRank(rank);

	if (useJson)
		cout << "[";

	for (int i = 0; i<rank; i++) {
		if (useJson) {
			cout << "\"" << *(primeList + i) << "\"";
			if (i + 1 != rank)
				cout << ",";
		}
		else {
			cout << *(primeList + i) << endl;
		}
	}

	if (useJson)
		cout << "]" << endl;
	return 0;
}
