#ifndef WRITE_H
#define WRITE_H

#include <stdio.h>
#include <string.h>

#include "context.h"
#include "write.h"

void openOutputFile(FILE** file, char* fileName);

void closeOutputFile();

void write(char* str);

#endif