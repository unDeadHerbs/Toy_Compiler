#include "macro_expander.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

struct token_dict {
	struct token* tok;
	struct token_dict* next;
};

static struct def_dict {
	struct token* name;
	char has_params;
	struct token_dict* params;
	struct token_dict* value;
	struct def_dict* next;
	struct def_dict* prior;
} top_def_dict, *current_def_dict = &top_def_dict;

#define add_def_to_dict_bot(PTR_TOK)                                       \
	do {                                                                     \
		struct token* force_eval = PTR_TOK;                                    \
		if (NULL != current_def_dict->name) {                                  \
			if (NULL ==                                                          \
			    (current_def_dict->next = calloc(sizeof(struct def_dict), 1))) { \
				perror("calloc");                                                  \
				exit(-1);                                                          \
			}                                                                    \
			current_def_dict->next->prior = current_def_dict;                    \
			current_def_dict = current_def_dict->next;                           \
		}                                                                      \
		current_def_dict->name = force_eval;                                   \
	} while (0)
#define add_token_to_last_def(PTR_TOK)                           \
	do {                                                           \
		struct token_dict* last = current_def_dict->value;           \
		while (last != NULL) last = last->next;                      \
		if (NULL == (last = calloc(sizeof(struct token_dict), 1))) { \
			perror("calloc");                                          \
			exit(-1);                                                  \
		}                                                            \
		last->tok = PTR_TOK;                                         \
	} while (0)
#define add_token_to_last_def_params(PTR_TOK)                    \
	do {                                                           \
		struct token_dict* last = current_def_dict->params;          \
		while (last != NULL) last = last->next;                      \
		if (NULL == (last = calloc(sizeof(struct token_dict), 1))) { \
			perror("calloc");                                          \
			exit(-1);                                                  \
		}                                                            \
		last->tok = PTR_TOK;                                         \
	} while (0)

#define add_simple_def_STR(NAME, VALUE)                            \
	do {                                                             \
		struct token *macro_name, *macro_value;                        \
		if (NULL == (macro_name = calloc(sizeof(struct token), 1))) {  \
			perror("calloc");                                            \
			exit(-1);                                                    \
		}                                                              \
		if (NULL == (macro_value = calloc(sizeof(struct token), 1))) { \
			perror("calloc");                                            \
			exit(-1);                                                    \
		}                                                              \
		strcpy(macro_name->token, NAME);                               \
		strcat(macro_value->token, VALUE);                             \
		macro_name->line = macro_value->line = current_file->line;     \
		macro_name->path = macro_value->path = current_file->path;     \
		macro_name->type = IDENT;                                      \
		macro_value->type = STRING;                                    \
		add_def_to_dict_bot(macro_name);                               \
		add_token_to_last_def(macro_value);                            \
	} while (0)

static struct file_position {
	char const* path;
	int fd;
	int line;
	struct file_position* child;
	struct file_position* parrent;
} top_file, *current_file = &top_file;
#define add_file_to_stack(PATH)                                                \
	do {                                                                         \
		char* path_buf;                                                            \
		if (NULL == (path_buf = calloc(sizeof(char), (size_t)strlen(PATH) + 1))) { \
			perror("calloc");                                                        \
			exit(-1);                                                                \
		}                                                                          \
		strcpy(path_buf, PATH);                                                    \
		if (current_file->path != NULL) {                                          \
			if (NULL ==                                                              \
			    (current_file->child = calloc(sizeof(struct file_position), 1))) {   \
				perror("calloc");                                                      \
				exit(-1);                                                              \
			}                                                                        \
			current_file->child->parrent = current_file;                             \
			current_file = current_file->child;                                      \
		}                                                                          \
		current_file->path = path_buf;                                             \
		if (-1 == (current_file->fd = open(path, 0))) {                            \
			perror("open");                                                          \
			exit(-1);                                                                \
		}                                                                          \
		add_simple_def_STR("__FILE__", path_buf);                                  \
	} while (0)

/* NOTE: Intentional leak of ~char* path~ so it stays in memory for
   tokens. */
#define pop_file_stack()                  \
	do {                                    \
		current_file = current_file->parrent; \
		free(current_file->child);            \
		current_file->child = NULL;           \
	} while (0)

void start_tokenizer(char const* const path) { add_file_to_stack(path); }

