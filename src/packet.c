#include <string.h>
#include "packet.h"
#include "util.h"

void read_request_packet(char buffer[], rqpacket_t* req_packet) {

	req_packet->opcode = buffer[1];

	int file_end = find_string_terminator(buffer, PACKET_SIZE, 2);
    //printf("file_end: %d\n", file_end);
	strncpy(req_packet->filename, buffer + 2, file_end - 1);

    int mode_end = find_string_terminator(buffer, PACKET_SIZE, file_end + 1);
    int mode_start = file_end + 1;
    //printf("mode_end: %d\n", mode_end);
    strncpy(req_packet->mode, buffer + mode_start, mode_end - mode_start + 1);

	return;
}

void read_ack_packet(char buffer[], ackpacket_t* ack_packet) {

	ack_packet->opcode = buffer[1];
    ack_packet->block_number = 0;
    ack_packet->block_number |= ((unsigned char) buffer[2] << 8);
    ack_packet->block_number |= (unsigned char) buffer[3];

	return;
}

int read_opcode(char buffer[]) {

    return buffer[1];
}
