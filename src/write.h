#ifndef WRITE_H
#define WRITE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "write.h"

extern FILE* PRIME_NUMBER_OUTPUT_FILE;

void openOutputFile(FILE** file, char* fileName);

void closeOutputFile();

void write(char* str);

#endif