
#include "../includes/header.h"
#include "../includes/functions.h"

struct sockaddr_in servaddr;
int sockfd;
char* buf;
char filename[MAXBUF];
int filefd;


int main(int argc, char* argv[]){
	int blksize = DEFAULT_BLKSIZE;
	int options = 0; //contains all the TFTP options in the form of bits
	buf = calloc(blksize + 4, sizeof(char));

	if(signal(SIGINT,ctrlc) == SIG_ERR){
		ctrlc();
	}

	if (argc != 3){
		printf("Usage : %s \"server\" \"port\"\n", argv[0]);
		exit(EXIT_FAILURE);
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



	printf("Welcome to the TFTP client.\n To get a file:\n  get \"filename\"\n To put a file:\n  put \"filename\"\n You can add options after the filename such as:\n  -blocksize\n");

	while (1){
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(atoi(argv[2]));
		if (inet_aton(argv[1], &(servaddr.sin_addr)) == 0) {
			printf("Invalid adress\n");
			exit(EXIT_FAILURE);
		}

		if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			perror("socket");
			exit(EXIT_FAILURE);
		}
		
		write(STDOUT_FILENO, "> ", 2);
		buf[read(STDIN_FILENO, buf, MAXBUF) - 1] = '\0';

		if ((getcommand(&options, &blksize)) == -1) continue;
		toserver(options, blksize);
		close(filefd);
		close(sockfd);
	}
}