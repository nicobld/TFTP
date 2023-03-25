#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "options.h"
#include "tftp.h"
#include "functions.h"

void options_set_default_blksize(){
	options.blksize = BLKSIZE_DEFAULT;
	options.isblksize = false;
}

void options_set_default_all(){
	options_set_default_blksize();
}

void options_parse(char** b){
	if (options.isblksize == true){
		*b += sprintf(*b, "blksize") + 1;
		*b += sprintf(*b, "%d", options.blksize) + 1;
	}
}

bool options_OACK_blksize(char* s){
	int newblksize;

	if (options.isblksize == false){
		return false; //TODO interrompre la communication, option reçue mais pas demandée.. nan en faite jsp
	}

	if ((s = strstr(s, "blksize")) == NULL){
		return false;
	}

	long strtol_val;
	if (strtol_check(&strtol_val, s + strlen("blksize")+1) == false){
		printf("strtol_check error\n");
		return false;
	}
	newblksize = strtol_val;

	if (newblksize != options.blksize){
		printf("Options error : server sent a different blksize\n");
		return false;
	}

	options.blksize = newblksize;
	return true;
}