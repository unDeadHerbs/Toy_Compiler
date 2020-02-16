#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "macro_expander.h"

int main(int argc, char** argv) {
	struct token* tok;
	if (argc != 2) {
		printf("Usage : %s [file]\n", argv[0]);
		return argc == 1;
	}
	start_tokenizer(argv[1]);
	tok = gettok();
	while (tok->type != EOF_TOKEN) {
		printf("Token at %s:%d\t%s\n", tok->path, tok->line, tok->token);
		free(tok);
		tok = gettok();
	}
}

/*
#include "parser.h"

void print_tree(struct node* root) {
  if (!root) return;
  static int depth = 0;
  int i;
  for (i = 0; i < depth; i++) printf("\t");
  printf("%s\n", root->token);
  depth++;
  print_tree(root->child);
!  depth--;
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
*/
