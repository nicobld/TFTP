#pragma once
#include <stdbool.h>


typedef struct options {
	int blksize;
} options_t;

/* Sets blksize option to default */
void options_set_default_blksize();

/* Sets all options to default */
void options_set_default_all();

/* Get options given by client */
bool options_parse(ssize_t recvsize);

/* Check that the server accepted the blksize option */
bool options_OACK_blksize(char* s);