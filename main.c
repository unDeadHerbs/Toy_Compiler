#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tokenizer.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Usage : %s [file]\n", argv[0]);
		return argc == 1;
	}
	printf("Opening %s\n", argv[1]);
	int fd;
	if (-1 == (fd = open(argv[1], 0))) {
		perror("open");
		exit(-1);
	}
	char buf[33];
	do {
		if (-1 == gettok(fd, buf)) {
			perror("gettok");
			exit(-1);
		}
		printf("Read: %s\n", buf);
	} while (*buf);
}
