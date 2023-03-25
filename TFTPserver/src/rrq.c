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
#include <stdbool.h>

#include "tftp.h"
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

void handle_rrq(ssize_t rrqrecvsize, struct client_info client_info){
	int recvsize;
	int recvtimeout;
	int readsize;
	uint16_t blockno = 1;
	struct client_info recv_info;
	// char* filebuf;

	if ((filefd = open(buf + RRQWRQ_HEADER_SIZE, O_RDONLY)) == -1){
		sendERR(EROP_FILNFOUND, NULL, client_info);
		return;
	}

	if (options_parse(rrqrecvsize) == true){
		char* b;
		b = buf;
		b += RRQWRQ_HEADER_SIZE;

		b += strlen(b) + 1;
		b += strlen(b) + 1;
		int oack_sendsize = rrqrecvsize - ((uintptr_t)b - (uintptr_t)buf) + 2;
		memmove(buf + 2, b, oack_sendsize);
		insert_opcode(OP_OACK);

		if (sendto(sockfd, buf, oack_sendsize, 0, (struct sockaddr*)&client_info.clientaddr, client_info.clientsize) == -1){
			perror("sendto error");
			return;
		}

		recv_info.clientsize = sizeof(struct sockaddr_in);
		if ((rrqrecvsize = recvfrom(sockfd, buf, ACK_SIZE, 0, (struct sockaddr*)&recv_info.clientaddr, &recv_info.clientsize)) == -1){
			perror("recvfrom error");
			return;
		}

		if (recv_info.clientaddr.sin_port != client_info.clientaddr.sin_port){
			fprintf(stderr, "Unknown TID\n");
			sendERR(EROP_UNKOWNTID, NULL, recv_info);
		}

		if (get_blockno() != 0){
			fprintf(stderr, "Blockno error\n");
			sendERR(EROP_NOTDEF, "Wrong blockno.", client_info);
			return;
		}

		if (rrq_check_opcode() == false){
			fprintf(stderr, "Illegal opcode\n");
			sendERR(EROP_ILLTFTPOP, "Illegal opcode.", client_info);
			return;
		}
	}

	// filebuf = malloc(options.blksize);

	while (1) {

		recvtimeout = 0;
		while (1) {
			insert_opcode(OP_DAT);
			insert_blockno(blockno);

			if ((readsize = read(filefd, buf + 4, options.blksize)) == -1){
				perror("read error");
				return;
			}

			if (sendto(sockfd, buf, readsize + DATA_HEADER_SIZE, 0, (struct sockaddr*)&client_info.clientaddr, client_info.clientsize) == -1){
				error_and_die("sendto error");
			}

			recv_info.clientsize = sizeof(struct sockaddr_in);
			if ((recvsize = recvfrom(sockfd, buf, ACK_SIZE, 0, (struct sockaddr*)&recv_info.clientaddr, &recv_info.clientsize)) == -1){
				if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("recvfrom error");
				recvtimeout++;
			}

			if (recv_info.clientaddr.sin_port != client_info.clientaddr.sin_port){
				fprintf(stderr, "Unknown TID\n");
				sendERR(EROP_UNKOWNTID, NULL, recv_info);
			} else if (get_blockno() != blockno){
				fprintf(stderr, "Wrong blockno\n");
				sendERR(EROP_NOTDEF, "Wrong blockno.", client_info);
				return;
			} else if (recvsize != -1) {
				break;
			}

			if (recvtimeout >= RECV_TIMEOUT_MAX) {
				printf("Error : sendtimeout reached\n");
				return;
			}
		}

		if (rrq_check_opcode() == false){
			fprintf(stderr, "Illegal opcode.\n");
			sendERR(EROP_ILLTFTPOP, "Illegal opcode.", client_info);
			return;
		}

		if (readsize < options.blksize)
			break;

		blockno++;
	}

	printf("Read request success\n");
	return;
}