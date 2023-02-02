#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>

#define HELP_MESSAGE "Params :\n\t-r (number)\t--rank (number)\t\tGet the rank of the suite.\n\t-js\t\t--json\t\t\tUse json format.\n"

unsigned char test(mpz_t value, mpz_t testedPrimeNumber) {
    mpf_t floatValue;
    mpf_init(floatValue);
    mpf_set_z(floatValue, value);

    mpf_t floatTestedPrimeNumber;
    mpf_init(floatTestedPrimeNumber);
    mpf_set_z(floatTestedPrimeNumber, testedPrimeNumber);

    mpf_t floatResult;
    mpf_init(floatResult);
    mpf_div(floatResult, floatValue, floatTestedPrimeNumber);

    mpz_t result;
    mpz_init(result);
    mpz_set_f(result, floatResult);

    if (mpf_cmp_z(floatResult, result) == 0) {
        return 1;
    }
    return 0;
}

mpz_t *primeNumbers(unsigned long long int rank) {
    mpz_t *primeList = (mpz_t*)malloc(sizeof(mpz_t) * rank);

    mpz_init(primeList[0]);
    mpz_set_ui(primeList[0], 2);
    mpz_init(primeList[1]);
    mpz_set_ui(primeList[1], 3);

    mpz_t actualNumber;
    mpz_init(actualNumber);
    mpz_set_ui(actualNumber, 5);

    register unsigned long long int iterator = 2;
    register unsigned long long int explorerIterator;
    unsigned char isPrime;

    while (iterator < rank) {
        isPrime = 1;

        for (explorerIterator = 1; explorerIterator < iterator / 2; explorerIterator = explorerIterator + 1) {
            if (test(actualNumber, primeList[explorerIterator]) == 1) {
                isPrime = 0;
                break;
            }
        }

        if (isPrime == 1) {
            mpz_init(primeList[iterator]);
            mpz_set(primeList[iterator], actualNumber);
            iterator = iterator + 1;
        }

        mpz_add_ui(actualNumber, actualNumber, 2);
    }

    return primeList;
}

int main(int argc, char* argv[]) {
    unsigned long long int rank = 0;

    unsigned char useJson = 0;

    for (register unsigned short int i = 1; i < argc; i++) {
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

    mpz_t *primeList = primeNumbers(rank);

    if (useJson) {
        printf("[");
    }

    for (register unsigned long long int i = 0; i < rank; i = i + 1) {
        mpz_out_str(stdout, 10, primeList[i]);
        if (!useJson) {
            printf("\n");
        } else if (i + 1 != rank) {
            printf(",");
        }
    }

    if (useJson) {
        printf("]\n");
    }
    return 0;
}
