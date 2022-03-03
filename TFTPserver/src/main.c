#include "../includes/header.h"
#include "../includes/functions.h"

char* buf;
int listenfd; //the socket that listens on port 69 for new clients
int sockfd;
struct sockaddr_in 	servaddr;
struct sockaddr_in 	clientaddr;

int main(int argc, char* argv[]){
	socklen_t clientsize;
	buf = calloc(512 + 4, sizeof(char));

	if(signal(SIGINT,ctrlc) == SIG_ERR){
		ctrlc();
	}


	if ((listenfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		error_and_die("socket error");
	}


	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family 	 = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port		 = htons(BASE_PORT);

	if((bind(listenfd, (SA *)&servaddr, sizeof(servaddr))) == -1){
		printf("bind error on port %d\n", BASE_PORT);
		error_and_die(NULL);
	}

	printf("waiting for a connection on port %d\n",BASE_PORT);
	fflush(stdout);

	while(1){
		//wait for a client on port BASE_PORT
		clientsize = sizeof(clientaddr);
		buf = reallocarray(buf, 512 + 4, sizeof(char));
		if (recvfrom(listenfd, buf, 512 + 4, 0, (SA *)&clientaddr, &clientsize) == -1){
			error_and_die("69 recvfrom error");
		}

		if(fork() == 0){ //fork to take care of client
			close(listenfd);
			//where the magic happens
			toclient();
			exit(EXIT_FAILURE);
		}
	}
}


