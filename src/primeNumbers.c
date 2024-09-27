#include "primeNumbers.h"

unsigned char PROCESS_PRIME_NUMBERS_FINISH = 0;

typedef struct {
    // Thread
    pthread_t thread;
    sem_t workSem;
    sem_t resultSem;
    pthread_mutex_t primeListMutex;

    // Context
    mpz_t *primeList;
    unsigned char work;

    // Task
    unsigned long long int iterator;
    mpz_t testedNumber;
    unsigned char isPrime;
} Worker;

void *primeTesterWorker(void *args) {
    Worker *worker = (Worker *) args;

    register unsigned long long int explorerIterator;

    mpz_t modulo;
    mpz_init(modulo);

    mpz_t sqrtTestedNumber;
    mpz_init(sqrtTestedNumber);

    while (worker->work) {
        sem_wait(&worker->workSem);
        worker->isPrime = 1;

        mpz_sqrt(sqrtTestedNumber, worker->testedNumber);

        for (explorerIterator = 1; explorerIterator <= worker->iterator / 2; explorerIterator++) {
            pthread_mutex_lock(&worker->primeListMutex);
            mpz_mod(modulo, worker->testedNumber, worker->primeList[explorerIterator]);

            if (mpz_cmp(worker->primeList[explorerIterator], sqrtTestedNumber) > 0) {
                pthread_mutex_unlock(&worker->primeListMutex);
                break;
            }
            pthread_mutex_unlock(&worker->primeListMutex);

            if (mpz_cmp_ui(modulo, 0) == 0) {
                worker->isPrime = 0;
                break;
            }
        }

        sem_post(&worker->resultSem);
    }

    return NULL;
}

mpz_t *primeNumbers(unsigned long long int rank, unsigned char print, unsigned char useJson) {
    const unsigned int nbrMaxThread = get_nprocs_conf() * 2;
    unsigned int nbrThreads = 1;
    register unsigned int threadIterator, threadIterator2;

    mpz_t *primeList = (mpz_t *) malloc(sizeof(mpz_t) * (rank == 0 ? 1000 : rank));
    PROCESS_PRIME_NUMBERS_FINISH = 0;

    // Create workers thread pool.
    Worker **workers = (Worker **) malloc(sizeof(Worker) * nbrMaxThread);
    for (threadIterator = 0; threadIterator < nbrMaxThread; threadIterator++) {
        workers[threadIterator] = (Worker *) malloc(sizeof(Worker));

        sem_init(&workers[threadIterator]->workSem, 0, 0);
        sem_init(&workers[threadIterator]->resultSem, 0, 0);
        pthread_mutex_init(&workers[threadIterator]->primeListMutex, NULL);
        mpz_init(workers[threadIterator]->testedNumber);
        workers[threadIterator]->primeList = primeList;
        workers[threadIterator]->work = 1;
        pthread_create(&workers[threadIterator]->thread, NULL, primeTesterWorker, workers[threadIterator]);
    }

    mpz_init_set_ui(primeList[0], 2);
    mpz_init_set_ui(primeList[1], 3);

    mpz_t actualNumber;
    mpz_init_set_ui(actualNumber, 5);

    mpz_t modulo;
    mpz_init(modulo);

    if (print) {
        write(mpz_get_str(NULL, 10, primeList[0]));
        write(useJson ? "," : "\n");
        write(mpz_get_str(NULL, 10, primeList[1]));
        write(useJson ? "," : "\n");
    }

    register unsigned long long int iterator = 2;

    while ((iterator < rank || rank == 0) && !PROCESS_PRIME_NUMBERS_FINISH) {
        // Send jobs to thread pool
        for (threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
            mpz_add_ui(workers[threadIterator]->testedNumber, actualNumber, threadIterator * 2);
            workers[threadIterator]->iterator = iterator;

            sem_post(&workers[threadIterator]->workSem);
        }

        // Get results in thread pool
        for (threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
            sem_wait(&workers[threadIterator]->resultSem);

            if (workers[threadIterator]->isPrime) {
                if (rank == 0 && iterator % 1000 == 0) {
                    for (threadIterator2 = 0; threadIterator2 < nbrThreads; threadIterator2++) {
                        pthread_mutex_lock(&workers[threadIterator2]->primeListMutex);
                    }

                    primeList = realloc(primeList, sizeof(mpz_t) * (iterator + 1000));

                    for (threadIterator2 = 0; threadIterator2 < nbrThreads; threadIterator2++) {
                        workers[threadIterator2]->primeList = primeList;
                        pthread_mutex_unlock(&workers[threadIterator2]->primeListMutex);
                    }
                }

                mpz_init_set(primeList[iterator], workers[threadIterator]->testedNumber);

                if (print) {
                    write(mpz_get_str(NULL, 10, primeList[iterator]));
                    if ((iterator + 1 != rank || rank == 0) && !PROCESS_PRIME_NUMBERS_FINISH) {
                        write(useJson ? "," : "\n");
                    } else {
                        PROCESS_PRIME_NUMBERS_FINISH = 1;
                        break;
                    }
                }

                iterator++;
            }
        }

        mpz_add_ui(actualNumber, actualNumber, 2 * nbrThreads);

        if (nbrThreads < iterator && nbrThreads < nbrMaxThread) {
            nbrThreads += 1;
        }
    }

    for (register unsigned int i = 0; i < nbrMaxThread; i++) {
        workers[i]->work = 0;
        sem_destroy(&workers[i]->workSem);
        sem_destroy(&workers[i]->resultSem);
        pthread_mutex_destroy(&workers[i]->primeListMutex);
        free(workers[i]);
    }
    free(workers);

    return primeList;
}

void stopPrimeNumbers() {
    PROCESS_PRIME_NUMBERS_FINISH = 1;
}