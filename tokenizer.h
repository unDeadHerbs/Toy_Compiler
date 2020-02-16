#ifndef TOKENIZER_H__
#define TOKENIZER_H__

#include <sys/types.h>

enum token_type_raw {
	ERROR_raw,
	EOF_TOKEN_raw,
	NEWLINE_raw,
	IDENT_raw,
	NUMBER_raw,
	RELATION_raw,
	ARITH_UNARY_raw,
	OP_OTHER_raw,
	STRING_raw,
	CHAR_raw,
	PREPROC_raw,
	COMMENT_raw
};

ssize_t gettok_raw(int fd, char buf[33], enum token_type_raw* type);

#endif
