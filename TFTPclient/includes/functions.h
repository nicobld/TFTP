#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "header.h"
#include <time.h>
#include <stdlib.h>


void ctrlc();
int getcommand(int*, int*);
void die(const char[]);
void error_and_die(const char[]);
void toserver(int,int);
void sendERR(int error, char message[]);
int sendRRQ(int, int*);
int sendWRQ(int, int*);

extern int filefd;


#endif