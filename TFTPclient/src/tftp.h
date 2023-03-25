#pragma once
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "options.h"

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

#define RRQWRQ_HEADER_SIZE 2
#define DATA_HEADER_SIZE 4
#define ERR_HEADER_SIZE 4
#define ACK_SIZE 4

#define BLKSIZE_MIN 8
#define BLKSIZE_MAX 65464

#define DEBUG printf("yousk2\n")

#define MAXBUF 128
#define MAX_FILENAME_SIZE 128
#define BLKSIZE_DEFAULT 512
#define RECV_TIMEOUT_TIME 5
#define RECV_TIMEOUT_MAX 2

typedef enum Mode {
	MODE_NETASCII,
	MODE_OCTET,
	MODE_MAIL
} Mode;

struct server_info {
	struct sockaddr_in addr;
	socklen_t size;
};

extern char* buf;
extern char filename[MAX_FILENAME_SIZE];
extern int sockfd;
extern int filefd;
extern struct server_info server_info;
extern struct server_info recv_info;
extern options_t options;