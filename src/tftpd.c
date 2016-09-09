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

int main(int argc, char *argv[]) {

	uint8_t max_conns = 5;

	if(argc != 3 || argc > 3) {
		print_usage();
		exit(1);
	}

	// variable declerations
	int sockfd;
	int port;
	struct sockaddr_in server, client;

	// the server only takes on argument, the port
	port = atoi(argv[1]);
	printf("port %d\n", port);

	// setting up the directory where our files will be served from
	memset(&file_serve_directory, 0, MAX_FILENAME_LENGTH);
	int passed_serve_directory_size = strlen(argv[2]);

	if(passed_serve_directory_size > MAX_FILENAME_LENGTH - 1) {
		printf("directory path is too large\n");
		return -1;
	}

	strcpy(file_serve_directory, argv[2]);
	// we make sure that there is a trailing slash on the file serve directory
	if(file_serve_directory[strlen(file_serve_directory)-1] != '/') {
		strcat(file_serve_directory, "/");
	}
	printf("data directory is: %s\n", file_serve_directory);

	clientconninfo_t clientconn_infos[max_conns];

	for(uint8_t i = 0; i < max_conns; i++) {
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

	if(bind(sockfd, (struct sockaddr*) &server, (socklen_t) sizeof(server)) < 0) {
		perror("unable to bind socket");
		exit(1);
	}

	// this is our listen loop, we grab a udp packet one at a time, figure out if there is a client connection active.
	// if there is no active connection we create one, otherwise we find the connection based on the source port and then
	// start/continue serving the file to the client
	while(1) {

		clientconninfo_t* current_conn;
		//clientconninfo_t conninfo_error;
		socklen_t client_len = (socklen_t) sizeof(client);
		//rqpacket_t req_header;
		int free_conn_number = -1;
		int conn_number = -1;
		int num_bytes_received;
		int no_free_connections = 0;
		int opcode_recv = -1;
		int initialization_result = -1;
		char buffer[PACKET_SIZE];

		// we have this conninfo_error as just a place holder for error situations in where we don't
		// have any real info, or dont care about the info, of a bad request/situation
		//reset_clientconn_info(&conninfo_error);

		memset(&buffer, 0, sizeof(buffer));

		num_bytes_received = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*) &client, &client_len);
		if(num_bytes_received == -1) {
			perror("recvfrom error");
			continue;
		}

		// any transfers that have finished or timed out get cleaned up
		clear_complete_clientconn_infos(clientconn_infos, max_conns);

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
				initialization_result = intialize_clientconn_info(current_conn, file_serve_directory, buffer, &client);

				if(initialization_result == FILE_NOT_FOUND){
					current_conn->complete = 1;
					send_error_packet_to_client(sockfd, TFTP_ERROR_FILE_NOT_FOUND, "FILE NOT FOUND", (struct sockaddr*) &client, client_len);
					break;
				}

				if(initialization_result == FILE_ACCESS_VIOLATION){
					current_conn->complete = 1;
					send_error_packet_to_client(sockfd, TFTP_ERROR_FILE_ACCESS_VILOATION, "FILE ACCESS VIOLATION", (struct sockaddr*) &client, client_len);
					break;
				}

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

				//printf("ack #%d on port %d\n", ack_packet.block_number, client.sin_port);

				if(current_conn->block_number != ack_packet.block_number + 1) {
					//current_conn->block_number--;
					current_conn->retry_last_data = 1;
				} else {

					if(current_conn->sent_last_packet == 1) {
						current_conn->complete = 1;
						printf("final ack on port %d\n", client.sin_port);
					}

				}

				if(current_conn->complete == 0) {
					send_data_packet_to_client(sockfd, current_conn, (struct sockaddr*) &client, client_len);
				}

				break;

			case OPCODE_WRITE:

				send_error_packet_to_client(sockfd, TFTP_ERROR_ACCESS_ILLEGAL_OP, "WRITE NOT PERMITTED", (struct sockaddr*) &client, client_len);
				break;

			default:

				send_error_packet_to_client(sockfd, TFTP_ERROR_ACCESS_ILLEGAL_OP, "INVALID OPERATION", (struct sockaddr*) &client, client_len);
				break;

		}

		if(no_free_connections) {
			send_error_packet_to_client(sockfd, TFTP_ERROR_OTHER, "NO FREE CONNECTIONS", (struct sockaddr*) &client, client_len);
		}

	}

	close(sockfd);
	return 0;
}
