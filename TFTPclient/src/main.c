#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>

#include "tftp.h"
#include "functions.h"
#include "rrq.h"
#include "wrq.h"
#include "options.h"

int sockfd;
char* buf;
char filename[MAXBUF];
int filefd;
options_t options = {
	.isblksize = false,
	.blksize = BLKSIZE_DEFAULT,
};
struct server_info server_info;
struct server_info recv_info;


void toserver(){
	//------------------------------- RRQ -----------------------------------------
	if (!strncmp(buf, "get", 3)){
		if (handle_rrq() == false){
			printf("Read request failed.\n");
		} else {
			printf("Read request success.\n");
		}
	}

	//------------------------------- WRQ -----------------------------------------
	else if (!strncmp(buf, "put", 3)){
		if (handle_wrq() == false){
			printf("Write request failed.\n");
		} else {
			printf("Write request success.\n");
		}
	}

	options_set_default_all();
}

int main(int argc, char* argv[]){
	ssize_t readsize;
	bool direct_command = false;

	buf = calloc(options.blksize + 4, sizeof(char));

	if(signal(SIGINT, ctrlc) == SIG_ERR){
		ctrlc();
	}

	if (argc < 3){
		printf("Usage : %s \"server\" \"port\"\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (argc > 3){
		char* b = buf;
		size_t bytes = 0;

		direct_command = true;
		for (int a = 3; a < argc; a++){
			bytes += snprintf(b, options.blksize - bytes, "%s ", argv[a]);
			b += bytes;
		}
	}
	/*
	struct addrinfo hints = {0};

	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_socktype = SOCK_DGRAM;

	if (getaddrinfo(argv[1], argv[2], &hints, &addr) == -1){
		fprintf(stderr, "getaddrinfo error\n");
		exit(EXIT_FAILURE);
	}
	*/

	memset(&server_info.addr, 0, sizeof(struct sockaddr_in));
	server_info.addr.sin_family = AF_INET;
	server_info.addr.sin_port = htons(atoi(argv[2]));
	if (inet_aton(argv[1], &(server_info.addr.sin_addr)) == 0) {
		printf("Invalid adress\n");
		exit(EXIT_FAILURE);
	}

	if (direct_command == true){
		if (getcommand() == -1){
			return EXIT_FAILURE;
		}

		if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			perror("socket");
			exit(EXIT_FAILURE);
		}

		struct timeval timeout = {RECV_TIMEOUT_TIME, 0}; //sec and usec
		//set recv timeout
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, (socklen_t)sizeof(timeout)) == -1) {
			error_and_die("setsockop error");
		}
		
		filefd = -1;
		server_info.size = sizeof(struct sockaddr_in);
		
		toserver();

		close(sockfd);
		if (filefd != -1) close(filefd);
		return 0;
	}

	printf("\
Welcome to the TFTP client.\n\
 To get a file:\n\
   get \"filename\"\n\
 To put a file:\n\
  put \"filename\"\n\
 You can add options after the filename such as:\n\
  blksize <int>\n"
);

	while (1) {
		do {
			printf("> ");
			fflush(stdout);
			if ((readsize = read(STDIN_FILENO, buf, MAXBUF)) == -1)
				error_and_die("read");

			buf[readsize - 1] = '\0';
		} while (getcommand() == -1);

		if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			perror("socket");
			exit(EXIT_FAILURE);
		}

		struct timeval timeout = {RECV_TIMEOUT_TIME, 0}; //sec and usec
		//set recv timeout
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, (socklen_t)sizeof(timeout)) == -1) {
			error_and_die("setsockop error");
		}
		
		filefd = -1;
		server_info.size = sizeof(struct sockaddr_in);
		
		toserver();

		close(sockfd);
		if (filefd != -1) close(filefd);
	}
	return 0;
}