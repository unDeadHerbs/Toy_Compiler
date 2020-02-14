#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

void free_node(struct node* n) {
	if (n == NULL) return;
	free_node(n->sibling);
	free_node(n->child);
	free(n);
}

void append_child(struct node* parent, struct node* child) {
	if (parent->child == NULL) {
		parent->child = child;
		return;
	}
	parent = parent->child;
	while (parent->sibling) parent = parent->sibling;
	parent->sibling = child;
}

ssize_t safetok(int fd, char buf[33], enum token_type* type) {
	ssize_t count;
	if (-1 == (count = gettok(fd, buf, type))) {
		perror("gettok");
		exit(-1);
	}
	return count;
}

struct node* parse_root(int fd) {
	struct node* root = malloc(sizeof(struct node));
	enum token_type type;
read_token:
	safetok(fd, root->token, &type);
	switch (type) {
		case ERROR:
			fprintf(stderr, "parse_root: got tokenizer error\n");
			exit(-1);
		case COMMENT:
		case NEWLINE:
			goto read_token;
		case PREPROC:
			if (0 == strncmp("#include ", root->token, 9)) return root;
			/* TODO: the rest of them */
			return root;

		case EOF_TOKEN:
		case IDENT:
		case NUMBER:
		case RELATION:
		case ARITH_UNARY:
		case OP_OTHER:
		case STRING:
		case CHAR:
			fprintf(stderr, "Parse_Root: Bad Root \"%s\"\n", root->token);
			exit(-1);
	}
}

struct node* parse_next(int fd) {
	return parse_root(fd);
}
