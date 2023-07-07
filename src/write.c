#include "write.h"

void openOutputFile(FILE** file, char* fileName) {
    *file = fopen(fileName, "a");
}

void closeOutputFile() {
    if (PRIME_NUMBER_OUTPUT_FILE != NULL) {
        fclose(PRIME_NUMBER_OUTPUT_FILE);
    }
}

void write(char* str) {
    if (PRIME_NUMBER_OUTPUT_FILE == NULL) {
        printf("%s", str);
    } else {
        fprintf(PRIME_NUMBER_OUTPUT_FILE, "%s", str);
    }
}