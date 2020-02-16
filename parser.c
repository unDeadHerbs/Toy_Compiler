#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macro_expander.h"

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
	free(n->tok);
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

/* -^- -_-_- -^- Divider -^- -_-_- -^- */

static struct token* current_token = NULL;

#define cur_tok() (current_token->token)
#define cur_type() (current_token->type)

#define use_tok_as(TO_NODE)                                   \
	do {                                                        \
		if (NULL == (TO_NODE = calloc(sizeof(struct node), 1))) { \
			perror("calloc");                                       \
			exit(-1);                                               \
		}                                                         \
		TO_NODE->tok = current_token;                             \
		current_token = gettok();                                 \
	} while (0)

#define make_node(NAME, DESCRIPTION)                             \
	do {                                                           \
		if (NULL == (NAME = calloc(sizeof(struct node), 1))) {       \
			perror("calloc");                                          \
			exit(-1);                                                  \
		}                                                            \
		if (NULL == (NAME->tok = calloc(sizeof(struct token), 1))) { \
			perror("calloc");                                          \
			exit(-1);                                                  \
		}                                                            \
		strcpy(NAME->tok->token, DESCRIPTION);                       \
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

struct node* parse_type() {
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

struct node* parse_expression() {
	/* (ident|op_other)* */
	/* TODO: this is just a stub so it builds. */
	struct node* expression;
	DBG("entered");
	make_node(expression, "EXPRESSION");
	while (cur_type() == IDENT ||
	       (cur_type() == OP_OTHER && 0 != strcmp(")", cur_tok())) ||
	       cur_type() == CHAR || cur_type() == STRING) {
		struct node* v;
		use_tok_as(v);
		append_child(expression, v);
	}
	return expression;
}

struct node* parse_condition() {
	/* expression [ relation expression ] */
	struct node* cond;
	DBG("entered");
	make_node(cond, "CONDITION");
	append_child(cond, parse_expression());
	if (cur_type() == RELATION) {
		struct node* rel;
		use_tok_as(rel);
		append_child(cond, rel);
		append_child(cond, parse_expression());
	}
	return cond;
}

struct node* parse_block(void);

struct node* parse_statement() {
	struct node* statement;
	DBG("entered");
	/* ; */
	if (0 == strcmp(";", cur_tok())) {
		use_tok_as(statement);
		return statement;
	}
	/* Controll */
	if (0 == strcmp("while", cur_tok())) {
		struct node* control;
		use_tok_as(statement);
		/* TODO: lookup names of keywords? */
		/* eat '(' cond ')' block */
		if (0 != strcmp("(", cur_tok()))
			parse_error("Expected condition after while");
		use_tok_as(control);
		append_child(control, parse_condition());
		nom_expect(")");
		append_child(statement, control);
		append_child(statement, parse_block());
		return statement;
	}
	/* Declaration */
	/* check if in type table */

	/* Function call or Asignment */
	/* check if known ident */
	use_tok_as(statement);
	return statement;
	/* return NULL; */
}

struct node* parse_block() {
	/* statement or { statement } */
	struct node* block;
	DBG("entered");
	if (0 == strcmp("{", cur_tok())) {
		use_tok_as(block);
		while (0 != strcmp("}", cur_tok())) append_child(block, parse_statement());
		nom_expect("}");
		return block;
	} else {
		return parse_statement();
	}
}

struct node* parse_function() {
	/* (args) block */
	/* (args) ; */
	struct node* func;
	struct node* params;
	DBG("entered");
	make_node(func, "Function");
	make_node(params, "Parameters");
	append_child(func, params);
	nom_expect("(");
	while (0 != strcmp(")", cur_tok())) {
		struct node* param;
		make_node(param, "Parameter");
		append_child(param, parse_type());
		if (cur_type() == IDENT) append_tok_to(param);
		append_child(params, param);
		if (0 != strcmp(")", cur_tok()))
			nom_expect(","); /* This allows for a traling comma? */
	}
	nom_expect(")");
	if (0 == strcmp("{", cur_tok()))
		append_child(func, parse_block());
	else
		nom_expect(";");
	return func;
}

struct node* parse_top_usage(void) {
	DBG("entered");
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
			append_child(name, parse_function());
			return name;
		}
		parse_error("Expected value, function, or end of statement");
	}
}

struct node* parse_top_statement(void) {
	struct node* root;
	DBG("entered");
	make_node(root, "top_level_statement");
	append_child(root, parse_type());
	append_child(root, parse_top_usage());
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

struct node* parse_root(void) {
	DBG("entered");
	switch (cur_type()) {
		case EOF_TOKEN:
			return NULL;

		case IDENT:
			return parse_top_statement();

		case PREPROC:
		case NUMBER:
		case RELATION:
		case ARITH_UNARY:
		case OP_OTHER:
		case STRING:
		case CHAR:
			parse_error("Bad Root");
	}
}

void start_parser(char const* const path) { start_tokenizer(path); }

struct node* parse_next(void) {
	DBG("entered");
	if (current_token == NULL) nom_tok();
	return parse_root();
}
