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
#include <sys/time.h>

#include "functions.h"
#include "tftp.h"
#include "options.h"
#include "rrq.h"
#include "wrq.h"

char* buf;
int listenfd; //the socket that listens on port 69 for new clients
int sockfd;
int filefd;
struct sockaddr_in 	servaddr;
socklen_t servaddrsize = sizeof(struct sockaddr_in);
options_t options = {
	.blksize = BLKSIZE_DEFAULT,
};

int toclient(ssize_t recvsize, struct client_info client_info){
	srand((unsigned) time(NULL));

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		error_and_die("fork socket error");
	}
	
	struct timeval timeout = {RECV_TIMEOUT_TIME, 0}; //sec and usec
	//set recv timeout
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1){
		error_and_die("setsockop error");
	}

	//forbid access to other directories
	// strncpy(filename, buf + 2, blksize);
	// if (buf[2] == '.' || buf[2] == '/'){//TODO
	// 	sendERR(EROP_ILLTFTPOP," : you cannot change directory.");
	// }

	if (checkMode() != MODE_OCTET){
		return -1;
	}

	// --------------------------- RRQ ---------------------------------
	if (get_opcode() == OP_RRQ){
		handle_rrq(recvsize, client_info);
	}

	// --------------------------- WRQ -------------------------------
	
	else if (get_opcode() == OP_WRQ){
		handle_wrq(recvsize, client_info);
	}

	else {
		sendERR(EROP_ILLTFTPOP, "Unrecognized opcode", client_info);
		return -1;
	}

	return 0;
}

int main(){
	buf = calloc(512 + 4, sizeof(char));

	if(signal(SIGINT, ctrlc) == SIG_ERR){
		ctrlc();
	}


	if ((listenfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		error_and_die("socket error");
	}


	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family 	 = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port		 = htons(BASE_PORT);

	if((bind(listenfd, (SA *)&servaddr, sizeof(servaddr))) == -1){
		printf("bind error on port %d\n", BASE_PORT);
		error_and_die(NULL);
	}

	printf("waiting for a connection on port %d\n", BASE_PORT);

	ssize_t recvsize;
	struct client_info client_info;
	client_info.clientsize = sizeof(SA);
	while(1){
		if ((recvsize = recvfrom(listenfd, buf, 512 + 4, 0, (SA *)&client_info.clientaddr, &client_info.clientsize)) == -1){
			error_and_die("1069 recvfrom error");
		}

		if(fork() == 0){
			close(listenfd);
			toclient(recvsize, client_info);
			exit(EXIT_SUCCESS);
		}
	}
}


