#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

#if 0
#define DBG(MSG) printf("%s:%d MGS=%s\n", __FILE__, __LINE__, MSG)
#else
#define DBG(MSG) \
	do {           \
	} while (0)
#endif

void free_node(struct node* n) {
	if (n == NULL) return;
	free_node(n->sibling);
	free_node(n->child);
	free(n);
}

void append_child(struct node* parent, struct node* child) {
	struct node* pred_child;
	if (parent->child == NULL) {
		parent->child = child;
		return;
	}
	pred_child = parent->child;
	while (pred_child->sibling != NULL) pred_child = pred_child->sibling;
	pred_child->sibling = child;
}

ssize_t safetok(int fd, char buf[33], enum token_type* type) {
	ssize_t count;
	buf[0] = '\0';
	if (-1 == (count = gettok(fd, buf, type))) {
		if (buf[0] != '\0') {
			fprintf(stderr, "gettok \"%s\"", buf);
			perror("");
		} else
			perror("gettok");
		exit(-1);
	}
	return count;
}

/* -^- -_-_- -^- Divider -^- -_-_- -^- */

static struct node* current_token = NULL;
static enum token_type current_type;

#define cur_tok() (current_token->token)

#define cur_type() (current_type)

#define use_tok_as(TO_NODE)                                         \
	do {                                                              \
		TO_NODE = current_token;                                        \
		if (NULL == (current_token = calloc(sizeof(struct node), 1))) { \
			perror("calloc");                                             \
			exit(-1);                                                     \
		}                                                               \
		safetok(fd, current_token->token, &current_type);               \
	} while (0)

#define make_node(NAME, DESCRIPTION)                       \
	do {                                                     \
		if (NULL == (NAME = calloc(sizeof(struct node), 1))) { \
			perror("calloc");                                    \
			exit(-1);                                            \
		}                                                      \
		strcpy(NAME->token, DESCRIPTION);                      \
	} while (0)

#define parse_error(DESCRIPTION)                                            \
	do {                                                                      \
		fprintf(stderr, "parse_type: %s got \"%s\"\n", DESCRIPTION, cur_tok()); \
		exit(-1);                                                               \
	} while (0)

#define append_tok_to(TO_NODE)        \
	do {                                \
		struct node* temp_name;           \
		use_tok_as(temp_name);            \
		append_child(TO_NODE, temp_name); \
	} while (0)

#define nom_tok()           \
	do {                      \
		struct node* temp_name; \
		use_tok_as(temp_name);  \
	} while (0)

#define nom_expect(STR)                                                   \
	do {                                                                    \
		struct node* temp_name;                                               \
		if (0 != strcmp(STR, cur_tok())) parse_error("Expected \"" STR "\""); \
		use_tok_as(temp_name);                                                \
	} while (0)

/* -^- -_-_- -^- Divider -^- -_-_- -^- */

struct node* parse_type(int fd) {
	/* [static] (struct|enum) [name] [{things}] [*] */
	/* [static] (int|void|char) [*] */
	struct node* type_node;
	DBG("entered");
	make_node(type_node, "type");
	if (0 == strcmp("static", cur_tok())) {
		struct node* stat;
		use_tok_as(stat);
		append_child(type_node, stat);
	}
	if (cur_type() != IDENT) parse_error("expected type name");
	if (0 == strcmp("struct", cur_tok()) || 0 == strcmp("enum", cur_tok())) {
		struct node* typetype;
		int found = 0;
		use_tok_as(typetype);
		append_child(type_node, typetype);
		if (cur_type() == IDENT) {
			struct node* typename;
			use_tok_as(typename);
			append_child(type_node, typename);
			found++;
		}
		if (0 == strcmp("{", cur_tok())) {
			/* TODO content */
			found++;
		}
		while (0 == strcmp("*", cur_tok())) append_tok_to(type_node);
		if (0 == found) parse_error("expected a name or a description");
		return type_node;
	} else if (0 == strcmp("int", cur_tok()) || 0 == strcmp("void", cur_tok()) ||
	           0 == strcmp("char", cur_tok())) {
		struct node* typename;
		use_tok_as(typename);
		append_child(type_node, typename);
		while (0 == strcmp("*", cur_tok())) append_tok_to(type_node);
		return type_node;
	} else
		parse_error("unkown type");
}

