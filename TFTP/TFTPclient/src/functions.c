#include "../includes/functions.h"

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
    printf("%s\n", diemsg);
    close(filefd);
    close(sockfd);
    free(buf);
    exit(EXIT_SUCCESS);
}

/* Gets the command writen by user
    return -1 if error*/
int getcommand(int* options, int* blksize){
    char *token;
    char *endptr;
    int optionvalue;
    char *ptrfile;

    *options = 0;
    *blksize = DEFAULT_BLKSIZE;

    if ((token = strtok(buf, " ")) == NULL){
        printf("Error, wrong command\n");
        return -1;
    }

    if (strncmp(token, "get", 3) != 0 && strncmp(token, "put",3) != 0){
        printf("Error, wrong command\n");
        return -1;
    }
    
    if ((token = strtok(NULL, " ")) == NULL){
        printf("Please enter a filename\n");
        return -1;
    }
    ptrfile = token;

    if ((token = strtok(NULL, " ")) != NULL){
        optionvalue = strtol(token, &endptr, 10);

        if (token == endptr){
            printf("Invalid blocksize\n");
            return -1;
        }

        if (optionvalue < 8 || optionvalue > 65464){
            printf("Chose blocksize between 8 and 65464\n");
            return -1;
        }

        *blksize = optionvalue;
        *options = 1;
    }

    strncpy(filename, buf + 4, strlen(ptrfile) + 1);
    return 0;
}

void toserver(int options, int blksize){
    int filenamesize = strlen(filename) + 1;
    int datalength;
    int recvtimeout;
    int blockno = 1;

    struct timeval timeout = { RECV_TIMEOUT_TIME,0 }; //sec and usec
    socklen_t timeoutsize = sizeof(timeout);
    //set recv timeout
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        error_and_die("setsockop error");
    }

    //------------------------------- RRQ -----------------------------------------
    if (!strncmp(buf, "get", 3)){

        if (sendRRQ(options, &blksize) == -1) return;

        if ((filefd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666)) == -1) {
            if (errno == EEXIST) {
                printf("File already exists\n");
                return;
            } error_and_die("open file error");
        }

        datalength = blksize + 4;

        while (1) {
            recvtimeout = 0;

            if (buf[0] != 0 || (buf[1] != OP_ERR && buf[1] != OP_DAT)) {
                printf("Illegal packet received\n");
                return;
            }

            //error packet
            if (buf[1] == OP_ERR){
                printf("Error packet reveived : %s\n", buf + 4);
                return;
            }


            write(filefd, buf + 4, datalength - 4);

            buf[0]=0;
            buf[1]=OP_ACK;

            do {
                if (send(sockfd, buf, 4, 0) == -1){
                    perror("send ack");
                    exit(EXIT_FAILURE);
                }

                if (datalength > 0 && datalength < blksize + 4) { // THE NORMAL EXIT FOR THE WHILE
                    goto ENDRRQ;
                }

                if ((datalength = recv(sockfd, buf, blksize + 4, 0)) == -1){
                    if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("sendto RRQ error");
                    recvtimeout++;
                }
                if (recvtimeout >= RECV_TIMEOUT_MAX) {
                    printf("Error : timeout reached\n");
                    return;
                }
            } while (datalength == -1);
        }

        ENDRRQ:
        printf("Read request succes for %s\n", filename);
        return;
        
    }

    else if (!strncmp(buf, "put", 3)){
        int recvsize;
        if (sendWRQ(options, &blksize) == -1) return;

        if ((filefd = open(filename, O_RDONLY)) == -1) {
            error_and_die("open file error");
        }

        datalength = blksize + 4;

        while (1) {
            recvtimeout = 0;

            if (buf[0] != 0 || (buf[1] != OP_ERR && buf[1] != OP_ACK)) {
                printf("Illegal packet received\n");
                return;
            }

            //error packet
            if (buf[1] == OP_ERR){
                printf("Server error : %s\n", buf + 4);
                return;
            }

            if ((datalength = read(filefd, buf + 4, blksize - 4)) == -1){
                perror("read error");
                return;
            }

            buf[0]=0;
            buf[1]=OP_DAT;

            do {
                if (send(sockfd, buf, 4, 0) == -1){
                    perror("send ack");
                    exit(EXIT_FAILURE);
                }

                if (datalength < blksize - 4) { // THE NORMAL EXIT FOR THE WHILE
                    goto ENDWRQ;
                }

                if ((recvsize = recv(sockfd, buf, blksize + 4, 0)) == -1){
                    if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("sendto RRQ error");
                    recvtimeout++;
                }
                if (recvtimeout >= RECV_TIMEOUT_MAX) {
                    printf("Error : timeout reached\n");
                    return;
                }
            } while (recvsize == -1);
        }

        ENDWRQ:
        printf("Write request succes for %s\n", filename);
        return;
    }

}

/* Sends an RRQ to server, reveives the answer
    changes blksize in case of an option */
