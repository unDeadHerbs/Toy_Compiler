#include "tokenizer.h"
#include <errno.h>
#include <unistd.h>

ssize_t gettok(int fd, char buf[33]) {
	char* cur = buf;
	do {
		*cur = '\0';
		if (-1 == read(fd, cur, 1)) return -1;
		if (*cur == '\0') break;
		if (*cur == ' ' || *cur == '\n' || *cur == '\t') {
			if (cur == buf) continue;
			break;
		}
		if (cur++ - buf > 32) break;
	} while (1);
	if (cur - buf == 34) {
		errno = ENOBUFS;
		return -1;
	}
	*cur = '\0';
	return cur - buf;
}