struct node* parse_block(int fd) {
	/* ; or { statements } */
	struct node* first;
	enum token_type type;
	DBG("entered");
	if (NULL == (first = calloc(sizeof(struct node), 1))) {
		perror("calloc");
		exit(-1);
	}
	safetok(fd, first->token, &type);
	if (0 == strcmp(";", first->token)) return first;
	if (0 == strcmp("{", first->token)) {
	}
	fprintf(stderr, "parse_block: Expected either ';' or '{', got \"%s\"\n",
	        first->token);
	exit(-1);
}

struct node* parse_function(int fd) {
	/* (args) block */
	/* (args) ; */
	struct node* func;
	struct node* params;
	make_node(func, "Function");
	make_node(params, "Parameters");
	append_child(func, params);
	nom_expect("(");
	while (0 != strcmp(")", cur_tok())) {
		struct node* param;
		make_node(param, "Parameter");
		append_child(param, parse_type(fd));
		if (cur_type() == IDENT) append_tok_to(param);
		append_child(params, param);
		if (0 != strcmp(")", cur_tok()))
			nom_expect(","); /* This allows for a traling comma? */
	}
	nom_expect(")");
	if (0 == strcmp("{", cur_tok()))
		append_child(func, parse_block(fd));
	else
		nom_expect(";");
	return func;
}

struct node* parse_top_usage(int fd) {
	if (0 == strcmp(";", cur_tok())) {
		struct node* semi;
		use_tok_as(semi);
		return semi;
	}
	if (cur_type() != IDENT) parse_error("expected name");
	/* TODO check not reserved */
	{
		struct node* name;
		use_tok_as(name);
		if (0 == strcmp("=", cur_tok())) {
			/* TODO default value */
			return name;
		} else if (0 == strcmp(";", cur_tok())) {
			/* no initialization */
			return name;
		} else if (0 == strcmp("(", cur_tok())) {
			append_child(name, parse_function(fd));
			return name;
		}
		parse_error("Expected value, function, or end of statement");
	}
}

struct node* parse_top_statement(int fd) {
	struct node* root;
	make_node(root, "top_level_statement");
	append_child(root, parse_type(fd));
	append_child(root, parse_top_usage(fd));
	/*
	  type = (struct|enum) [name] [{ content }]
	       | void | int
	  full_type = [static] type
	  usage = name [= default] ;
	        | name func_del
	        | ;
	  top_block = full_type usage
	*/
	return root;
}

struct node* parse_root(int fd) {
	struct node* root;
read_token:
	switch (cur_type()) {
		case ERROR:
			DBG("at");
			parse_error("got tokenizer error");

		case COMMENT:
		case NEWLINE:
			nom_tok();
			goto read_token;

		case PREPROC:
			if (0 == strncmp("#include ", cur_tok(), 9)) {
				use_tok_as(root);
				return root;
			}
			if (0 == strcmp("#if", cur_tok()) || 0 == strcmp("#ifdef", cur_tok()) ||
			    0 == strcmp("#ifndef", cur_tok()) ||
			    0 == strcmp("#define", cur_tok())) {
				use_tok_as(root);
				while (cur_type() != NEWLINE) append_tok_to(root);
				return root;
			}
			/* TODO: the rest of them */
			use_tok_as(root);
			return root;

		case EOF_TOKEN:
			return NULL;

		case IDENT:
			return parse_top_statement(fd);

		case NUMBER:
		case RELATION:
		case ARITH_UNARY:
		case OP_OTHER:
		case STRING:
		case CHAR:
			parse_error("Bad Root");
	}
}

struct node* parse_next(int fd) {
	if (current_token == NULL) nom_tok();
	return parse_root(fd);
}
