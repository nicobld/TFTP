#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "options.h"
#include "tftp.h"
#include "functions.h"

void options_set_default_blksize(){
	printf("options set to default\n");
	options.blksize = BLKSIZE_DEFAULT;
}

void options_set_default_all(){
	options_set_default_blksize();
}

bool options_parse(ssize_t recvsize){
	char* b;

	b = buf;
	b += RRQWRQ_HEADER_SIZE;

	b += strlen(b) + 1;
	b += strlen(b) + 1;

	if ((uintptr_t)b - (uintptr_t)buf == (unsigned long) recvsize){
		return false;
	}

	while ((uintptr_t)b - (uintptr_t)buf < (unsigned long) recvsize){
		printf("New option %s\n", b);
		if (strcmp(b, "blksize") == 0){
			b += strlen("blksize") + 1;

			long strtol_val;
			if (strtol_check(&strtol_val, b) == false)
				return false;


			if (options.blksize < BLKSIZE_MIN || options.blksize > BLKSIZE_MAX){
				printf("Chose blocksize between %u and %u\n", BLKSIZE_MIN, BLKSIZE_MAX);
				return false;
			}

			options.blksize = strtol_val;
			buf = realloc(buf, options.blksize + 4);
		}

		b += strlen(b) + 1;
	}

	return true;
}