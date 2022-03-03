#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "header.h"

void error_and_die(const char[]);
void ctrlc();

int toclient();
void sendERR(int error, char[]);
Mode checkMode();

extern char* buf;
extern int listenfd; //the socket that listens on port 69 for new clients
extern int sockfd;
extern struct sockaddr_in 	servaddr;
extern struct sockaddr_in 	clientaddr;

#endif