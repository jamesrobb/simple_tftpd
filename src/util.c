#include "util.h"

int find_string_terminator(char* buffer, int buffer_len, int start) {

	for(int i = start; i < buffer_len; i++) {

        //printf("buffer val: %d\n", buffer[i]);

		if(buffer[i] == 0x0) {
			return i;
		}

	}

	return 0;
}

void print_usage() {

	printf("usage: tftpd [port_number] [data_directory]\n");

	return;
}