struct token* gettok(void) {
	static int reading_define = 0;
	struct token* ret;
	static struct token* storage = NULL;
	/* Storage is only non-NULL if the last token was a function like
	   macro and it didn't have a parameter list. */
	enum token_type_raw type;
	if (NULL == storage) {
		if (NULL == (ret = calloc(sizeof(struct token), 1))) {
			perror("calloc");
			exit(-1);
		}
		gettok_raw(current_file->fd, ret->token, &type);
		ret->path = current_file->path;
		ret->line = current_file->line;
	} else {
		printf("found storage\n");
		ret = storage;
		storage = NULL;
		return ret;
	}
	if (current_file == NULL) return ret;
	switch (type) {
		case ERROR_raw:
			perror("gettok_raw");
			exit(-1);

		case EOF_TOKEN_raw:
			if (current_file != &top_file) {
				pop_file_stack();
				free(ret);
				return gettok();
			} else {
				ret->type = EOF_TOKEN;
				return ret;
			}
		case IDENT_raw: {
			struct def_dict* def_check = current_def_dict;
			ret->type = IDENT;
			while (NULL != def_check) {
				if (0 == strcmp(def_check->name->token, ret->token)) break;
				def_check = def_check->prior;
			}
			if (NULL != def_check) {
				if (def_check->has_params) {
					storage = gettok();
					if (0 != strcmp("(", storage->token)) break;
					free(storage);
					storage = NULL;
					/* Read Parameters into defs */
				}
				/* Put onto Macro Stack */
				free(ret);
				return gettok();
			} else {
				break;
			}
		}
		case NUMBER_raw:
			ret->type = NUMBER;
			break;
		case RELATION_raw:
			ret->type = RELATION;
			break;
		case ARITH_UNARY_raw:
			ret->type = ARITH_UNARY;
			break;
		case OP_OTHER_raw:
			ret->type = OP_OTHER;
			break;
		case STRING_raw:
			ret->type = STRING;
			break;
		case CHAR_raw:
			ret->type = CHAR;
			break;

		case PREPROC_raw:
			/* check the type */
			if (0 == strncmp("#include ", ret->token, 9)) {
				/* check if next char is " or < */
				char* f = ret->token;
				while (*++f != ' ')
					;
				while (*++f == ' ')
					;
				if (*f == '"') {
					char* e = f;
					while (*++e != '"')
						;
					*e = '\0';
					start_tokenizer(f + 1);
					return gettok();
				}
			}
			if (0 == strcmp("#define", ret->token)) {
				struct token* tmp;
				if (reading_define) {
					errno = EINVAL;
					perror("macro_expander");
					exit(-1);
				}
				add_def_to_dict_bot(gettok()); /* first is name */
				reading_define = 1;
				tmp = gettok();
				if (0 == strcmp("(", tmp->token)) {
					current_def_dict->has_params = 1;
					free(tmp);
					tmp = gettok();
					while (1) {
						if (tmp->type != IDENT) {
							errno = EINVAL;
							fprintf(stderr, "Expected IDENT, got \"%s\"", tmp->token);
							perror("macro_expander");
							exit(-1);
						}
						add_token_to_last_def_params(tmp);
						tmp = gettok();
						if (0 == strcmp(",", tmp->token)) {
							free(tmp);
							tmp = gettok();
						} else if (0 == strcmp(")", tmp->token)) {
							break;
						} else {
							errno = EINVAL;
							perror("macro_expander");
							exit(-1);
						}
					}
				}
				while (reading_define) {
					add_token_to_last_def(tmp);
					tmp = gettok();
				}
				return gettok();
			}
			if (0 == strcmp("#undef", ret->token)) {
				struct token* to_rm;
				if (NULL == (to_rm = calloc(sizeof(struct token), 1))) {
					perror("calloc");
					exit(-1);
				}
				gettok_raw(current_file->fd, to_rm->token, &type);
				/* Can't use gettok as it evaluates macros*/
				/* TODO: Remove it from the dict. */
				free(to_rm);
				return gettok();
			}

			ret->type = PREPROC;
			break;

		case NEWLINE_raw:
			current_file->line++;
			if (reading_define) {
				ret->type = EOF_TOKEN;
				reading_define = 0;
				break;
			}

		case COMMENT_raw:
			free(ret);
			return gettok();
	}
	return ret;
}
