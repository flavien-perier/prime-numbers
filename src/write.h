#ifndef WRITE_H
#define WRITE_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

extern FILE *PRIME_NUMBER_OUTPUT_FILE;

void openOutputFile(FILE **file, const char *fileName);
void closeOutputFile(void);
void writeOutput(const char *str);

#endif