#!/bin/bash

EXPECTED_RESULT_100=`cat ./test/primeNumbers-100.json`
EXPECTED_RESULT_1000=`cat ./test/primeNumbers-1000.json`

# r=100
RESULT=`./primeNumbers -r 100 -js`
if [ $? -eq 0 ]
then
    echo "Execution (r=100) 0K"
else
    echo "Execution (r=100) FAILED"
    echo $RESULT
    exit -1
fi

if [ $RESULT = $EXPECTED_RESULT_100 ]
then
    echo "Test (r=100) 0K"
else
    echo "Test (r=100) FAILED"
    echo $RESULT
    exit -1
fi

# r=1000
RESULT=`./primeNumbers --rank 1000 --json`
if [ $? -eq 0 ]
then
    echo "Execution (r=1000) 0K"
else
    echo "Execution (r=1000) FAILED"
    echo $RESULT
    exit -1
fi

if [ $RESULT = $EXPECTED_RESULT_1000 ]
then
    echo "Test (r=1000) 0K"
else
    echo "Test (r=1000) FAILED"
    echo $RESULT
    exit -1
fi