#ifndef PARSER_H__
#define PARSER_H__

struct node {
	char token[33];
	struct node* sibling;
	struct node* child;
};

void free_node(struct node*);

struct node* parse_next(int fd);

#endif
