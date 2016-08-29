#include <string.h>
#include "packet.h"
#include "util.h"

void read_header_packet(char buffer[], rqpacket_t* req_packet) {

	req_packet->opcode = buffer[1];

	int file_end = find_string_terminator(buffer, 2);
	strncpy(req_packet->filename, buffer+2, file_end - 1);

	return;
}
