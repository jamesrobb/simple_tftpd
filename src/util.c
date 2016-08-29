#include "util.h"

int find_string_terminator(char* buffer, int start) {
	int end_index = 0;

	for(int i = start; i < sizeof(buffer); i++) {

		if(buffer[i] == 0x0) {
			return i;
		}

	}

	return 0;
}

void print_usage() {

	printf("usage: tftpd [-p port_number]\n");

	return;
}
