#pragma once
#include <stdbool.h>

typedef struct options {
	bool isblksize;
	int blksize;
} options_t;

/* Sets blksize option to default */
void options_set_default_blksize();

/* Sets all options to default */
void options_set_default_all();

/* Parse options to send to server */
void options_parse(char** b);

/* Check that the server accepted the blksize option */
bool options_OACK_blksize(char* s);