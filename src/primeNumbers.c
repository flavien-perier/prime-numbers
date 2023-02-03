#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>

#define HELP_MESSAGE "Params :\n\t-r (number)\t--rank (number)\t\tGet the rank of the suite (> 2).\n\t-js\t\t--json\t\t\tUse json format.\n"

mpz_t *primeNumbers(unsigned long long int rank, unsigned char print, unsigned char useJson) {
    mpz_t *primeList = (mpz_t *) malloc(sizeof(mpz_t) * (rank == 0 ? 1000 : rank));;

    mpz_init_set_ui(primeList[0], 2);
    mpz_init_set_ui(primeList[1], 3);

    mpz_t actualNumber;
    mpz_init_set_ui(actualNumber, 5);

    mpz_t zero;
    mpz_init_set_ui(zero, 0);

    mpz_t modulo;
    mpz_init(modulo);

    if (print) {
        mpz_out_str(stdout, 10, primeList[0]);
        printf(useJson ? "," : "\n");
        mpz_out_str(stdout, 10, primeList[1]);
        printf(useJson ? "," : "\n");
    }

    register unsigned long long int iterator = 2;
    register unsigned long long int explorerIterator;
    unsigned char isPrime;

    while (iterator < rank || rank == 0) {
        isPrime = 1;

        for (explorerIterator = 1; explorerIterator < iterator / 2; explorerIterator++) {
            mpz_mod(modulo, actualNumber, primeList[explorerIterator]);

            if (mpz_cmp(modulo, zero) == 0) {
                isPrime = 0;
                break;
            }
        }

        if (isPrime) {
            if (rank == 0 && iterator % 1000 == 0) {
                primeList = realloc(primeList, sizeof(mpz_t) * (iterator + 1000));
            }

            mpz_init_set(primeList[iterator], actualNumber);

            if (print) {
                mpz_out_str(stdout, 10, actualNumber);
                if (iterator + 1 != rank || rank == 0) {
                    printf(useJson ? "," : "\n");
                }
            }

            iterator++;
        }

        mpz_add_ui(actualNumber, actualNumber, 2);
    }

    return primeList;
}

int main(int argc, char *argv[]) {
    unsigned long long int rank = 0;

    unsigned char useJson = 0;

    for (register unsigned short int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rank") == 0 || strcmp(argv[i], "-r") == 0) {
            if (i + 1 < argc) {
                rank = atoi(argv[i + 1]);

                if (rank < 3) {
                    printf(HELP_MESSAGE);
                    exit(EXIT_FAILURE);
                }
            } else {
                printf(HELP_MESSAGE);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-js") == 0 || strcmp(argv[i], "--json") == 0) {
            useJson = 1;
        }
    }

    if (useJson) {
        printf("[");
    }

    primeNumbers(rank, 1, useJson);

    if (useJson) {
        printf("]\n");
    }
    exit(EXIT_SUCCESS);
}