int sendRRQ(int options, int* blksize){
    int filenamesize = strlen(filename) + 1;
    char blksizestring[8];
    int blksizestringsize;
    socklen_t servaddrsize = sizeof(servaddr);


    buf[0] = 0;
    buf[1] = OP_RRQ;
    strncpy(buf + 2, filename, filenamesize);
    strncpy(buf + 2 + filenamesize, "octet", 6);

    //blksize option
    if (options != 0) {
        strncpy(buf + 2 + filenamesize + 6, "blksize", 8);
        sprintf(blksizestring, "%d", *blksize);
        blksizestringsize = strlen(blksizestring) + 1;
        strncpy(buf + 2 + filenamesize + 6 + 8, blksizestring, blksizestringsize);
    }


    //send RRQ
    if (sendto(sockfd, buf, 2 + filenamesize + 6 + (options ? 8 + blksizestringsize : 0), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        error_and_die("sendto RRQ error");
    }

    //receive answer
    if (recvfrom(sockfd, buf, *blksize + 4, 0, (struct sockaddr*)&servaddr, &servaddrsize) == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("sendto RRQ error");
        printf("Timeout : Server not answering\n");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        error_and_die("connect error");
    }


    if (options != 0) {
        int recvtimeout = 0;
        //if receive DATA then option not confirmed
        if (buf[1] == OP_DAT) {
            *blksize = DEFAULT_BLKSIZE;
            printf("blksize not accepted by server\n");
            return 0;
        }

        if (buf[1] != OP_ERR) {
            printf("Error packet received : %s\n", buf + 2);
            return -1;
        }

        if (buf[1] != OP_OACK){
            printf("Illegal packet received\n");
            return -1;
        }

        if (strncmp(buf + 2, "blksize", 8) != 0) {
            printf("Server error wrong option received\n");
            sendERR(8, NULL);
            return -1;
        }

        if (strtol(buf + 9, NULL, 10) != *blksize) {
            printf("Server error blksizes don't match\n");
            sendERR(8, NULL);
            return -1;
        }

        //if it's all good, we send an ACK
        buf[2] = 0;
        buf[3] = 0;
        while (recvtimeout < RECV_TIMEOUT_MAX){
            if (send(sockfd, buf, 4, 0) == -1){
                error_and_die("send error");
            }

            if (recv(filefd, buf, *blksize + 4, 0) == -1){
                if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("sendto RRQ error");
                recvtimeout++;
            }
        } if (recvtimeout == RECV_TIMEOUT_MAX) {
            printf("Timeout reached\n");
            return -1;
        }

        return 0;
    }
}

int sendWRQ(int options, int* blksize){
    int filenamesize = strlen(filename) + 1;
    char blksizestring[8];
    int blksizestringsize;
    socklen_t servaddrsize = sizeof(servaddr);


    buf[0] = 0;
    buf[1] = OP_WRQ;
    strncpy(buf + 2, filename, filenamesize);
    strncpy(buf + 2 + filenamesize, "octet", 6);

    //blksize option
    if (options != 0) {
        strncpy(buf + 2 + filenamesize + 6, "blksize", 8);
        sprintf(blksizestring, "%d", *blksize);
        blksizestringsize = strlen(blksizestring) + 1;
        strncpy(buf + 2 + filenamesize + 6 + 8, blksizestring, blksizestringsize);
    }


    //send WRQ
    if (sendto(sockfd, buf, 2 + filenamesize + 6 + (options ? 8 + blksizestringsize : 0), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        error_and_die("sendto RRQ error");
    }

    //receive answer
    if (recvfrom(sockfd, buf, *blksize + 4, 0, (struct sockaddr*)&servaddr, &servaddrsize) == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("sendto WRQ error");
        printf("Timeout : Server not answering\n");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        error_and_die("connect error");
    }

    if (options != 0) {
        int recvtimeout = 0;
        //if receive ACK then option not confirmed
        if (buf[1] == OP_ACK) {
            *blksize = DEFAULT_BLKSIZE;
            printf("blksize not accepted by server\n");
            return 0;
        }

        if (buf[1] != OP_ERR) {
            printf("Error packet received : %s\n", buf + 2);
            return -1;
        }

        if (buf[1] != OP_OACK){
            printf("Illegal packet received\n");
            return -1;
        }

        if (strncmp(buf + 2, "blksize", 8) != 0) {
            printf("Server error wrong option received\n");
            sendERR(8, NULL);
            return -1;
        }

        if (strtol(buf + 9, NULL, 10) != *blksize) {
            printf("Server error blksizes don't match\n");
            sendERR(8, NULL);
            return -1;
        }

        //if it's all good, we send an ACK
        buf[2] = 0;
        buf[3] = 0;
        while (recvtimeout < RECV_TIMEOUT_MAX){
            if (send(sockfd, buf, 4, 0) == -1){
                error_and_die("send error");
            }

            if (recv(filefd, buf, *blksize + 4, 0) == -1){
                if (errno != EAGAIN && errno != EWOULDBLOCK) error_and_die("sendto RRQ error");
                recvtimeout++;
            }
        } if (recvtimeout == RECV_TIMEOUT_MAX) {
            printf("Timeout reached\n");
            return -1;
        }
        return 0;
    }

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
        case 8:
            strncpy(buf+4,"Wrong option.",14);
            break;
		default:
			strncpy(buf+4,"Undefined error.",17);
			break;
	}
	//add the additionnal message
	if (message != NULL) strncpy(buf + 4 + strlen(buf+4) - 1, message, strlen(message)+1);

	if(send(sockfd, buf, 4 + strlen(buf+4)+1, 0) == -1){
		error_and_die("sendERR error");
	}
}