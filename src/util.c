#include "util.h"

int find_string_terminator(char* buffer, int buffer_len, int start) {
	int end_index = 0;

    //printf("buffer siize: %ld\n", sizeof(buffer));

	for(int i = start; i < buffer_len; i++) {

        //printf("buffer val: %d\n", buffer[i]);

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
