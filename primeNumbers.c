#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define HELP_MESSAGE "Params :\n\t-r (number)\t--rank (number)\t\tGet the rank of the suite.\n\t-t (number)\t--threads (number)\tConfigures the number of threads.\n\t-js\t\t--json\t\t\tUse json format.\n"

#define OPERATIONS_BEFORE_NEW_THREAD 50

typedef struct {
	// Worker context
	unsigned int id;
	pthread_t thread;
	sem_t workCompletedSem;
	unsigned long long int primeNumberBuffer;
	unsigned char primeNumberBuffered;
	sem_t primeNumberBufferedSem;
	unsigned char workCompleted;

	// Global context
	unsigned int *rank;
	unsigned int *progression;
	unsigned int *nbrThreads;
	unsigned int *nbrMaxThreads;
	unsigned long long int *initValue;

	unsigned long long int *primeList;

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

	for (register unsigned long long int i = *task->initValue; *task->progression < *task->rank; i = i + (2 * *task->nbrThreads)) {
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

			if (task->id == 0 && *task->nbrThreads < *task->nbrMaxThreads && *task->progression % OPERATIONS_BEFORE_NEW_THREAD == 0) {
				*task->nbrThreads += 1;
				*task->initValue = i + 2;
				sem_post(task->workerOrchestrator);
			}
		}
	}

	task->workCompleted = 1;
	sem_post(&task->workCompletedSem);
	pthread_exit(NULL);
}

unsigned long long int *primeNumbers(unsigned int rank, unsigned int nbrMaxThreads) {
	unsigned int i;

	unsigned int nbrThreads = 1;
	unsigned long long int initValue = 5;

	unsigned int progression = 2;
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

		tasks[i]->id = i;
		sem_init(&tasks[i]->workCompletedSem, 0, 0);
		sem_init(&tasks[i]->primeNumberBufferedSem, 0, 0);
		tasks[i]->primeNumberBuffered = 0;

		tasks[i]->workCompleted = 0;

		tasks[i]->rank = &rank;
		tasks[i]->nbrThreads = &nbrThreads;
		tasks[i]->nbrMaxThreads = &nbrMaxThreads;

		tasks[i]->progression = &progression;

		tasks[i]->primeList = primeList;

		tasks[i]->initValue = &initValue;

		tasks[i]->workerOrchestrator = &workerOrchestrator;
	}

	// Launches threads
	for (i = 0; i < nbrMaxThreads; i++) {
		pthread_create(&tasks[i]->thread, NULL, primeNumbersWorker, tasks[i]);
	}

	// Wait threads
	unsigned char allTasksBuffered;
	unsigned char alltasksCompleted = 0;
	while(!alltasksCompleted) {
		// Test if all tasks as buffered
		allTasksBuffered = 1;
		alltasksCompleted = 1;
		for (i = 0; i < nbrThreads; i++) {
			if (!tasks[i]->primeNumberBuffered) {
				allTasksBuffered = 0;
			}
			if (!tasks[i]->workCompleted) {
				alltasksCompleted = 0;
			}
		}

		if (allTasksBuffered) {
			for (i = 0; i < nbrThreads; i++) {
				primeList[progression++] = tasks[i]->primeNumberBuffer;
				tasks[i]->primeNumberBuffered = 0;
				sem_post(&tasks[i]->primeNumberBufferedSem);
			}
		}
	}

	// Awaits all tasks
	for (i = 0; i < nbrMaxThreads; i++) {
		sem_wait(&tasks[i]->workCompletedSem); 
	}

	// Free memory
	for (i = 0; i < nbrMaxThreads; i++) {
		sem_destroy(&tasks[i]->workCompletedSem);
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
