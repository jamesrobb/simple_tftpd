#ifndef _clientconn_h
#define _clientconn_h

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include "packet.h"

#define FILE_NOT_FOUND	1
#define FILE_ACCESS_VIOLATION 2

typedef struct _clientconn_ {
	FILE* fp;
	char mode[10];
	uint16_t block_number;
	unsigned short port_number;
	uint8_t available;
	uint8_t sent_last_packet;
	uint8_t complete;
	uint8_t retry_last_data;
	uint8_t last_num_bytes_read;
	char last_data_packet[DATA_SIZE + 4];
} clientconninfo_t;

// we multiply by 2 for no other reason than just because
char file_serve_directory[MAX_FILENAME_LENGTH*2];

void clear_complete_clientconn_infos(clientconninfo_t* clientconn_infos, uint8_t clientconn_infos_len);

int find_clientconn_info(clientconninfo_t* clientconn_infos, uint8_t clientconn_infos_len, unsigned short client_port_number);

int find_free_clientconn_info(clientconninfo_t* clientconn_infos, uint8_t clientconn_infos_len);

int intialize_clientconn_info(clientconninfo_t* clientconn_info, char file_directory[], char buffer[], struct sockaddr_in* client);

int send_data_packet_to_client(int sockfd, clientconninfo_t* clientconn_info, struct sockaddr* client, int client_len);

void reset_clientconn_info(clientconninfo_t* clientconn_info);

void send_error_packet_to_client(int sockfd, int error_code, const char* error_message, struct sockaddr* client, int client_len);

#endif
