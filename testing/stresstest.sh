#!/bin/bash
set -e

cp ../TFTPclient/clientTFTP .

N=100

for (( i=0; i<N; i++ ))
do
	mkdir dir${i}
	cd dir${i}
	./clientTFTP 127.0.0.1 1069 get test.txt &
	cd ..
done