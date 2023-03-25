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
#include <limits.h>

#include "functions.h"

void error_and_die(const char errormsg[]){
	perror(errormsg);
	close(listenfd);
	free(buf);
	exit(EXIT_FAILURE);
}

void die(const char msg[]){
	printf("%s\n", msg);
	close(listenfd);
	free(buf);
	exit(EXIT_FAILURE);
}

void ctrlc(){
	fprintf(stdout,"Bye\n");
	close(listenfd);
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


void sendERR(uint16_t error, char message[], struct client_info client_info){
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

	if(sendto(sockfd, buf, (size_t) ((uintptr_t)b - (uintptr_t)buf), 0, (struct sockaddr*)&client_info.clientaddr, client_info.clientsize) == -1){
		error_and_die("sendERR error");
	}
}

Mode checkMode(struct client_info client_info){
	char* ptr = buf + 2 + strlen(buf + 2) + 1;
	if (!strcasecmp(ptr, "netascii")) {
		sendERR(EROP_ILLTFTPOP, " : netascii not supported.", client_info);
		return -1;
	}
	if (!strcasecmp(ptr, "octet")) {
		return MODE_OCTET;
	}
	if (!strcasecmp(ptr, "mail")) {
		sendERR(EROP_ILLTFTPOP, " : mail mode is obsolete.", client_info);
		return -1;
	}
	sendERR(EROP_ILLTFTPOP, " : wrong mode", client_info);
	return -1;
}

