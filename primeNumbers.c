#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define HELP_MESSAGE "Params :\n\
\t-r (number)\t--rank (number)\t\tSet the max rank of the suite.\n\
\t-v (value)\t--value (value)\t\tSet the max value of the suite.\n\
\t-t (number)\t--threads (number)\tConfigures the number of threads.\n\
\t-js\t\t--json\t\t\tUse json format.\n\
\t-h\t\t--help\t\t\tShow this message.\n"

typedef struct {
	// Worker context
	pthread_t thread;
	sem_t primeNumberBufferedSem;

	unsigned int id;
	unsigned long long int initValue;
	unsigned long long int maxValue;
	unsigned long long int primeNumberBuffer;
	unsigned char primeNumberBuffered;
	unsigned char workCompleted;

	// Global context
	unsigned long long int *primeList;

	unsigned int *progression;
	unsigned int *nbrThreads;
	unsigned char *workInProgress;
} Task;

unsigned char isInteger(long double value) {
	if ((unsigned long long int)(value + 0.5) == value) {
		return 1;
	}
	return 0;
}

void *primeNumbersWorker(void *args) {
	Task *task = (Task*)args;

	unsigned char isPrime;
	register unsigned int i;

	for (register unsigned long long int value = task->initValue; value <= task->maxValue && *task->workInProgress; value += (2 * *task->nbrThreads)) {
		isPrime = 1;
		for (i = 1; i < *task->progression && task->primeList[i] < value >> 1; i++) {
			if (isInteger(value * 1.0 / task->primeList[i])) {
				isPrime = 0;
				break;
			}
		}

		if (isPrime) {
			task->primeNumberBuffer = value;
			task->primeNumberBuffered = 1;
			sem_wait(&task->primeNumberBufferedSem);
		}
	}

	task->workCompleted = 1;
	pthread_exit(NULL);
}

unsigned long long int *primeNumbers(unsigned int *maxRank, unsigned long long int *maxValue, unsigned int nbrThreads) {
	register unsigned int i;

	unsigned int progression = 2;
	unsigned char workInProgress = 1;

	pthread_mutex_t progressionMutex = PTHREAD_MUTEX_INITIALIZER;

	unsigned long long int *primeList = (unsigned long long int*)malloc(sizeof(long long int) * *maxRank);
	primeList[0] = 2;
	primeList[1] = 3;

	Task **tasks = (Task**)malloc(sizeof(Task) * nbrThreads);

	// Initializes all tasks
	for (i = 0; i < nbrThreads; i++) {
		tasks[i] = (Task*)malloc(sizeof(Task));

		sem_init(&tasks[i]->primeNumberBufferedSem, 0, 0);

		tasks[i]->id = i;
		tasks[i]->maxValue = *maxValue;
		tasks[i]->primeNumberBuffered = 0;
		tasks[i]->workCompleted = 0;

		tasks[i]->primeList = primeList;

		tasks[i]->progression = &progression;
		tasks[i]->nbrThreads = &nbrThreads;
		tasks[i]->workInProgress = &workInProgress;
	}

	for (i = 0; i < nbrThreads; i++) {
		tasks[i]->initValue = 5 + 2*i;
		pthread_create(&tasks[i]->thread, NULL, primeNumbersWorker, tasks[i]);
	}

	unsigned int primeListIterator = 1;
	unsigned long long int tmpElement;
	unsigned char allTasksCompleted;

	while(workInProgress) {
		allTasksCompleted = 1;
		for (i = 0; i < nbrThreads; i++) {
			if (!tasks[i]->workCompleted) {
				allTasksCompleted = 0;
			}

			if (tasks[i]->primeNumberBuffered) {
				if (progression >= *maxRank) {
					workInProgress = 0;
					break;
				}
				primeList[progression++] = tasks[i]->primeNumberBuffer;
				tasks[i]->primeNumberBuffered = 0;
				sem_post(&tasks[i]->primeNumberBufferedSem);
			}
		}
		
		if (allTasksCompleted) {
			workInProgress = 0;
			*maxRank = progression;
			break;
		}

		// Bubble sort
		if (primeList[primeListIterator - 1] > primeList[primeListIterator]) {
			tmpElement = primeList[primeListIterator];
			primeList[primeListIterator] = primeList[primeListIterator - 1];
			primeList[primeListIterator - 1] = tmpElement;
		}

		if (primeListIterator++ >= progression - 1) {
			primeListIterator = progression / 2;
		}
	}

	// Free memory
	for (i = 0; i < nbrThreads; i++) {
		sem_destroy(&tasks[i]->primeNumberBufferedSem);
		free(tasks[i]);
	}
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
	unsigned int maxRank = 0;
	unsigned long long int maxValue = __INT64_MAX__;
	unsigned int nbrThreads = 1;
	unsigned char useJson = 0;

	for (register unsigned short int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--rank") == 0 || strcmp(argv[i], "-r") == 0) {
			if (i + 1 < argc) {
				maxRank = atoi(argv[i + 1]);
			} else {
				printf(HELP_MESSAGE);
				return -1;
			}
		} else if (strcmp(argv[i], "--value") == 0 || strcmp(argv[i], "-v") == 0) {
			if (i + 1 < argc) {
				maxValue = atoi(argv[i + 1]);
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
		} else if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-js") == 0) {
			useJson = 1;
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			printf(HELP_MESSAGE);
			return 0;
		}
	}

	if (maxRank == 0) {
		printf(HELP_MESSAGE);
		return -1;
	}

	unsigned long long int *primeList = primeNumbers(&maxRank, &maxValue, nbrThreads);

	if (nbrThreads > 1) {
		sort(primeList, maxRank);
	}

	if (useJson) {
		printf("[");
	}

	for (register unsigned long long int i = 0; i < maxRank; i++) {
		if (useJson) {
			printf("%d", *(primeList + i));
			if (i + 1 != maxRank) {
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
