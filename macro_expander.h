#ifndef MACRO_EXPANDER_H__
#define MACRO_EXPANDER_H__

#include <sys/types.h>

enum token_type {
	EOF_TOKEN,
	IDENT,
	NUMBER,
	RELATION,
	ARITH_UNARY,
	OP_OTHER,
	STRING,
	CHAR,
	PREPROC
};

struct token {
	enum token_type type;
	char token[33];
	char const* path;
	int line;
};

void start_tokenizer(char const* const path);
struct token* gettok(void);

#endif
