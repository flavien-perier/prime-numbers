#include "write.h"

FILE *PRIME_NUMBER_OUTPUT_FILE = NULL;

void openOutputFile(FILE **file, const char *fileName) {
    struct stat buffer;
    if (stat(fileName, &buffer) == 0) {
        fprintf(stderr, "File already exists: %s\n", fileName);
        exit(EXIT_FAILURE);
    }

    *file = fopen(fileName, "a");
    if (*file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
}

void closeOutputFile(void) {
    if (PRIME_NUMBER_OUTPUT_FILE != NULL) {
        if (fclose(PRIME_NUMBER_OUTPUT_FILE) != 0) {
            perror("fclose");
        }
        PRIME_NUMBER_OUTPUT_FILE = NULL;
    }
}

void writeOutput(const char *const str) {
    if (PRIME_NUMBER_OUTPUT_FILE == NULL) {
        if (printf("%s", str) < 0) {
            perror("printf");
            exit(EXIT_FAILURE);
        }
    } else {
        if (fprintf(PRIME_NUMBER_OUTPUT_FILE, "%s", str) < 0) {
            perror("fprintf");
            exit(EXIT_FAILURE);
        }
    }
}