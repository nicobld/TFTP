#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "functions.h"
#include "options.h"
#include "rrq.h"
#include "wrq.h"
#include "tftp.h"

void ctrlc(){
	close(sockfd);
	close(filefd);
	free(buf);
	exit(EXIT_SUCCESS);
}

void error_and_die(const char errormsg[]){
	perror(errormsg);
	close(filefd);
	close(sockfd);
	free(buf);
	exit(EXIT_FAILURE);
}

void die(const char diemsg[]){
	fprintf(stderr, "%s\n", diemsg);
	close(filefd);
	close(sockfd);
	free(buf);
	exit(EXIT_SUCCESS);
}

void insert_blockno(uint16_t blockno){
	buf[2] = (char) blockno >> 8;
	buf[3] = (char) blockno;
}

uint16_t get_blockno(){
	return (uint16_t)(buf[2] << 8) + (uint16_t) buf[3];
}

void insert_opcode(uint16_t opcode){
	buf[0] = (char) opcode >> 8;
	buf[1] = (char) opcode;
}

uint16_t get_opcode(){
	return (uint16_t)(buf[0] << 8) + (uint16_t) buf[1];
}

void insert_errorcode(uint16_t errorcode){
	buf[2] = (char) errorcode >> 8;
	buf[3] = (char) errorcode;
}

uint16_t get_errorcode(){
	return (uint16_t)(buf[2] << 8) + (uint16_t) buf[3];
}

bool strtol_check(long* val, char* str){
	char* endptr;
	errno = 0;	/* To distinguish success/failure after call */
	*val = strtol(str, &endptr, 10);

	/* Check for various possible errors */

	if ((errno == ERANGE && (*val == LONG_MAX || *val == LONG_MIN))
			|| (errno != 0 && *val == 0)) {
		perror("strtol");
		return false;
	}

	if (endptr == str) {
		fprintf(stderr, "No digits were found\n");
		return false;
	}

	return true;
}



/* Gets the command writen by user
	return -1 if error*/
int getcommand(){
	char *token;

	if ((token = strtok(buf, " ")) == NULL){
		printf("Error, wrong command\n");
		return -1;
	}

	if (strncmp(token, "get", 3) != 0 && strncmp(token, "put", 3) != 0){
		printf("Error, wrong command\n");
		return -1;
	}
	
	if ((token = strtok(NULL, " ")) == NULL){
		printf("Please enter a filename\n");
		return -1;
	}

	char* optionname;
	char* optionvalue;
	while ((optionname = strtok(NULL, " ")) != NULL){
		printf("new option: %s\n", optionname);

		if ((optionvalue = strtok(NULL, " ")) == NULL){
			printf("Please add an option value\n");
			return -1;
		}

		if (strcmp(optionname, "blksize") == 0){
			options.isblksize = true;

			long strtol_val;
			if (strtol_check(&strtol_val, optionvalue) == false)
				options_set_default_blksize();

			options.blksize = strtol_val;

			if (options.blksize < BLKSIZE_MIN || options.blksize > BLKSIZE_MAX){
				options_set_default_blksize();
				printf("Chose blocksize between %u and %u\n", BLKSIZE_MIN, BLKSIZE_MAX);
				return -1;
			}
			buf = realloc(buf, options.blksize + 4);
		}
	}

	snprintf(filename, MAX_FILENAME_SIZE, "%s", buf + 4);
	return 0;
}

void sendERR(int error, char message[], struct server_info server_info){
	char* b;
	insert_opcode(OP_ERR);
	insert_errorcode(error);

	b = buf + ERR_HEADER_SIZE;

	switch (error){
		case EROP_FILNFOUND:
			b += sprintf(b, "File not found.") + 1;
			break;
		case EROP_ILLTFTPOP:
			b += sprintf(b, "Illegal TFTP operation.") + 1;
			break;
		case EROP_FILALREXI:
			b += sprintf(b, "File already exists.") + 1;
			break;
		case 8:
			b += sprintf(b, "Wrong option.") + 1;
			break;
		default:
			b += sprintf(b, "Undefined error.") + 1;
			break;
	}
	//add the additionnal message
	if (message != NULL) b += sprintf(b - 1, " %s", message) + 1;

	if(sendto(sockfd, buf, (size_t) ((uintptr_t)b - (uintptr_t)buf), 0, (struct sockaddr*)&server_info.addr, server_info.size) == -1){
		error_and_die("sendERR error");
	}
}


Mode checkMode(){
	char* ptr = buf + RRQWRQ_HEADER_SIZE + strlen(buf + RRQWRQ_HEADER_SIZE) + 1;
	if (!strcasecmp(ptr, "netascii")) sendERR(EROP_ILLTFTPOP, " : netascii not supported.", server_info);
	else if (!strcasecmp(ptr, "octet")) return MODE_OCTET;
	else if (!strcasecmp(ptr, "mail")) sendERR(EROP_ILLTFTPOP, " : mail mode is obsolete.", server_info);
	return -1;
}