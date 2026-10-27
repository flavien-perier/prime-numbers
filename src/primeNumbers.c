#define _GNU_SOURCE
#include "primeNumbers.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>

#define WORKER_CANDIDATES 64

static volatile sig_atomic_t PROCESS_PRIME_NUMBERS_FINISH = 0;

typedef struct {
    pthread_t thread;
    sem_t workSem;
    sem_t resultSem;

    volatile bool work;
    mpz_t baseNumber;
    uint64_t primeMask;
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

    mpz_t candidate;
    mpz_init(candidate);

    while (true) {
        sem_wait(&worker->workSem);
        if (!worker->work) {
            break;
        }

        uint64_t primeMask = 0;
        mpz_set(candidate, worker->baseNumber);
        for (unsigned int index = 0; index < WORKER_CANDIDATES; index++) {
            if (mpz_probab_prime_p(candidate, 2) != 0) {
                primeMask |= UINT64_C(1) << index;
            }
            mpz_add_ui(candidate, candidate, 2);
        }
        worker->primeMask = primeMask;

        sem_post(&worker->resultSem);
    }

    mpz_clear(candidate);
    return NULL;
}

static Worker *createWorker(void) {
    Worker *worker = (Worker *) malloc(sizeof(Worker));
    if (worker == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    worker->work = true;
    worker->primeMask = 0;

    if (sem_init(&worker->workSem, 0, 0) == -1 || sem_init(&worker->resultSem, 0, 0) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    mpz_init(worker->baseNumber);

    const int error = pthread_create(&worker->thread, NULL, primeTesterWorker, worker);
    if (error != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }

    return worker;
}

static void waitForWorkerResults(const unsigned int nbrThreads, Worker **const workers, bool *const interrupted) {
    for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
        int waitResult;
        do {
            waitResult = sem_wait(&workers[threadIterator]->resultSem);
        } while (waitResult == -1 && errno == EINTR && !PROCESS_PRIME_NUMBERS_FINISH);

        if (waitResult == -1) {
            *interrupted = true;
            return;
        }
    }
}

mpz_t *primeNumbers(const unsigned long long int rank, const bool print, const bool useJson, unsigned long long int *count) {
    const int configuredProcessors = get_nprocs_conf();
    const unsigned int nbrThreads = (configuredProcessors > 0) ? (unsigned int) configuredProcessors * 2 : 2;
    const unsigned long long int batchCandidates = (unsigned long long int) WORKER_CANDIDATES * nbrThreads;
    unsigned long long int iterator = (rank == 1) ? 1 : 2;
    unsigned long long int capacity = (rank == 0) ? 1000 : rank;

    mpz_t *primeList = (mpz_t *) malloc(sizeof(mpz_t) * capacity);
    if (primeList == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    Worker **workers = (Worker **) malloc(sizeof(Worker *) * nbrThreads);
    if (workers == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
        workers[threadIterator] = createWorker();
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

    mpz_t candidate;
    mpz_init(candidate);

    bool interrupted = false;

    while ((rank == 0 || iterator < rank) && !PROCESS_PRIME_NUMBERS_FINISH) {
        if (rank == 0 && capacity < iterator + batchCandidates) {
            capacity = iterator + batchCandidates + 1000;
            mpz_t *newPrimeList = (mpz_t *) realloc(primeList, sizeof(mpz_t) * capacity);
            if (newPrimeList == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            primeList = newPrimeList;
        }

        for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
            Worker *worker = workers[threadIterator];
            mpz_add_ui(worker->baseNumber, actualNumber, 2ULL * WORKER_CANDIDATES * threadIterator);
            sem_post(&worker->workSem);
        }

        waitForWorkerResults(nbrThreads, workers, &interrupted);
        if (interrupted) {
            goto cleanup;
        }

        for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
            Worker *worker = workers[threadIterator];
            const uint64_t primeMask = worker->primeMask;

            for (unsigned int index = 0; index < WORKER_CANDIDATES; index++) {
                if ((primeMask >> index) & UINT64_C(1)) {
                    if (rank != 0 && iterator >= rank) {
                        break;
                    }

                    mpz_add_ui(candidate, worker->baseNumber, 2 * index);
                    mpz_init_set(primeList[iterator], candidate);
                    if (print) {
                        writeSeparator(useJson);
                        writePrime(primeList[iterator]);
                    }
                    iterator++;
                }
            }
        }

        mpz_add_ui(actualNumber, actualNumber, 2 * batchCandidates);
    }

cleanup:
    for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
        workers[threadIterator]->work = false;
        sem_post(&workers[threadIterator]->workSem);
    }
    for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
        Worker *worker = workers[threadIterator];
        pthread_join(worker->thread, NULL);
        sem_destroy(&worker->workSem);
        sem_destroy(&worker->resultSem);
        mpz_clear(worker->baseNumber);
        free(worker);
    }
    free(workers);
    mpz_clear(actualNumber);
    mpz_clear(candidate);

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