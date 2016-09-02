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
#include "clientconn.h"
#include "packet.h"
#include "util.h"

char file_serve_directory[MAX_FILENAME_LENGTH*2];

int main(int argc, char *argv[]) {

	uint8_t max_conns = 2;

	if(argc != 3) {
		print_usage();
		exit(1);
	}

	// variable declerations
	int sockfd, connfd;
	int port;
	struct sockaddr_in server, client;

	// setting up the directory where our files will be served from
	memset(&file_serve_directory, 0, MAX_FILENAME_LENGTH);
	int passed_serve_directory_size = strlen(argv[2]);

	if(passed_serve_directory_size > MAX_FILENAME_LENGTH - 1) {
		printf("directory path is too large\n");
		return -1;
	}

	strcpy(file_serve_directory, argv[2]);
	printf("data directory is: %s\n", file_serve_directory);

	// the server only takes on argument, the port
	port = atoi(argv[1]);

	clientconninfo_t clientconn_infos[max_conns];

	for(uint8_t i = 0; i < 10; i++) {
		clientconninfo_t new_clientconn_info;
		reset_clientconn_info(&new_clientconn_info);
		clientconn_infos[i] = new_clientconn_info;
	}

	// create and bind a udp socket
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
		int no_free_connections = 0;
		int opcode_recv = -1;
		clientconninfo_t* current_conn;
		char buffer[PACKET_SIZE];
		socklen_t client_len = (socklen_t) sizeof(client);
		rqpacket_t req_header;

		int free_conn_number = -1;
		int conn_number = -1;

		clear_complete_clientconn_infos(clientconn_infos, max_conns);

		memset(&buffer, 0, sizeof(buffer));

		num_bytes_received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*) &client, &client_len);
		if(num_bytes_received == -1) {
			perror("recvfrom error");
			continue;
		}

		opcode_recv = read_opcode(buffer);
		printf("opcode_rev: %d\n", opcode_recv);

		switch(opcode_recv) {

			case OPCODE_READ:

				free_conn_number = find_free_clientconn_info(clientconn_infos, max_conns);

				if(free_conn_number < 0) {
					// could not find any free connections to use
					// error handling
					printf("connection discarded: no free connection\n");
					no_free_connections = 1;
					break;
				}

				conn_number = free_conn_number;
				current_conn = &clientconn_infos[free_conn_number];
				intialize_clientconn_info(current_conn, file_serve_directory, buffer, &client);
				//send_data_packet_to_client(int sockfd, clientconninfo_t* clientconn_info, struct sockaddr* client, int client_len)
				send_data_packet_to_client(sockfd, current_conn, (struct sockaddr*) &client, client_len);

				break;

			case OPCODE_ACK:

				conn_number = find_clientconn_info(clientconn_infos, max_conns, client.sin_port);

				if(conn_number < 0) {
					printf("received ack for unregistered connection\n");
					continue;
				}

				current_conn = &clientconn_infos[conn_number];

				ackpacket_t ack_packet;
				read_ack_packet(&ack_packet, buffer);

				printf("ack #%d on port %d\n", ack_packet.block_number, client.sin_port);

				if(current_conn->block_number != ack_packet.block_number + 1) {
					current_conn->block_number--;
				}

				if(current_conn->complete == 0) {
					send_data_packet_to_client(sockfd, current_conn, (struct sockaddr*) &client, client_len);
				}

				break;

		}

		continue;

		//printf("received %d bytes\n", num_bytes);

		if(num_bytes_received > 0) {
			buffer[PACKET_SIZE-1] = '\0';
			//printf("b: %s\n", buffer);

			read_request_packet(&req_header, buffer);

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
					num_bytes_read = fread(file_buffer, (size_t) sizeof(unsigned char), 512, rq_file);

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

				int num_bytes_sent = sendto(sockfd, send_buffer, 4 + num_bytes_read, 0, (struct sockaddr*) &client, client_len);

				while(1) {

					memset(&buffer, 0, sizeof(buffer));
					num_bytes_received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*) &client, &client_len);

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
							read_ack_packet(&ack_packet, buffer);

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
