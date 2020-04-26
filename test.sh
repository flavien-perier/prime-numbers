#!/bin/bash

EXPECTED_RESULT_100=`cat primeNumbers-100.json`
EXPECTED_RESULT_1000=`cat primeNumbers-1000.json`

# r=100, t=1

RESULT=`./primeNumbers -r 100 -js`
if [ $? -eq 0 ]
then
    echo "Execution (r=100, t=1) 0K"
else
    echo "Execution (r=100, t=1) FAILED"
    echo $RESULT
    exit -1
fi

if [ $RESULT = $EXPECTED_RESULT_100 ]
then
    echo "Test (r=100, t=1) 0K"
else
    echo "Test (r=100, t=1) FAILED"
    echo $RESULT
    exit -1
fi

# r=100, t=2, v=541

RESULT=`./primeNumbers -r 100 -v 541 -t 2 -js`
if [ $? -eq 0 ]
then
    echo "Execution (r=100, t=2, v=541) 0K"
else
    echo "Execution (r=100, t=2, v=541) FAILED"
    echo $RESULT
    exit -1
fi

if [ $RESULT = $EXPECTED_RESULT_100 ]
then
    echo "Test (r=100, t=2, v=541) 0K"
else
    echo "Test (r=100, t=2, v=541) FAILED"
    echo $RESULT
    exit -1
fi

# r=1000, t=4, v=7919

RESULT=`./primeNumbers -r 1000 -v 7919 -t 4 -js`
if [ $? -eq 0 ]
then
    echo "Execution (r=1000, t=4, v=7919) 0K"
else
    echo "Execution (r=1000, t=4, v=7919) FAILED"
    echo $RESULT
    exit -1
fi

if [ $RESULT = $EXPECTED_RESULT_1000 ]
then
    echo "Test (r=1000, t=4, v=7919) 0K"
else
    echo "Test (r=1000, t=4, v=7919) FAILED"
    echo $RESULT
    exit -1
fi
