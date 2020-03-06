#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define HELP_MESSAGE "Params :\n\t-r (number)\t--rank (number)\t\tGet the rank of the suite.\n\t-t (number)\t--threads (number)\tConfigures the number of threads.\n\t-js\t\t--json\t\t\tUse json format.\n"

#define OPERATIONS_BEFORE_NEW_THREAD 50

typedef struct {
	unsigned int id;

	unsigned int *rank;
	unsigned int *nbrThreads;
	unsigned int *nbrMaxThreads;
	unsigned long long int *primeList;

	unsigned long long int *initValue;
	unsigned int *initProgression;

	sem_t *workerOrchestrator;
	pthread_t thread;
	sem_t sem;
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

	unsigned int progression = *task->initProgression;
	unsigned char isPrime;

	for (register unsigned long long int i = *task->initValue; progression < *task->rank; i = i + (2 * *task->nbrThreads)) {
		isPrime = 1;
		for (register unsigned int j = 1; j < progression && task->primeList[j] < i >> 1; j++) {
			if (isInteger(i * 1.0 / task->primeList[j])) {
				isPrime = 0;
				break;
			}
		}

		if (isPrime) {
			task->primeList[progression] = i;
			progression += *task->nbrThreads;

			if (task->id == 0 && *task->nbrThreads < *task->nbrMaxThreads && progression % OPERATIONS_BEFORE_NEW_THREAD == 0) {
				*task->nbrThreads += 1;
				*task->initValue = i + 2;
				*task->initProgression = progression + 1;
				sem_post(task->workerOrchestrator);
			}
		}
	}

	sem_post(&task->sem);
	pthread_exit(NULL);
}

unsigned long long int *primeNumbers(unsigned int rank, unsigned int nbrMaxThreads) {
	unsigned int nbrThreads = 1;
	unsigned long long int initValue = 5;
	unsigned int initProgression = 2;

	unsigned long long int *primeList = (unsigned long long int*)malloc(sizeof(long long int) * rank);
	primeList[0] = 2;
	primeList[1] = 3;

	Task **tasks = (Task**)malloc(sizeof(Task) * nbrMaxThreads);

	sem_t workerOrchestrator;
	sem_init(&workerOrchestrator, 0, 1);

	// Initializes all tasks
	for (unsigned int i = 0; i < nbrMaxThreads; i++) {
		tasks[i] = (Task*)malloc(sizeof(Task));
		tasks[i]->id = i;

		tasks[i]->rank = &rank;
		tasks[i]->nbrThreads = &nbrThreads;
		tasks[i]->nbrMaxThreads = &nbrMaxThreads;
		tasks[i]->primeList = primeList;

		tasks[i]->initValue = &initValue;
		tasks[i]->initProgression = &initProgression;

		tasks[i]->workerOrchestrator = &workerOrchestrator;
		sem_init(&tasks[i]->sem, 0, 0);
	}

	// Launches threads
	for (unsigned int i = 0; i < nbrMaxThreads; i++) {
		pthread_create(&tasks[i]->thread, NULL, primeNumbersWorker, tasks[i]);
	}

	// Awaits all tasks
	for (unsigned int i = 0; i < nbrMaxThreads; i++) {
		sem_wait(&tasks[i]->sem);
	}

	// Free memory
	for (unsigned int i = 0; i < nbrMaxThreads; i++) {
		sem_destroy(&tasks[i]->sem);
		free(tasks[i]);
	}
	sem_destroy(&workerOrchestrator);
	free(tasks);

	return primeList;
}

void sort(unsigned long long int *primeList, unsigned int length) {
	unsigned long long int min;
	unsigned long long int max;
	unsigned int minIndex;
	unsigned int maxIndex;

	for (register unsigned int i = 0; i < length / 2; i++) {
		min = __INT64_MAX__;
		max = 0;

		for (register unsigned int j = i; j < length - i; j++) {
			if (primeList[j] <= min) {
				min = primeList[j];
				minIndex = j;
			}

			if (primeList[j] >= max) {
				max = primeList[j];
				maxIndex = j;
			}
		}

		primeList[minIndex] = primeList[i];
		primeList[i] = min;
		primeList[maxIndex] = primeList[length - i - 1];
		primeList[length - i - 1] = max;
	}
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

	if (nbrThreads > 1) {
		sort(primeList, rank);
	}

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
	return 0;
}
