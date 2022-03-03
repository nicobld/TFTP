#include "../includes/functions.h"

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


int toclient(){
	int 				blksize = 512; //512 by default
	int  				file, filesize, recvvalue;
	char				ackbuf[4];
	unsigned int		blockno = 1;
	ssize_t 			recvsize;
	int 				recvcount = 0;
	char 				filename[512];

	srand((unsigned) time(NULL));

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		error_and_die("fork socket error");
	}
	
	struct timeval timeout = {RECV_TIMEOUT_TIME,0}; //sec and usec
	//set recv timeout
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1){
		error_and_die("setsockop error");
	}


	if (connect(sockfd, (SA*)&clientaddr, sizeof(clientaddr)) == -1){
		error_and_die("connect error");
	}

	//forbid access to other directories
	strncpy(filename, buf + 2, blksize);
	if (buf[2] == '.' || buf[2] == '/'){
		sendERR(EROP_ILLTFTPOP," : you cannot change directory.");
	}

	// --------------------------- RRQ ---------------------------------
	if (buf[0] == 0 && buf[1] == OP_RRQ){
		checkMode();

		if ((file = open(filename,O_RDONLY)) == -1){
			sendERR(EROP_FILNFOUND, NULL);
		}

		while (1){
			if ((filesize = read(file, buf + 4, blksize)) == -1){
				error_and_die("read error");
			}

			recvcount = 0;
			do {
				buf[1] = OP_DAT;
				buf[2] = (unsigned int) blockno >> 8;
				buf[3] = (unsigned char) blockno;

				if (send(sockfd, buf, filesize+4, 0) == -1){
					error_and_die("sendto error");
				}

				if (filesize < blksize) goto FINRRQ;

				if (recv(sockfd, buf, blksize + 4, 0) == -1){
					if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("recv error");
					recvcount++;
				}
				else recvcount++;
				if(recvcount >= RECV_TIMEOUT_COUNT) die("RECV_TIMEOUT error");


			} while ((unsigned int)(buf[2] << 8) + (unsigned char) buf[3] != blockno);

			blockno ++;
		}
		FINRRQ:
		printf("Read request success : %s\n",filename);
	}

	// --------------------------- WRQ -------------------------------
	if (buf[0] == 0 && buf[1] == OP_WRQ){
		checkMode();

		if ((file = open(filename, O_CREAT|O_WRONLY|O_EXCL, 0666)) == -1){
			sendERR(EROP_FILALREXI, NULL);
		}

		blockno = 0;
		//answer to WRQ is blockno 0
		buf[2] = 0;
		buf[3] = 0;
		recvvalue = blksize + 4;

		while (1){
			recvcount = 0;
			do {
				buf[1] = OP_ACK;
				if (send(sockfd, buf, 4, 0) == -1){
					error_and_die("send error");
				}

				if (recvvalue < blksize + 4) goto FINWRQ;

				if ((recvvalue = recv(sockfd, buf, blksize + 4, 0)) == -1){
					if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("recv error");
					recvcount++;
					if (recvcount >= RECV_TIMEOUT_COUNT) error_and_die ("RECV_TIMEOUT error");
				}

			} while (recvvalue == -1);

			if (write(file, buf + 4, recvvalue - 4) == -1){
				error_and_die("write error");
			}
		}
		FINWRQ:
		printf("Write request sucess : %s\n", filename);
	}

	close(sockfd);
	close(file);
	exit(EXIT_SUCCESS);

}

void sendERR(int error, char message[]){
	buf[0] = 0;
	buf[1] = OP_ERR;

	buf[2] = 0;
	buf[3] = error;

	switch (error){
		case EROP_FILNFOUND:
			strncpy(buf+4,"File not found.",16);
			break;
		case EROP_ILLTFTPOP:
			strncpy(buf+4,"Illegal TFTP operation.",24);
			break;
		case EROP_FILALREXI:
			strncpy(buf+4,"File already exists.",21);
			break;
		default:
			strncpy(buf+4,"Undefined error.",17);
			break;
	}
	//add the additionnal message
	if (message != NULL) strncpy(buf + 4 + strlen(buf+4) - 1, message, strlen(message)+1);

	if(sendto(sockfd, buf, 4 + strlen(buf+4)+1, 0, (SA*)&clientaddr, sizeof(clientaddr)) == -1){
		error_and_die("sendERR error");
	}
	exit(EXIT_SUCCESS);
}

Mode checkMode(){
	char* ptr = buf + 2 + strlen(buf + 2) + 1;
	if (!strncasecmp(ptr, "netascii", 8)) sendERR(EROP_ILLTFTPOP, " : netascii not supported.");
	if(!strncasecmp(ptr,"octet",5)) return octet;
	if(!strncasecmp(ptr,"mail",4)) sendERR(EROP_ILLTFTPOP, " : mail mode is obsolete.");

}

