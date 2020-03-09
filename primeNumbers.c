#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define HELP_MESSAGE "Params :\n\t-r (number)\t--rank (number)\t\tGet the rank of the suite.\n\t-t (number)\t--threads (number)\tConfigures the number of threads.\n\t-js\t\t--json\t\t\tUse json format.\n"

#define OPERATIONS_BEFORE_NEW_THREAD 50

typedef struct {
	// Worker context
	pthread_t thread;

	unsigned long long int primeNumberBuffer;
	unsigned char primeNumberBuffered;
	sem_t primeNumberBufferedSem;

	// Global context
	unsigned long long int *primeList;

	unsigned int *rank;
	unsigned int *progression;
	unsigned int *nbrThreads;
	unsigned long long int *initValue;
	unsigned char *workInProgress;

	sem_t *workerOrchestrator;
} Task;

unsigned char isInteger(long double value) {
	if ((unsigned long long int)(value + 0.5) == value) {
		return 1;
	}
	return 0;
}

void *primeNumbersWorker(void *args) {
	Task *task = (Task*)args;

	sem_wait(task->workerOrchestrator);

	unsigned char isPrime;

	for (register unsigned long long int i = *task->initValue; *task->workInProgress; i = i + (2 * *task->nbrThreads)) {
		isPrime = 1;
		for (register unsigned int j = 1; j < *task->progression && task->primeList[j] < i >> 1; j++) {
			if (isInteger(i * 1.0 / task->primeList[j])) {
				isPrime = 0;
				break;
			}
		}

		if (isPrime) {
			task->primeNumberBuffer = i;
			task->primeNumberBuffered = 1;
			sem_wait(&task->primeNumberBufferedSem);
		}
	}

	pthread_exit(NULL);
}

unsigned long long int *primeNumbers(unsigned int rank, unsigned int nbrMaxThreads) {
	register unsigned int i;

	unsigned int nbrThreads = 1;
	unsigned int progression = 2;
	unsigned long long int initValue = 5;
	unsigned char workInProgress = 1;

	pthread_mutex_t progressionMutex = PTHREAD_MUTEX_INITIALIZER;

	unsigned long long int *primeList = (unsigned long long int*)malloc(sizeof(long long int) * rank);
	primeList[0] = 2;
	primeList[1] = 3;

	Task **tasks = (Task**)malloc(sizeof(Task) * nbrMaxThreads);

	sem_t workerOrchestrator;
	sem_init(&workerOrchestrator, 0, 1);

	// Initializes all tasks
	for (i = 0; i < nbrMaxThreads; i++) {
		tasks[i] = (Task*)malloc(sizeof(Task));

		tasks[i]->primeNumberBuffered = 0;
		sem_init(&tasks[i]->primeNumberBufferedSem, 0, 0);

		tasks[i]->primeList = primeList;

		tasks[i]->rank = &rank;
		tasks[i]->progression = &progression;
		tasks[i]->nbrThreads = &nbrThreads;
		tasks[i]->initValue = &initValue;
		tasks[i]->workInProgress = &workInProgress;

		tasks[i]->workerOrchestrator = &workerOrchestrator;
	}

	// Launches threads
	for (i = 0; i < nbrMaxThreads; i++) {
		pthread_create(&tasks[i]->thread, NULL, primeNumbersWorker, tasks[i]);
	}

	// Wait threads
	unsigned int oldNbrThreads;
	unsigned int primeListIterator = 1;
	unsigned int tmpListElement;
	while(workInProgress) {
		oldNbrThreads = nbrThreads;

		// Test if task as buffered
		for (i = 0; i < oldNbrThreads; i++) {
			if (tasks[i]->primeNumberBuffered) {
				if (progression >= rank) {
					workInProgress = 0;
					break;
				}
				primeList[progression++] = tasks[i]->primeNumberBuffer;
				tasks[i]->primeNumberBuffered = 0;
				sem_post(&tasks[i]->primeNumberBufferedSem);
				
				if (progression % OPERATIONS_BEFORE_NEW_THREAD == 0 && nbrThreads < nbrMaxThreads) {
					nbrThreads++;
					initValue = tasks[i]->primeNumberBuffer + 2;
					sem_post(&workerOrchestrator);
				}
			}
		}

		// Bubble sort
		if (primeList[primeListIterator - 1] > primeList[primeListIterator]) {
			tmpListElement = primeList[primeListIterator];
			primeList[primeListIterator] = primeList[primeListIterator - 1];
			primeList[primeListIterator - 1] = tmpListElement;
		}
		if (primeListIterator++ >= progression - 1) {
			primeListIterator = 1;
		}
	}

	// Free memory
	for (i = 0; i < nbrMaxThreads; i++) {
		sem_destroy(&tasks[i]->primeNumberBufferedSem);
		free(tasks[i]);
	}
	sem_destroy(&workerOrchestrator);
	free(tasks);

	return primeList;
}

int main(int argc, char* argv[]) {
	unsigned int rank = 0;
	unsigned int nbrThreads = 1;
	unsigned char useJson = 0;

	for (register unsigned short int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--rank") == 0 || strcmp(argv[i], "-r") == 0) {
			if (i + 1 < argc) {
				rank = atoi(argv[i + 1]);
			} else {
				printf(HELP_MESSAGE);
				return -1;
			}
		} else if (strcmp(argv[i], "--threads") == 0 || strcmp(argv[i], "-t") == 0) {
			if (i + 1 < argc) {
				nbrThreads = atoi(argv[i + 1]);
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

	unsigned long long int *primeList = primeNumbers(rank, nbrThreads);

	if (useJson) {
		printf("[");
	}

	for (register unsigned long long int i = 0; i < rank; i++) {
		if (useJson) {
			printf("%d", *(primeList + i));
			if (i + 1 != rank) {
				printf(",");
			}
		} else {
			printf("%d\n", *(primeList + i));
		}
	}

	if (useJson) {
		printf("]\n");
	}

	free(primeList);

	return 0;
}
