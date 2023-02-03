#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <semaphore.h>

#define HELP_MESSAGE "Params :\n\t-r (number)\t--rank (number)\t\tGet the rank of the suite (> 2).\n\t-js\t\t--json\t\t\tUse json format.\n"

typedef struct {
    // Thread
    pthread_t thread;
    sem_t workSem;
    sem_t resultSem;

    // Context
    mpz_t* primeList;
    unsigned char work;

    // Task
    unsigned long long int iterator;
    mpz_t testedNumber;
    unsigned char isPrime;
} Worker;

void* primeTesterWorker(void *args) {
    Worker *worker = (Worker*)args;

    register unsigned long long int explorerIterator;

    mpz_t modulo;
    mpz_init(modulo);

    while (worker->work) {
        sem_wait(&worker->workSem);
        worker->isPrime = 1;

        for (explorerIterator = 1; explorerIterator < worker->iterator / 2; explorerIterator++) {
            mpz_mod(modulo, worker->testedNumber, worker->primeList[explorerIterator]);

            if (mpz_cmp_ui(modulo, 0) == 0) {
                worker->isPrime = 0;
                break;
            }
        }

        sem_post(&worker->resultSem);
    }
}

mpz_t* primeNumbers(unsigned long long int rank, unsigned char print, unsigned char useJson) {
    const unsigned int nbrMaxThread = get_nprocs_conf();
    unsigned int nbrThreads = 1;
    register unsigned int threadIterator;

    mpz_t* primeList = (mpz_t*) malloc(sizeof(mpz_t) * (rank == 0 ? 1000 : rank));

    // Create workers thread pool.
    Worker** workers = (Worker**) malloc(sizeof(Worker) * nbrMaxThread);
    for (threadIterator = 0; threadIterator < nbrMaxThread; threadIterator++) {
        workers[threadIterator] = (Worker*)malloc(sizeof(Worker));

        sem_init(&workers[threadIterator]->workSem, 0, 0);
        sem_init(&workers[threadIterator]->resultSem, 0, 0);
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
        mpz_out_str(stdout, 10, primeList[0]);
        printf(useJson ? "," : "\n");
        mpz_out_str(stdout, 10, primeList[1]);
        printf(useJson ? "," : "\n");
    }

    register unsigned long long int iterator = 2;

    while (iterator < rank || rank == 0) {
        // Send jobs to thread pool
        for (threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
            mpz_add_ui(workers[threadIterator]->testedNumber, actualNumber, threadIterator);
            workers[threadIterator]->iterator = iterator;

            sem_post(&workers[threadIterator]->workSem);
        }

        // Get results in thread pool
        for (threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
            sem_wait(&workers[threadIterator]->resultSem);

            if (workers[threadIterator]->isPrime) {
                if (rank == 0 && iterator % 1000 == 0) {
                    primeList = realloc(primeList, sizeof(mpz_t) * (iterator + 1000));
                    for (threadIterator = 0; threadIterator < nbrThreads; threadIterator++) {
                        workers[threadIterator]->primeList = primeList;
                    }
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
        }

        if (nbrThreads < iterator && nbrThreads < nbrMaxThread) {
            nbrThreads += 1;
        }

        mpz_add_ui(actualNumber, actualNumber, 2 * nbrThreads);
    }

    for (register unsigned int i = 0; i < nbrMaxThread; i++) {
        workers[i]->work = 0;
        sem_destroy(&workers[i]->workSem);
        sem_destroy(&workers[i]->resultSem);
        free(workers[i]);
    }
    free(workers);

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