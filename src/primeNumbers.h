#ifndef PRIME_NUMBERS_H
#define PRIME_NUMBERS_H

#include <stdlib.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <gmp.h>

#include "write.h"

mpz_t *primeNumbers(unsigned long long int rank, unsigned char print, unsigned char useJson);

void stopPrimeNumbers();

#endif