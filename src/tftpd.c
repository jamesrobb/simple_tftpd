/* your code goes here. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include "packet.h"
#include "util.h"


int main(int argc, char *argv[]) {

	// variable declerations
	int sockfd, connfd;
	int port;
	struct sockaddr_in server, client;

	// the server only takes on argument, the port
	if(argc != 2) {
		print_usage();
		exit(1);
	}

	port = atoi(argv[1]);

	// create and bind a tcp socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1) {
		perror("unable to create socket");
		exit(1);
	}

	// zero out the server struct and set the appropriate configurations
	// we use hton functions to convert values to network byte order
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);

	printf("port %d\n", port);

	if(bind(sockfd, (struct sockaddr*) &server, (socklen_t) sizeof(server)) < 0) {
		perror("unable to bind socket");
		exit(1);
	}

	// start listening and set a backlog of 1
	//listen(sockfd, 1);

	while(1) {

		int num_bytes;
		char buffer[PACKET_SIZE];
		socklen_t len = (socklen_t) sizeof(client);

		memset(&buffer, 0, sizeof(buffer));

		num_bytes = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*) &client, &len);

		if(num_bytes == -1) {
			perror("recvfrom error");
		}

		//printf("received %d bytes\n", num_bytes);

		if(num_bytes > 0) {
			buffer[PACKET_SIZE-1] = '\0';
			//printf("b: %s\n", buffer);

			rqpacket_t req_header;
			read_header_packet(buffer, &req_header);

			printf("opcode: %d\n", req_header.opcode);
			printf("file name: %s\n", req_header.filename);
		}

		// printf("inside outter loop\n");
		//
		// socklen_t len = (socklen_t) sizeof(client);
		// int connfd = accept(sockfd, (struct sockaddr*) &client, &len);
		// if(connfd == -1) {
		// 	perror("unable to create client connection");
		// 	continue;
		// }
		//
		// printf("accepted client connection\n");
		//
		// char buffer[PACKET_SIZE];
		// char data_buffer[DATA_SIZE];
		// int receive_request = 1;
		// int num_bytes;
		//
		// FILE* read_file;
		//
		// memset(&buffer, 0, sizeof(buffer));
		// memset(&data_buffer, 0, sizeof(data_buffer));
		//
		// while(receive_request) {
		//
		// 	printf("inside inner loop\n");
		//
		// 	num_bytes = recv(connfd, buffer, PACKET_SIZE, 0);
		// 	if(num_bytes == -1) {
		// 		perror("receive error");
		// 		break;
		// 	}
		//
		//
		// 	printf("%s\n", buffer);
		//
		//
		// 	break;
		// }
		//
		// break;

	}


	close(sockfd);
	return 0;
}
