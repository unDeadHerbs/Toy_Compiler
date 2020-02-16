#ifndef PARSER_H__
#define PARSER_H__
#include "macro_expander.h"

struct node {
	struct token* tok;
	struct node* sibling;
	struct node* child;
};

void free_node(struct node*);

void start_parser(char const* const path);
struct node* parse_next(void);

#endif
