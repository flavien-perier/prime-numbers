#define _GNU_SOURCE
#include "primeNumbers.h"

#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

#define WORKER_CANDIDATES 64
#define WORKER_WINDOW 4

static const unsigned long WHEEL_INCREMENT[8] = {4, 2, 4, 2, 4, 6, 2, 6};
static const unsigned long WHEEL_OFFSET[8] = {0, 4, 6, 10, 12, 16, 22, 24};
static const unsigned long SPECIAL_PRIMES[3] = {2, 3, 5};

static volatile sig_atomic_t PROCESS_PRIME_NUMBERS_FINISH = 0;

typedef struct {
    pthread_t thread;
    sem_t workSem;
    sem_t resultSem;

    _Atomic bool work;

    mpz_t workBase[WORKER_WINDOW + 1];
    unsigned long long int workWrite;

    uint64_t primeMask[WORKER_WINDOW + 1];
} Worker;

static void writePrime(const char *const separator, const mpz_t prime) {
    static char *buffer = NULL;
    static size_t bufferCapacity = 0;

    const size_t separatorLength = strlen(separator);
    const size_t needed = mpz_sizeinbase(prime, 10) + separatorLength + 3;
    if (needed > bufferCapacity) {
        char *newBuffer = (char *) realloc(buffer, needed);
        if (newBuffer == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        buffer = newBuffer;
        bufferCapacity = needed;
    }

    memcpy(buffer, separator, separatorLength);
    mpz_get_str(buffer + separatorLength, 10, prime);
    writeOutput(buffer);
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

    unsigned long long int workRead = 0;
    unsigned long long int resultWrite = 0;

    while (true) {
        while (sem_wait(&worker->workSem) == -1 && errno == EINTR);
        if (!atomic_load_explicit(&worker->work, memory_order_acquire)) {
            break;
        }

        mpz_set(candidate, worker->workBase[workRead % (WORKER_WINDOW + 1)]);
        workRead++;

        uint64_t primeMask = 0;
        for (unsigned int index = 0; index < WORKER_CANDIDATES; index++) {
            if (mpz_probab_prime_p(candidate, 2) != 0) {
                primeMask |= UINT64_C(1) << index;
            }
            mpz_add_ui(candidate, candidate, WHEEL_INCREMENT[index & 7]);
        }

        worker->primeMask[resultWrite % (WORKER_WINDOW + 1)] = primeMask;
        resultWrite++;
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

    atomic_init(&worker->work, true);
    worker->workWrite = 0;

    if (sem_init(&worker->workSem, 0, 0) == -1 || sem_init(&worker->resultSem, 0, 0) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    for (unsigned int index = 0; index <= WORKER_WINDOW; index++) {
        mpz_init(worker->workBase[index]);
    }

    const int error = pthread_create(&worker->thread, NULL, primeTesterWorker, worker);
    if (error != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }

    return worker;
}

static void dispatchBlock(Worker *const worker, const mpz_t base) {
    mpz_set(worker->workBase[worker->workWrite % (WORKER_WINDOW + 1)], base);
    worker->workWrite++;
    sem_post(&worker->workSem);
}

mpz_t *primeNumbers(const unsigned long long int rank, const bool print, const bool useJson, const bool store, unsigned long long int *count) {
    const int configuredProcessors = get_nprocs_conf();
    const unsigned int nbrThreads = (configuredProcessors > 0) ? (unsigned int) configuredProcessors : 2;
    const unsigned long long int blockSpan = 30ULL * (WORKER_CANDIDATES / 8);
    const char *const separator = useJson ? "," : "\n";
    unsigned long long int iterator = 0;
    unsigned long long int capacity = (rank == 0) ? 1000 : rank;

    mpz_t *primeList = NULL;
    if (store) {
        primeList = (mpz_t *) malloc(sizeof(mpz_t) * capacity);
        if (primeList == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }

    Worker **workers = (Worker **) malloc(sizeof(Worker *) * nbrThreads);
    if (workers == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
        workers[threadIterator] = createWorker();
    }

    unsigned long long int *const consumed = (unsigned long long int *) calloc(nbrThreads, sizeof(unsigned long long int));
    if (consumed == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    mpz_t specialPrime;
    mpz_init(specialPrime);
    for (unsigned int index = 0; index < 3; index++) {
        if (rank != 0 && iterator >= rank) {
            break;
        }
        mpz_set_ui(specialPrime, SPECIAL_PRIMES[index]);
        if (store) {
            mpz_init_set(primeList[iterator], specialPrime);
        }
        if (print) {
            writePrime((iterator == 0) ? "" : separator, specialPrime);
        }
        iterator++;
    }
    mpz_clear(specialPrime);

    mpz_t nextBase;
    mpz_init_set_ui(nextBase, 7);

    mpz_t candidate;
    mpz_init(candidate);

    unsigned long long int nextDispatch = 0;
    unsigned long long int nextConsume = 0;

    while ((rank == 0 || iterator < rank) && nextDispatch < (unsigned long long int) nbrThreads * WORKER_WINDOW && !PROCESS_PRIME_NUMBERS_FINISH) {
        dispatchBlock(workers[nextDispatch % nbrThreads], nextBase);
        nextDispatch++;
        mpz_add_ui(nextBase, nextBase, blockSpan);
    }

    while ((rank == 0 || iterator < rank) && nextConsume < nextDispatch && !PROCESS_PRIME_NUMBERS_FINISH) {
        Worker *const worker = workers[nextConsume % nbrThreads];

        int waitResult;
        do {
            waitResult = sem_wait(&worker->resultSem);
        } while (waitResult == -1 && errno == EINTR && !PROCESS_PRIME_NUMBERS_FINISH);
        if (waitResult == -1) {
            break;
        }

        if (store && rank == 0 && capacity < iterator + WORKER_CANDIDATES) {
            capacity *= 2;
            mpz_t *newPrimeList = (mpz_t *) realloc(primeList, sizeof(mpz_t) * capacity);
            if (newPrimeList == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            primeList = newPrimeList;
        }

        const unsigned int slot = (unsigned int) (consumed[nextConsume % nbrThreads] % (WORKER_WINDOW + 1));
        const uint64_t primeMask = worker->primeMask[slot];
        const mpz_t *const base = &worker->workBase[slot];

        for (unsigned int index = 0; index < WORKER_CANDIDATES && (rank == 0 || iterator < rank); index++) {
            if ((primeMask >> index) & UINT64_C(1)) {
                mpz_add_ui(candidate, *base, WHEEL_OFFSET[index & 7] + 30ULL * (index >> 3));
                if (store) {
                    mpz_init_set(primeList[iterator], candidate);
                }
                if (print) {
                    writePrime((iterator == 0) ? "" : separator, candidate);
                }
                iterator++;
            }
        }

        consumed[nextConsume % nbrThreads]++;
        nextConsume++;

        if ((rank == 0 || iterator < rank) && !PROCESS_PRIME_NUMBERS_FINISH) {
            dispatchBlock(workers[nextDispatch % nbrThreads], nextBase);
            nextDispatch++;
            mpz_add_ui(nextBase, nextBase, blockSpan);
        }
    }

    for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
        atomic_store_explicit(&workers[threadIterator]->work, false, memory_order_release);
        sem_post(&workers[threadIterator]->workSem);
    }
    for (unsigned int threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
        Worker *worker = workers[threadIterator];
        pthread_join(worker->thread, NULL);
        sem_destroy(&worker->workSem);
        sem_destroy(&worker->resultSem);
        for (unsigned int index = 0; index <= WORKER_WINDOW; index++) {
            mpz_clear(worker->workBase[index]);
        }
        free(worker);
    }
    free(workers);
    free(consumed);
    mpz_clear(nextBase);
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
