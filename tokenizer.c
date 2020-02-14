#include "tokenizer.h"
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

char cur_char_holder = '\0';

ssize_t gettok(int fd, char buf[33]) {
	char* ptr = buf;
	*ptr = '\0';

#define error(ERNO) \
	do {              \
		errno = ERNO;   \
		return -1;      \
	} while (0)
#define cur_char() (cur_char_holder)
#define do_read()  \
	if (!cur_char()) \
		if (-1 == read(fd, &cur_char_holder, 1)) return -1
#define nom()               \
	do {                      \
		cur_char_holder = '\0'; \
		do_read();              \
	} while (0)
#define consume_char()              \
	do {                              \
		*ptr++ = cur_char();            \
		*ptr = '\0';                    \
		nom();                          \
		if (ptr - buf > 32) goto check; \
	} while (0)

	// prime
	do_read();

	// rm leading whitespace
	while (isspace(cur_char()) && cur_char() != '\n') nom();
	if (cur_char() == '\n') {
		consume_char();
		goto good;
	}

	// check that there is an input
	if (cur_char() == '\0') goto good;

	// if identifier or keyword
	if (isalpha(cur_char()) || cur_char() == '_') {
		do {
			consume_char();
			do_read();
		} while (isalnum(cur_char()) || cur_char() == '_');
		goto check;
	}

	// if number
	if (isdigit(cur_char())) {
		if (cur_char() == '0') {
			consume_char();
			if (cur_char() == 'x') {
				// hex
				while (isdigit(cur_char()) ||
				       ('a' <= cur_char() && cur_char() <= 'f') ||
				       ('A' <= cur_char() && cur_char() <= 'F'))
					consume_char();
			} else if (cur_char() == 'b') {
				// binary
				while (cur_char() == '0' || cur_char() == '1') consume_char();
			} else if (cur_char() != '0') {
				// octal
				while ('0' <= cur_char() && cur_char() <= '7') consume_char();
			} else
				error(EINVAL);
		} else {
			while (isdigit(cur_char())) consume_char();
			if (cur_char() == '.') {
				consume_char();
				while (isdigit(cur_char())) consume_char();
			}
			if (cur_char() == 'e' || cur_char() == 'E') {
				consume_char();
				if (cur_char() == '-') consume_char();
				if (isdigit(cur_char()))
					while (isdigit(cur_char())) consume_char();
				else
					error(EINVAL);
			}
		}
		goto good;
	}

	// if symbol
	if (ispunct(cur_char())) {
		// check for multi symbol tokens
		switch (cur_char()) {
			case '<':
			case '=':
			case '>':
			case '!':
				consume_char();
				if (cur_char() == '=') consume_char();
				break;
			case ':':
				consume_char();
				if (cur_char() == ':') consume_char();
				break;
			case '"':
				consume_char();
				while (cur_char() != '"') {
					if (cur_char() == '\\') consume_char();
					consume_char();
				}
				consume_char();
				break;
			case '\'':
				consume_char();
				if (cur_char() == '\\') {
					consume_char();
					// TODO: handel multibyte chars
					consume_char();
				} else {
					if (cur_char() == '\'') error(EINVAL);
					consume_char();
				}
				if (cur_char() != '\'') error(EINVAL);
				consume_char();
				break;
			case '#':
				consume_char();
				if (cur_char() == 'i') {
					consume_char();
					// clang-format off
					if (cur_char() == 'f'){
						consume_char();
						if (cur_char() == 'n') consume_char();
						if (cur_char() != 'd') error(EINVAL); else consume_char();
						if (cur_char() != 'e') error(EINVAL); else consume_char();
						if (cur_char() != 'f') error(EINVAL); else consume_char();
					}else{
						if (cur_char() != 'n') error(EINVAL); else consume_char();
						if (cur_char() != 'c') error(EINVAL); else consume_char();
						if (cur_char() != 'l') error(EINVAL); else consume_char();
						if (cur_char() != 'u') error(EINVAL); else consume_char();
						if (cur_char() != 'd') error(EINVAL); else consume_char();
						if (cur_char() != 'e') error(EINVAL); else consume_char();
						// clang-format on
						while (isspace(cur_char())) consume_char();
						if (cur_char() == '"') {
							consume_char();
							while (cur_char() != '"') consume_char();
							consume_char();
						} else if (cur_char() == '<') {
							consume_char();
							while (cur_char() != '>') consume_char();
							consume_char();
						} else
							error(EINVAL);
					}
				} else if (cur_char() == 'd') {
					consume_char();
					// clang-format off
					if (cur_char() != 'e') error(EINVAL); else consume_char();
					if (cur_char() != 'f') error(EINVAL); else consume_char();
					if (cur_char() != 'i') error(EINVAL); else consume_char();
					if (cur_char() != 'n') error(EINVAL); else consume_char();
					if (cur_char() != 'e') error(EINVAL); else consume_char();
					if (cur_char() != ' ') error(EINVAL); else consume_char();
					// clang-format on
					break;
				} else if (cur_char() == 'u') {
					consume_char();
					// clang-format off
					if (cur_char() != 'n') error(EINVAL); else consume_char();
					if (cur_char() != 'd') error(EINVAL); else consume_char();
					if (cur_char() != 'e') error(EINVAL); else consume_char();
					if (cur_char() != 'f') error(EINVAL); else consume_char();
					// clang-format on
				} else if (cur_char() == 'e') {
					consume_char();
					// clang-format off
					if (cur_char() == 'n') {
						consume_char();
					if (cur_char() != 'd') error(EINVAL); else consume_char();
					if (cur_char() != 'i') error(EINVAL); else consume_char();
					if (cur_char() != 'f') error(EINVAL); else consume_char();
				}else{
					if (cur_char() != 'l') error(EINVAL); else consume_char();
					if (cur_char() != 's') error(EINVAL); else consume_char();
					if (cur_char() != 'e') error(EINVAL); else consume_char();
					if (isspace(cur_char())) break;
					if (cur_char() != 'i') error(EINVAL); else consume_char();
					if (cur_char() != 'f') error(EINVAL); else consume_char();
					    }
					// clang-format on
				} else if (cur_char() == 'e') {
					consume_char();
					// clang-format off
					//clang-format on
				}					           else
					error(EINVAL);
				break;
			case '/':
				consume_char();
				if (cur_char() == '/') goto eat_line;
				break;
			eat_line:
				while (cur_char() != '\n' && cur_char() != '\r') {
					if (cur_char() == '\\') consume_char();
					consume_char();
				}
				break;
			default:
				consume_char();
				break;
		}
		goto good;
	}

// else it is a mystery
bad:
	error(EINVAL);

check:
	if (ptr - buf >= 32) {
		do_read();
		if (isspace(cur_char()) || cur_char() == '\0') goto good;
		error(ENOBUFS);
	}
good:
	*ptr = '\0';
	return ptr - buf;

#undef error
#undef cur_char
#undef do_read
#undef nom
#undef consume_char
}
