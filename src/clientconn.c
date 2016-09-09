 #include "clientconn.h"


void clear_complete_clientconn_infos(clientconninfo_t* clientconn_infos, uint8_t clientconn_infos_len) {

	for(int i = 0; i < clientconn_infos_len; i++) {

		if(clientconn_infos[i].complete == 1) {
			reset_clientconn_info(&clientconn_infos[i]);
		}

	}

	return;
}

int find_clientconn_info(clientconninfo_t* clientconn_infos, uint8_t clientconn_infos_len, unsigned short client_port_number) {

	for(uint8_t i = 0; i < clientconn_infos_len; i++) {

		if(clientconn_infos[i].port_number == client_port_number) {
			return i;
		}

	}

	return -1;
}

int find_free_clientconn_info(clientconninfo_t* clientconn_infos, uint8_t clientconn_infos_len) {

	for(uint8_t i = 0; i < clientconn_infos_len; i++) {

		if(clientconn_infos[i].available == 1) {
			return i;
		}

	}

	return -1;
}

int intialize_clientconn_info(clientconninfo_t* clientconn_info, char file_directory[], char buffer[], struct sockaddr_in* client) {

	rqpacket_t req_packet;
	read_request_packet(&req_packet, buffer);

    if(strstr(req_packet.filename, "..")) {
        return FILE_ACCESS_VIOLATION;
    }

	//printf("reqpacketfile: %d\n", req_packet.filename[4]);

	char file_location[MAX_FILENAME_LENGTH];
	memset(&file_location, 0, 19);

	strncpy(file_location, file_directory, strlen(file_directory));
	strcat(file_location, req_packet.filename);

	strcpy(clientconn_info->mode, req_packet.mode);

	// if(strcmp(clientconn_info->mode, "netascii")) {
	// 	clientconn_info->fp = fopen(file_location, "r");
	// } else {
	// 	clientconn_info->fp = fopen(file_location, "rb");
	// }
    // the above code is no longer needed, we also transfer in binary
    clientconn_info->fp = fopen(file_location, "rb");

	clientconn_info->port_number = client->sin_port;

	printf("new connection (file, mode, port): %s, %s, %d\n", file_location, clientconn_info->mode, clientconn_info->port_number);

	if(clientconn_info->fp == NULL) {
		// could not open file
		perror("connection failed (file not found)");

		return FILE_NOT_FOUND;
	}

	clientconn_info->available = 0;

	return 0;
}

int send_data_packet_to_client(int sockfd, clientconninfo_t* clientconn_info, struct sockaddr* client, int client_len) {


	char send_buffer[DATA_SIZE + 4];
	int num_bytes_read = -1;

	memset(&send_buffer, 0, sizeof(send_buffer));

	if(!clientconn_info->retry_last_data) {

		char file_buffer[DATA_SIZE];

		memset(&file_buffer, 0, sizeof(file_buffer));

        // we always want to send in binary as netascii is dumb and noone used it anymore
		//if(strcmp(clientconn_info->mode, "octet") == 0) {
		num_bytes_read = fread(file_buffer, (size_t) sizeof(unsigned char), DATA_SIZE, clientconn_info->fp);
		clientconn_info->last_num_bytes_read = num_bytes_read;

		if(num_bytes_read < DATA_SIZE) {
			clientconn_info->sent_last_packet = 1;
		}

		send_buffer[1] = OPCODE_DATA;
		send_buffer[2] = (clientconn_info->block_number >> 8) & 0xFF;
		send_buffer[3] = clientconn_info->block_number & 0xFF;
		clientconn_info->last_data_packet[0] = 0;
		clientconn_info->last_data_packet[1] = send_buffer[1];
		clientconn_info->last_data_packet[2] = send_buffer[2];
		clientconn_info->last_data_packet[3] = send_buffer[3];

		for(int i = 4; i < num_bytes_read+4; i++) {
			//printf("f: %d\n", send_buffer[i]);
			send_buffer[i] = file_buffer[i-4];
			clientconn_info->last_data_packet[i] = file_buffer[i-4];
		}

	} else {

		num_bytes_read = clientconn_info->last_num_bytes_read;

		for(int i = 0; i < num_bytes_read; i++) {
			send_buffer[i] = clientconn_info->last_data_packet[i];
		}

	}

	printf("sending block numbers %d %d %d to port %d\n", clientconn_info->block_number, send_buffer[2], send_buffer[3], clientconn_info->port_number);

	int bytes_sent = sendto(sockfd, send_buffer, 4 + num_bytes_read, 0, client, client_len);

	printf("bytes_sent %d to %d\n", bytes_sent, clientconn_info->port_number);

	if(bytes_sent == -1) {
		clientconn_info->complete = 0;
		clientconn_info->retry_last_data = 1;
	} else {

        if(!clientconn_info->retry_last_data) {
            clientconn_info->block_number++;
        }
	}

	return bytes_sent;
}

void reset_clientconn_info(clientconninfo_t* clientconn_info) {

	strcpy(clientconn_info->mode, "");
	clientconn_info->block_number = 1;
	clientconn_info->port_number = 0;
	clientconn_info->available = 1;
    	clientconn_info->sent_last_packet = 0;
	clientconn_info->complete = 0;
	clientconn_info->retry_last_data = 0;
	clientconn_info->last_num_bytes_read = 0;

	return;
}

void send_error_packet_to_client(int sockfd, int error_code, const char * error_message, struct sockaddr* client, int client_len) {

    int error_message_len = strlen(error_message);
    char error_buffer[error_message_len + 5]; // length of string plus terminator plus tftp header info

    error_buffer[0] = 0;
    error_buffer[1] = 5;
    error_buffer[2] = 0;
    error_buffer[3] = error_code;
    // we copy the error message into the right place
    memcpy(error_buffer + 4, error_message, error_message_len + 1);

    //printf("error payload is: %s\n", error_buffer+1);

    sendto(sockfd, error_buffer, error_message_len + 5, 0, client, client_len);

    return;
}
