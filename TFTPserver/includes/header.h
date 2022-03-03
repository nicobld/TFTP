#ifndef HEADER_H
#define HEADER_H

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DAT 3
#define OP_ACK 4
#define OP_ERR 5
#define OP_OACK 6

#define EROP_NOTDEF 0
#define EROP_FILNFOUND 1
#define EROP_ACCESVIOL 2
#define EROP_DISKFULL 3
#define EROP_ILLTFTPOP 4
#define EROP_UNKOWNTID 5
#define EROP_FILALREXI 6
#define EROP_NOSUCHUSR 7

#define SA struct sockaddr
#define DEBUG fprintf(stdout,"debug\n")
#define BASE_PORT 1069
#define RECV_TIMEOUT_COUNT 3
#define RECV_TIMEOUT_TIME 5

typedef enum Mode {
	netascii,
	octet,
	mail
} Mode;

#endif