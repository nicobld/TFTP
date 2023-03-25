#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

#include "wrq.h"
#include "options.h"
#include "functions.h"

static bool rrq_check_opcode(){
	if (get_opcode() == OP_ERR){
		printf("Client error : %s\n", buf + ERR_HEADER_SIZE);
		return false;
	}
	else if (get_opcode() != OP_ACK){
		fprintf(stderr, "Illegal packet received\n");
		return false;
	} 
	return true;
}

int send_wrq(){
	char* b;
	// int recvtimeout = 0;

	insert_opcode(OP_WRQ);
	b = buf + 2;
	b += snprintf(b, MAX_FILENAME_SIZE, "%s", filename) + 1;
	b += sprintf(b, "octet") + 1;

	options_parse(&b);

	//send WRQ
	if (sendto(sockfd, buf, (uintptr_t)b - (uintptr_t)buf, 0, (struct sockaddr*)&server_info.addr, server_info.size) == -1){
		perror("wrq sendto");
		return -1;
	}

	//receive answer
	server_info.size = sizeof(struct sockaddr_in);
	if (recvfrom(sockfd, buf, options.blksize + 4, 0, (struct sockaddr*)&server_info.addr, &server_info.size) == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) perror("wrq recvfrom");
		fprintf(stderr, "Timeout : Server not answering\n");
		return -1;
	}


	if (get_opcode() == OP_ACK){
		options_set_default_all();
	}

	else if (get_opcode() == OP_ERR){
		printf("Error packet received : %s\n", buf + ERR_HEADER_SIZE);
		return -1;
	}

	else if (get_opcode() == OP_OACK){
		if (options_OACK_blksize(buf + 2) == false){
			options_set_default_blksize();
		}
	}

	else {
		printf("Illegal packet received\n");
		return -1;
	}

	return 0;
}

bool handle_wrq(){
	int recvsize;
	int recvtimeout;
	int datalength;
	uint16_t blockno = 1;

	if ((filefd = open(filename, O_RDONLY)) == -1){
		perror("wrq open");
		return false;
	}

	if (send_wrq() == -1) {
		return false;
	}

	while (1) {

		if ((datalength = read(filefd, buf + DATA_HEADER_SIZE, options.blksize)) == -1){
			perror("wrq read");
			return false;
		}

		insert_opcode(OP_DAT);
		insert_blockno(blockno);

		recvtimeout = 0;
		while (1) {
			if (sendto(sockfd, buf, datalength + DATA_HEADER_SIZE, 0, (struct sockaddr*)&server_info.addr, server_info.size) == -1){
				perror("wrq sendto");
				return false;
			}

			recv_info.size = sizeof(struct sockaddr_in);
			if ((recvsize = recvfrom(sockfd, buf, ACK_SIZE, 0, (struct sockaddr*)&recv_info.addr, &recv_info.size)) == -1){
				if (errno != EAGAIN && errno != EWOULDBLOCK) {
					perror("wrq recvfrom");
					return false;
				}
				recvtimeout++;
			}

			if (recv_info.addr.sin_port != server_info.addr.sin_port){
				sendERR(EROP_UNKOWNTID, NULL, recv_info);
			} 
			
			else if (recvsize != -1){
				break;
			}

			if (recvtimeout >= RECV_TIMEOUT_MAX) {
				fprintf(stderr, "Server timeout reached\n");
				return false;
			}
		}

		if (rrq_check_opcode() == false){
			sendERR(EROP_ILLTFTPOP, "Illegal opcode.", server_info);
			return false;
		}

		else if (get_blockno() != blockno){
			fprintf(stderr, "Wrong blockno received\n");
			return false;
		}

		if (datalength < options.blksize) {
			break;
		}

		blockno++;
	}

	return true;
}