#!/bin/bash

rm primeNumbers 2>/dev/null

gcc primeNumbers.c -lpthread -lm -o primeNumbers
if [ $? -eq 0 ]
then
    echo "Build 0K"
else
    echo "Build FAILED"
    exit -1
fi

./test.sh
