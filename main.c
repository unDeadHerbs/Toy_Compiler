#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"

void print_tree(struct node* root) {
	static int depth = 0;
	int i;
	if (!root) return;
	for (i = 0; i < depth; i++) printf("\t");
	printf("%s\n", root->tok->token);
	depth++;
	print_tree(root->child);
	depth--;
	print_tree(root->sibling);
}

int main(int argc, char** argv) {
	struct node* tree;
	if (argc != 2) {
		printf("Usage : %s [file]\n", argv[0]);
		return argc == 1;
	}
	start_parser(argv[1]);
	do {
		tree = parse_next();
		print_tree(tree);
		free_node(tree);
	} while (tree);
}
