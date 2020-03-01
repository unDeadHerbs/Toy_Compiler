#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macro_expander.h"

#if 0
#define DBG(MSG) printf("%s:%d MGS=%s\n", __FILE__, __LINE__, MSG)
#define DBGi(MSG, \
             NUM) /*printf("%s:%d\t%s=%d\n", __FILE__, __LINE__, MSG, NUM)*/
#else
#define DBG(MSG)
#define DBGi(MSG, NUM)
#endif

#define WERROR 1

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

#define rename_use_tok_as(TO_NODE, DESCRIPTION)               \
	do {                                                        \
		if (NULL == (TO_NODE = calloc(sizeof(struct node), 1))) { \
			perror("calloc");                                       \
			exit(-1);                                               \
		}                                                         \
		TO_NODE->tok = current_token;                             \
		current_token = gettok();                                 \
		strcpy(TO_NODE->tok->token, DESCRIPTION);                 \
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

#define parse_error(DESCRIPTION)                                               \
	do {                                                                         \
		char buffer[33];                                                           \
		struct node* error;                                                        \
		sprintf(buffer, "parse: %s got \"%s\" on line %d", DESCRIPTION, cur_tok(), \
		        current_token->line);                                              \
		make_node(error, buffer);                                                  \
		fprintf(stderr, "%s\n", buffer);                                           \
		if (WERROR) exit(-1);                                                      \
                                                                               \
		return error;                                                              \
	} while (0)

#define append_tok_to(TO_NODE)                  \
	do {                                          \
		struct node* temp_name_append_to;           \
		use_tok_as(temp_name_append_to);            \
		append_child(TO_NODE, temp_name_append_to); \
	} while (0)

#define nom_tok()               \
	do {                          \
		struct node* temp_name_nom; \
		use_tok_as(temp_name_nom);  \
	} while (0)

#define nom_expect(STR)                                                    \
	do {                                                                     \
		struct node* temp_name_nom_expect;                                     \
		if (0 != strcmp(STR, cur_tok())) {                                     \
			char buffer[33];                                                     \
			sprintf(buffer, "parse: Expected \"%s\" got \"%s\" on line %d", STR, \
			        cur_tok(), current_token->line);                             \
			make_node(temp_name_nom_expect, buffer);                             \
			fprintf(stderr, "%s\n", buffer);                                     \
			if (WERROR) exit(-1);                                                \
			nom_tok();                                                           \
			return temp_name_nom_expect;                                         \
		}                                                                      \
		use_tok_as(temp_name_nom_expect);                                      \
	} while (0)

/* -^- -_-_- -^- Divider -^- -_-_- -^- */

struct node* parse_block(void);

struct node* parse_type() {
	/* [static] (struct|enum) [name] [{things}] [*] */
	/* [static] (int|void|char) [*] */
	struct node* type_node;
	DBG("type");
	make_node(type_node, "TYPE");
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
			append_child(type_node, parse_block());
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

enum operator_associativity { ltr, rtl, unary_l, unary_r };
static struct operator{
	enum operator_associativity const asoc;
	char const symbol[4];
	char const symbol2[4];
}
const operators_in_prescience[] = {
    {ltr, "?:", ""},     {ltr, "?", ":"},     {ltr, ",", ""},
    {rtl, "|=", ""},     {rtl, "^=", ""},     {rtl, "&=", ""},
    {rtl, ">>=", ""},    {rtl, "<<=", ""},    {rtl, "%=", ""},
    {rtl, "/=", ""},     {rtl, "*=", ""},     {rtl, "-=", ""},
    {rtl, "+=", ""},     {ltr, "=", ""},      {ltr, "||", ""},
    {ltr, "&&", ""},     {ltr, "|", ""},      {ltr, "^", ""},
    {ltr, "&", ""},      {ltr, "!=", ""},     {ltr, "==", ""},
    {ltr, ">=", ""},     {ltr, ">", ""},      {ltr, "<=", ""},
    {ltr, "<", ""},      {ltr, "<=>", ""},    {ltr, ">>", ""},
    {ltr, "<<", ""},     {ltr, "-", ""},      {ltr, "+", ""},
    {ltr, "%", ""},      {ltr, "/", ""},      {ltr, "*", ""},
    {unary_l, "&", ""},  {unary_l, "*", ""},  {unary_l, "~", ""},
    {unary_l, "!", ""},  {unary_l, "--", ""}, {unary_l, "++", ""},
    {unary_r, "[", "]"}, {unary_r, "(", ")"}, {unary_r, "--", ""},
    {unary_r, "++", ""}, {ltr, "::", ""},     {unary_l, "::", ""}};
/* Some C operators are excluded and some extras are included.  See
 * notes file for more details.
 */

struct node* parse_expression(void);

struct node* parse_ident() {
	struct node* name;
	DBG("ident");
	if (cur_type() == IDENT) {
		use_tok_as(name);
		return name;
	}
	parse_error("Expected: Parenthetical, IDENT, or NUMBER");
}

struct node* parse_value() {
	struct node* value;
	DBG("value");
	/* '(' expression ')' */
	if (0 == strcmp("(", cur_tok())) {
		DBG("Paren");
		use_tok_as(value);
		append_child(value, parse_expression());
		nom_expect(")");
		return value;
	}
	/* IDENT or literal (NUMBER, CHAR, or STRING) */
	if (cur_type() == IDENT || cur_type() == NUMBER || cur_type() == CHAR ||
	    cur_type() == STRING) {
		use_tok_as(value);
		return value;
	}
	parse_error("Expected: Parenthetical, IDENT, or NUMBER");
}

struct node* parse_unary(int);
struct node* parse_binary(int);
struct node* parse_op(int op) {
	if (op == sizeof(operators_in_prescience) / sizeof(struct operator))
		return parse_value();
	if (operators_in_prescience[op].asoc == ltr ||
	    operators_in_prescience[op].asoc == rtl)
		return parse_binary(op);
	/* if(operators_in_prescience[op].asoc==unary_l ||
	  operators_in_prescience[op].asoc==unary_r) */
	return parse_unary(op);
}

struct node* parse_binary(int op) {
	struct node* op_t = NULL;
	struct node* value;
	/*DBG("OP_Binary");*/
	DBGi("op", op);
	value = parse_op(op + 1);
	while (0 == strcmp(operators_in_prescience[op].symbol, cur_tok())) {
		if (NULL == op_t)
			use_tok_as(op_t);
		else
			nom_expect(operators_in_prescience[op].symbol);
		append_child(op_t, value);
		value = parse_op(op + 1);
		if (0 != strcmp("", operators_in_prescience[op].symbol2)) {
			append_child(op_t, value);
			nom_expect(operators_in_prescience[op].symbol2);
			if (0 == strcmp("?", operators_in_prescience[op].symbol)) {
				value = parse_op(op + 1);
				append_child(op_t, value);
			}
			return op_t;
		}
	}
	if (NULL == op_t) return value;
	append_child(op_t, value);
	return op_t;
}

struct node* parse_unary(int op) {
	struct node* op_t = NULL;
	struct node* value;
	/*DBG("OP_Unary");*/
	DBGi("op", op);
	if (operators_in_prescience[op].asoc == unary_l)
		if (0 == strcmp(operators_in_prescience[op].symbol, cur_tok()))
			use_tok_as(op_t);
	value = parse_op(op + 1);
	if (operators_in_prescience[op].asoc == unary_r)
		if (0 == strcmp(operators_in_prescience[op].symbol, cur_tok())) {
			use_tok_as(op_t);
			if (0 != strcmp("", operators_in_prescience[op].symbol2)) {
				DBG("LHS Grouping");
				append_child(op_t, value);
				append_child(op_t, parse_expression());
				nom_expect(operators_in_prescience[op].symbol2);
				return op_t;
			}
		}
	if (NULL == op_t) return value;
	append_child(op_t, value);
	return op_t;
}

struct node* parse_expression() {
	return parse_op(0);
}

struct node* parse_condition() {
	/* expression [ relation expression ] */
	struct node* cond;
	DBG("cond");
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

struct node* parse_statement() {
	struct node* statement;
	DBG("statement");
	/* ; */
	if (0 == strcmp(";", cur_tok())) {
		use_tok_as(statement);
		return statement;
	}
	/* Controll */
	if (0 == strcmp("{", cur_tok())) {
		return parse_block();
	}
	if (0 == strcmp("while", cur_tok())) {
		struct node* control;
		use_tok_as(statement);
		/* TODO: lookup names of keywords? */
		/* eat '(' cond ')' block */
		if (0 != strcmp("(", cur_tok()))
			parse_error("Expected condition after while");
		rename_use_tok_as(control, "CONTROL");
		append_child(control, parse_condition());
		nom_expect(")");
		append_child(statement, control);
		append_child(statement, parse_block());
		return statement;
	}
	/* Declaration */
	if (0 == strcmp("struct", cur_tok())) {
		struct node* decl;
		struct node* type = parse_type();
		struct node* ident = parse_ident();
		make_node(decl, "DECLARATION");
		append_child(decl, type);
		append_child(decl, ident);
		if (0 == strcmp("=", cur_tok())) {
			struct node* assignment;
			use_tok_as(assignment);
			append_child(assignment, parse_expression());
			append_child(decl, assignment);
		}
		return decl;
	}
	statement = parse_expression();
	nom_expect(";");
	return statement;
}

struct node* parse_block() {
	/* statement or { statement } */
	struct node* block;
	DBG("block");
	if (0 == strcmp("{", cur_tok())) {
		use_tok_as(block);
		while (0 != strcmp("}", cur_tok())) append_child(block, parse_statement());
		nom_expect("}");
		return block;
	} else {
		return parse_statement();
	}
}

struct node* parse_parameters() {
	/* (args) */
	struct node* params;
	DBG("params");
	make_node(params, "PARAMETERS");
	nom_expect("(");
	while (0 != strcmp(")", cur_tok())) {
		struct node* param;
		make_node(param, "PARAMETER");
		append_child(param, parse_type());
		if (cur_type() == IDENT) append_tok_to(param);
		append_child(params, param);
		if (0 != strcmp(")", cur_tok()))
			nom_expect(","); /* This allows for a traling comma? */
	}
	nom_expect(")");
	return params;
}

struct node* parse_top_usage(void) {
	DBG("top_usage");
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
			struct node* func;
			make_node(func, "FUNCTION");
			append_child(func, name);
			append_child(func, parse_parameters());
			append_child(func, parse_block());
			return func;
		}
		parse_error("Expected value, function, or end of statement");
	}
}

struct node* parse_top_statement(void) {
	struct node* root;
	DBG("statement");
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
	DBG("root");
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
	if (current_token == NULL) nom_tok();
	return parse_root();
}
