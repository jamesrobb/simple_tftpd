#ifndef _packet_h
#define _packet_h

#include <stdint.h>
#include <ctype.h>

#define PACKET_SIZE 1024
#define DATA_SIZE	512
#define DATA_HEADER 8
#define OPCODE_READ 1
#define OPCODE_WRITE 2
#define OPCODE_DATA 3
#define OPCODE_ACK  4

#define TFTP_ERROR_OTHER 0
#define TFTP_ERROR_FILE_NOT_FOUND 1
#define TFTP_ERROR_FILE_ACCESS_VILOATION 2
#define TFTP_ERROR_ACCESS_ILLEGAL_OP 4

#define MAX_FILENAME_LENGTH 512

typedef struct _rq_packet_ {
	int opcode;
	char mode[10];
	char filename[MAX_FILENAME_LENGTH];
} rqpacket_t;

typedef struct _ack_packet_ {
	int opcode;
	uint16_t block_number;
} ackpacket_t;

void read_request_packet(rqpacket_t* req_packet, char buffer[]);

void read_ack_packet(ackpacket_t* ackpacket, char buffer[]);

int read_opcode(char buffer[]);

#endif
