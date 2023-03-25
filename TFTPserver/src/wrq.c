#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "tftp.h"
#include "functions.h"

static bool wrq_check_opcode(){
	if (get_opcode() == OP_ERR){
		printf("Server error : %s\n", buf + 4);
		return false;
	} 
	if (get_opcode() != OP_DAT){
		return false;
	}
	
	return true;
}

void handle_wrq(ssize_t wrqrecvsize, struct client_info client_info){
	int recvsize;
	int recvtimeout;
	uint16_t blockno = 0;
	struct client_info recv_info;

	if ((filefd = open(buf + RRQWRQ_HEADER_SIZE, O_CREAT | O_EXCL | O_WRONLY, 0666)) == -1){
		sendERR(EROP_FILALREXI, NULL, client_info);
		return;
	}

	if (options_parse(wrqrecvsize) == true){
		char* b;
		b = buf;
		b += RRQWRQ_HEADER_SIZE;

		b += strlen(b) + 1;
		b += strlen(b) + 1;
		int oack_sendsize = wrqrecvsize - ((uintptr_t)b - (uintptr_t)buf) + 2;
		memmove(buf + 2, b, oack_sendsize);
		insert_opcode(OP_OACK);

		if (sendto(sockfd, buf, oack_sendsize, 0, (struct sockaddr*)&client_info.clientaddr, client_info.clientsize) == -1){
			perror("sendto error");
			return;
		}
	} else {
		insert_opcode(OP_ACK);
		insert_blockno(0);

		if (sendto(sockfd, buf, ACK_SIZE, 0, (struct sockaddr*)&client_info.clientaddr, client_info.clientsize) == -1)
			error_and_die("send ack");
		
	}

	do {
		recvtimeout = 0;

		while (1) {
			recv_info.clientsize = sizeof(struct sockaddr_in);
			if ((recvsize = recvfrom(sockfd, buf, options.blksize + DATA_HEADER_SIZE, 0, (struct sockaddr*)&recv_info.clientaddr, &recv_info.clientsize)) == -1){
				if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("recv error");
				recvtimeout++;
			}
			
			if (client_info.clientaddr.sin_port != recv_info.clientaddr.sin_port){
				fprintf(stderr, "error : Unknown TID\n");
				sendERR(EROP_UNKOWNTID, NULL, recv_info);
			}
			
			else if (recvsize != -1) {
				break;
			}

			if (recvtimeout >= RECV_TIMEOUT_MAX) {
				printf("Error : timeout reached\n");
				return;
			}
		}

		if (wrq_check_opcode() == false)
			return;

		if (write(filefd, buf + DATA_HEADER_SIZE, recvsize - DATA_HEADER_SIZE) == -1)
			error_and_die("write");

		blockno = get_blockno();

		insert_opcode(OP_ACK);
		insert_blockno(blockno);

		if (sendto(sockfd, buf, ACK_SIZE, 0, (struct sockaddr*)&client_info.clientaddr, client_info.clientsize) == -1)
			error_and_die("send");

	} while (recvsize == options.blksize + DATA_HEADER_SIZE);

	printf("Write request succes\n");
	return;
}