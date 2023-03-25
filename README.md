# TFTP
A client/server TFTP in C for Linux

## Build
	./build.sh

## Run
	./TFTPserver/serverTFTP
	./TFTPclient/clientTFTP <ip> <port> [<command>]

## Client example
	./clientTFTP 127.0.0.1 1069

	./clientTFTP 127.0.0.1 1069 get test.txt
	
	./clientTFTP 127.0.0.1 1069 "get test.txt"
	
	./clientTFTP 127.0.0.1 1069 get test.txt blksize 2048