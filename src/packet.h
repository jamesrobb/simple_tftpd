#ifndef _packet_h
#define _packet_h

#define PACKET_SIZE 1024
#define DATA_SIZE	512
#define DATA_HEADER 8

typedef struct _rq_packet_ {
	int opcode;
	char mode[10];
	char filename[];
} rqpacket_t;

void read_header_packet(char buffer[], rqpacket_t* req_packet);

#endif
