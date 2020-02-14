#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"

void print_tree(struct node* root) {
	if (!root) return;
	static int depth = 0;
	int i;
	for (i = 0; i < depth; i++) printf("\t");
	printf("%s\n", root->token);
	depth++;
	print_tree(root->child);
	depth--;
	print_tree(root->sibling);
}

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
	struct node* tree;
	do {
		tree = parse_next(fd);
		print_tree(tree);
		free_node(tree);
	} while (tree);
}
