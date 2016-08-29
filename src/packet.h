#ifndef _packet_h
#define _packet_h

#include <stdint.h>

#define PACKET_SIZE 1024
#define DATA_SIZE	512
#define DATA_HEADER 8
#define OPCODE_READ 1
#define OPCODE_DATA 3
#define OPCODE_ACK  4

typedef struct _rq_packet_ {
	int opcode;
	char mode[10];
	char filename[];
} rqpacket_t;

typedef struct _ack_packet_ {
	int opcode;
	uint16_t block_number;
} ackpacket_t;

void read_request_packet(char buffer[], rqpacket_t* req_packet);

void read_ack_packet(char buffer[], ackpacket_t* ackpacket);

int read_opcode(char buffer[]);

#endif
