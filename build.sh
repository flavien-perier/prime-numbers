#!/bin/bash

rm primeNumbers

gcc primeNumbers.c -lpthread -lm -o primeNumbers
if [ $? -eq 0 ]
then
    echo "Build 0K"
else
    echo "Build FAILED"
    exit -1
fi

RESULT=`./primeNumbers -r 100 -js`
if [ $? -eq 0 ]
then
    echo "Execution 0K"
else
    echo "Execution FAILED"
    echo $RESULT
    exit -1
fi

EXPECTED_RESULT="[2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,541]"
if [ $RESULT = $EXPECTED_RESULT ]
then
    echo "Test 0K"
else
    echo "Test FAILED"
    echo $RESULT
    exit -1
fi