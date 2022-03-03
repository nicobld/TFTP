#ifndef HEADER_H
#define HEADER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <signal.h>

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

#define DEBUG printf("yousk2\n")

#define MAXBUF 128
#define DEFAULT_BLKSIZE 512
#define RECV_TIMEOUT_TIME 5
#define RECV_TIMEOUT_MAX 2

extern char* buf;
extern char filename[MAXBUF];
extern int sockfd;
extern struct sockaddr_in servaddr;
#endif