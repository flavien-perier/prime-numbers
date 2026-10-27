#ifndef PRIME_NUMBERS_H
#define PRIME_NUMBERS_H

#include <stdlib.h>
#include <stdbool.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <semaphore.h>
#include <gmp.h>

#include "write.h"

mpz_t *primeNumbers(unsigned long long int rank, bool print, bool useJson, bool store, unsigned long long int *count);

void freePrimeNumbers(mpz_t *primeList, unsigned long long int count);

void stopPrimeNumbers(void);

#endif