#!/bin/bash

N=100

for (( i=0; i<N; i++ ))
do
	diff -q dir${i}/test.txt test.txt
done