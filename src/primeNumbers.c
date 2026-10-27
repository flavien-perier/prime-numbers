#define _GNU_SOURCE
#include "primeNumbers.h"

#include <errno.h>
#include <signal.h>
#include <string.h>

static volatile sig_atomic_t PROCESS_PRIME_NUMBERS_FINISH = 0;

typedef struct {
    pthread_t thread;
    sem_t workSem;
    sem_t resultSem;

    mpz_t *primeList;
    volatile bool work;

    unsigned long long int iterator;
    mpz_t testedNumber;
    bool isPrime;
} Worker;

static void writePrime(const mpz_t prime) {
    char *str = mpz_get_str(NULL, 10, prime);
    writeOutput(str);
    free(str);
}

static void writeSeparator(const bool useJson) {
    writeOutput(useJson ? "," : "\n");
}

static void *primeTesterWorker(void *args) {
    Worker *worker = (Worker *) args;

    sigset_t blockedSignals;
    sigemptyset(&blockedSignals);
    sigaddset(&blockedSignals, SIGINT);
    sigaddset(&blockedSignals, SIGQUIT);
    pthread_sigmask(SIG_BLOCK, &blockedSignals, NULL);

    mpz_t modulo;
    mpz_init(modulo);

    mpz_t sqrtTestedNumber;
    mpz_init(sqrtTestedNumber);

    while (true) {
        sem_wait(&worker->workSem);
        if (!worker->work) {
            break;
        }

        worker->isPrime = true;
        mpz_sqrt(sqrtTestedNumber, worker->testedNumber);

        for (unsigned long long int explorerIterator = 1; explorerIterator <= worker->iterator / 2; explorerIterator++) {
            if (mpz_cmp(worker->primeList[explorerIterator], sqrtTestedNumber) > 0) {
                break;
            }

            mpz_mod(modulo, worker->testedNumber, worker->primeList[explorerIterator]);
            if (mpz_cmp_ui(modulo, 0) == 0) {
                worker->isPrime = false;
                break;
            }
        }

        sem_post(&worker->resultSem);
    }

    mpz_clear(modulo);
    mpz_clear(sqrtTestedNumber);
    return NULL;
}

static Worker *createWorker(mpz_t *primeList) {
    Worker *worker = (Worker *) malloc(sizeof(Worker));
    if (worker == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    worker->primeList = primeList;
    worker->work = true;
    worker->iterator = 0;
    worker->isPrime = false;

    if (sem_init(&worker->workSem, 0, 0) == -1 || sem_init(&worker->resultSem, 0, 0) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    mpz_init(worker->testedNumber);

    const int error = pthread_create(&worker->thread, NULL, primeTesterWorker, worker);
    if (error != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }

    return worker;
}

mpz_t *primeNumbers(const unsigned long long int rank, const bool print, const bool useJson, unsigned long long int *count) {
    const int configuredProcessors = get_nprocs_conf();
    const unsigned int nbrMaxThread = (configuredProcessors > 0) ? (unsigned int) configuredProcessors * 2 : 2;
    unsigned int nbrThreads = 1;
    unsigned long long int iterator = (rank == 1) ? 1 : 2;
    unsigned long long int capacity = (rank == 0) ? 1000 : rank;

    mpz_t *primeList = (mpz_t *) malloc(sizeof(mpz_t) * capacity);
    if (primeList == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    Worker **workers = (Worker **) malloc(sizeof(Worker *) * nbrMaxThread);
    if (workers == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (unsigned int threadIterator = 0; threadIterator < nbrMaxThread; threadIterator++) {
        workers[threadIterator] = createWorker(primeList);
    }

    mpz_init_set_ui(primeList[0], 2);
    if (rank != 1) {
        mpz_init_set_ui(primeList[1], 3);
    }

    mpz_t actualNumber;
    mpz_init_set_ui(actualNumber, 5);

    if (print) {
        writePrime(primeList[0]);
        if (rank != 1) {
            writeSeparator(useJson);
            writePrime(primeList[1]);
        }
    }

    while ((rank == 0 || iterator < rank) && !PROCESS_PRIME_NUMBERS_FINISH) {
        if (rank == 0 && capacity < iterator + nbrThreads) {
            capacity = iterator + nbrThreads + 1000;
            mpz_t *newPrimeList = (mpz_t *) realloc(primeList, sizeof(mpz_t) * capacity);
            if (newPrimeList == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            primeList = newPrimeList;
            for (unsigned int threadIterator = 0; threadIterator < nbrMaxThread; threadIterator++) {
                workers[threadIterator]->primeList = primeList;
            }
        }

        for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
            Worker *worker = workers[threadIterator];
            mpz_add_ui(worker->testedNumber, actualNumber, threadIterator * 2);
            worker->iterator = iterator;
            sem_post(&worker->workSem);
        }

        for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
            Worker *worker = workers[threadIterator];
            int waitResult;
            do {
                waitResult = sem_wait(&worker->resultSem);
            } while (waitResult == -1 && errno == EINTR && !PROCESS_PRIME_NUMBERS_FINISH);

            if (waitResult == -1) {
                goto cleanup;
            }

            if (worker->isPrime) {
                mpz_init_set(primeList[iterator], worker->testedNumber);
                if (print) {
                    writeSeparator(useJson);
                    writePrime(primeList[iterator]);
                }
                iterator++;
                if (rank != 0 && iterator >= rank) {
                    break;
                }
            }
        }

        mpz_add_ui(actualNumber, actualNumber, 2 * nbrThreads);

        if (nbrThreads < iterator && nbrThreads < nbrMaxThread) {
            nbrThreads++;
        }
    }

cleanup:
    for (unsigned int threadIterator = 0; threadIterator < nbrMaxThread; threadIterator++) {
        workers[threadIterator]->work = false;
        sem_post(&workers[threadIterator]->workSem);
    }
    for (unsigned int threadIterator = 0; threadIterator < nbrMaxThread; threadIterator++) {
        Worker *worker = workers[threadIterator];
        pthread_join(worker->thread, NULL);
        sem_destroy(&worker->workSem);
        sem_destroy(&worker->resultSem);
        mpz_clear(worker->testedNumber);
        free(worker);
    }
    free(workers);
    mpz_clear(actualNumber);

    *count = iterator;
    return primeList;
}

void freePrimeNumbers(mpz_t *primeList, const unsigned long long int count) {
    if (primeList == NULL) {
        return;
    }
    for (unsigned long long int index = 0; index < count; index++) {
        mpz_clear(primeList[index]);
    }
    free(primeList);
}

void stopPrimeNumbers(void) {
    PROCESS_PRIME_NUMBERS_FINISH = 1;
}