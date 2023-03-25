#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "rrq.h"
#include "options.h"
#include "functions.h"

static bool rrq_check_opcode(){
	if (get_opcode() == OP_ERR){
		printf("Server error : %s\n", buf + ERR_HEADER_SIZE);
		return false;
	} 
	if (get_opcode() != OP_DAT){
		sendERR(EROP_ILLTFTPOP, "Illegal opcode.", server_info);
		return false;
	}
	
	return true;
}

int send_rrq(){
	char* b;
	ssize_t recvsize;

	insert_opcode(OP_RRQ);
	b = buf + RRQWRQ_HEADER_SIZE;
	b += snprintf(b, MAX_FILENAME_SIZE, "%s", filename) + 1;
	b += sprintf(b, "octet") + 1;

	options_parse(&b);

	//send RRQ
	if (sendto(sockfd, buf, (uintptr_t)b - (uintptr_t)buf, 0, (struct sockaddr*)&server_info.addr, server_info.size) == -1) {
		perror("send rrq");
		return -1;
	}

	//receive answer
	server_info.size = sizeof(struct sockaddr);
	if ((recvsize = recvfrom(sockfd, buf, options.blksize + DATA_HEADER_SIZE, 0, (struct sockaddr*)&server_info.addr, &server_info.size)) == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			perror("rrq recvfrom");
			return -1;
		}
		printf("Timeout : Server not answering\n");
		return -1;
	}

	if (get_opcode() == OP_DAT){
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

	return recvsize;
}

bool handle_rrq(){
	int recvsize;
	int recvtimeout;
	uint16_t blockno = 1;

	if ((recvsize = send_rrq()) == -1){
		return false;
	}

	if ((filefd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666)) == -1){
		perror("rrq open");
		return false;
	}

	// If we didn't get an OACK, do this once
	if (get_opcode() == OP_DAT){
		if (write(filefd, buf + DATA_HEADER_SIZE, recvsize - DATA_HEADER_SIZE) == -1){
			perror("rrq write");
			return false;
		}

		blockno = get_blockno();
	} else {
		blockno = 0;
	}

	insert_opcode(OP_ACK);
	insert_blockno(blockno);

	if (sendto(sockfd, buf, ACK_SIZE, 0, (struct sockaddr*)&server_info.addr, server_info.size) == -1){
		perror("rrq sendto");
		return false;
	}

	do {
		recvtimeout = 0;

		while (1) {
			recv_info.size = sizeof(struct sockaddr_in);
			if ((recvsize = recvfrom(sockfd, buf, options.blksize + DATA_HEADER_SIZE, 0, (struct sockaddr*)&recv_info.addr, &recv_info.size)) == -1){
				if (errno != EAGAIN && errno != EWOULDBLOCK){
					perror("rrq recvfrom");
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
				printf("Error : Server timeout reached\n");
				return false;
			}

		};

		if (rrq_check_opcode() == false)
			return false;

		if (write(filefd, buf + DATA_HEADER_SIZE, recvsize - DATA_HEADER_SIZE) == -1){
			perror("rrq write");
			return false;
		}

		blockno = get_blockno();

		insert_opcode(OP_ACK);
		insert_blockno(blockno);

		if (sendto(sockfd, buf, ACK_SIZE, 0, (struct sockaddr*)&server_info.addr, server_info.size) == -1){
			perror("rrq sendto");
			return false;
		}

	} while (recvsize == options.blksize + DATA_HEADER_SIZE);

	return true;
}