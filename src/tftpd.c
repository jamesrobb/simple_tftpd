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
#include <time.h>
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

	int req_established = 0;

	while(1) {

		int num_bytes_received;
		char buffer[PACKET_SIZE];
		socklen_t len = (socklen_t) sizeof(client);
		rqpacket_t req_header;

		memset(&buffer, 0, sizeof(buffer));

		num_bytes_received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*) &client, &len);

		if(num_bytes_received == -1) {
			perror("recvfrom error");
		}

		//printf("received %d bytes\n", num_bytes);

		if(num_bytes_received > 0) {
			buffer[PACKET_SIZE-1] = '\0';
			//printf("b: %s\n", buffer);

			read_request_packet(buffer, &req_header);

			printf("opcode: %d\n", req_header.opcode);
			printf("file name: %s\n", req_header.filename);
			printf("mode: %s\n", req_header.mode);

			if(req_header.opcode == OPCODE_READ) {
				req_established = 1;
			}

		} else {
			continue;
		}

		if(req_established) {

			FILE* rq_file;
			char file_location[PACKET_SIZE];
			uint16_t block_number = 1;
			int resend_last = 0;
			int failures = 0;

			strcpy(file_location, "../data/");
			strcat(file_location, req_header.filename);

			printf("file location: %s\n", file_location);

			rq_file = fopen(file_location, "r+b");

			if(rq_file == NULL) {
				perror("error opening file");
			}

			char last_file_buffer[512];
			int last_num_bytes_read = 0;

			while(1) {
				unsigned char file_buffer[512];
				int num_bytes_read = 0;

				if(!resend_last) {
					memset(&file_buffer, 0, sizeof(file_buffer));
					num_bytes_read = fread(file_buffer, 1, 512, rq_file);

					last_num_bytes_read = num_bytes_read;
					strncpy(last_file_buffer, file_buffer, num_bytes_read);
				} else {
					strncpy(file_buffer, last_file_buffer, last_num_bytes_read);
					num_bytes_read = last_num_bytes_read;

					printf("we are retrying!\n");
				}

				printf("num bytes read: %d\n", num_bytes_read);
				int break_to_new_request = 0;

				unsigned char send_buffer[4 + num_bytes_read];
				memset(&send_buffer, 0, sizeof(send_buffer));

				send_buffer[1] = OPCODE_DATA;
				send_buffer[2] = (block_number >> 8) & 0xFF;
				send_buffer[3] = block_number & 0xFF;
				printf("sending block numbers %d %d\n", send_buffer[2], send_buffer[3]);

				for(int i = 4; i < num_bytes_read+4; i++) {
					//printf("f: %d\n", send_buffer[i]);
					send_buffer[i] = file_buffer[i-4];
				}

				printf("send_buffer end\n");

				int num_bytes_sent = sendto(sockfd, send_buffer, 4 + num_bytes_read, 0, (struct sockaddr*) &client, len);

				while(1) {

					memset(&buffer, 0, sizeof(buffer));
					num_bytes_received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*) &client, &len);

					if(num_bytes_received == -1) {
						perror("error when receiving ack for transmission end");
						break_to_new_request = 1;
						block_number--;
						failures++;
					} else {

						int opcode = read_opcode(buffer);
						printf("opcode response is: %d\n", opcode);

						if(opcode == OPCODE_ACK) {
							ackpacket_t ack_packet;
							read_ack_packet(buffer, &ack_packet);

							printf("block_number: %d, ack_num: %d\n", block_number, ack_packet.block_number);

							if(ack_packet.block_number == block_number && num_bytes_read < 512) {
								printf("done sending %s\n", file_location);
								printf("failed blocks: %d\n", failures);
								break_to_new_request = 1;
								resend_last = 0;
							}

							if(ack_packet.block_number != block_number) {
								block_number -= 2;
								resend_last = 1;
								failures++;
							}

							break;
						}

					}

					// we break out of the loop waiting for the ack


				}

				if(break_to_new_request) {
					break;
				}

				block_number++;

			}

			fclose(rq_file);

		}


	}


	close(sockfd);
	return 0;
}
