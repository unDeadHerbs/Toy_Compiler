#ifndef TOKENIZER_H__
#define TOKENIZER_H__

#include <sys/types.h>

enum token_type {
	ERROR,
	EOF_TOKEN,
	NEWLINE,
	IDENT,
	NUMBER,
	RELATION,
	ARITH_UNARY,
	OP_OTHER,
	STRING,
	CHAR,
	PREPROC,
	COMMENT
};

ssize_t gettok(int fd, char buf[33], enum token_type* type);

#endif
