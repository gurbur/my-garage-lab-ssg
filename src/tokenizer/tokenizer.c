#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/list_head.h"
#include "../include/tokenizer.h"

static void create_and_add_token(TokenType type, char* value, struct list_head* head) {
	Token* new_token = (Token*)malloc(sizeof(Token));
	if (!new_token) {
		perror("Failed to allocate memory for token");
		return;
	}

	new_token->type = type;
	new_token->value = value ? strdup(value) : NULL;

	list_add_tail(&new_token->list, head);
}

static void append_to_buffer(char** buffer, int* index, int* buffer_capacity, char current_char) {
	if (*index >= *buffer_capacity - 1) {
		*buffer_capacity *= 2;
		char* new_buffer = realloc(*buffer, *buffer_capacity);
		if (!new_buffer) {
			perror("Failed to reallocate buffer");
			free(*buffer);
			exit(EXIT_FAILURE);
		}
		*buffer = new_buffer;
	}

	(*buffer)[(*index)++] = current_char;
}

static void flush_dynamic_buffer(char** buffer, int* index, int* buffer_capacity, struct list_head* head) {
	if (*buffer && *index > 0) {
		(*buffer)[*index] = '\0';
		create_and_add_token(TOKEN_TEXT, *buffer, head);
	}
	if (*buffer) {
		free(*buffer);
		*buffer = NULL;
	}
	*index = 0;
	*buffer_capacity = 0;
}

static int fpeek(FILE* stream) {
	int c = fgetc(stream);
	ungetc(c, stream);
	return c;
}

void tokenize_file(FILE *file, struct list_head *output) {
	char current_char;
	char* text_buffer = NULL;
	int text_buffer_idx = 0;
	int text_buffer_capacity = 0;

	while ((current_char = fgetc(file)) != EOF) {
		switch (current_char) {
			case '#': // TOKEN_HASH
			case '*': // TOKEN_ASTERISK
			case '-': // TOKEN_DASH
			case '\n':// TOKEN_NEWLINE
			case '\t':// TOKEN_TAB
			case '[': // TOKEN_LBRACKET
			case ']': // TOKEN_RBRACKET
			case '(': // TOKEN_LPAREN
			case ')': // TOKEN_RPAREN
			case '!': // TOKEN_EXCLAMATION
			case '>': // TOKEN_GREATER_THAN
			case '\\':// TOKEN_BACKSLASH
				flush_dynamic_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, output);

				if (current_char == '.') create_and_add_token(TOKEN_DOT, NULL, output);
				else if (current_char == '#') create_and_add_token(TOKEN_HASH, NULL, output);
				else if (current_char == '*') create_and_add_token(TOKEN_ASTERISK, NULL, output);
				else if (current_char == '-') create_and_add_token(TOKEN_DASH, NULL, output);
				else if (current_char == '\n') create_and_add_token(TOKEN_NEWLINE, NULL, output);
				else if (current_char == '\t') create_and_add_token(TOKEN_TAB, NULL, output);
				else if (current_char == '[') create_and_add_token(TOKEN_LBRACKET, NULL, output);
				else if (current_char == ']') create_and_add_token(TOKEN_RBRACKET, NULL, output);
				else if (current_char == '(') create_and_add_token(TOKEN_LPAREN, NULL, output);
				else if (current_char == ')') create_and_add_token(TOKEN_RPAREN, NULL, output);
				else if (current_char == '!') create_and_add_token(TOKEN_EXCLAMATION, NULL, output);
				else if (current_char == '>') create_and_add_token(TOKEN_GREATER_THAN, NULL, output);
				else if (current_char == '\\') create_and_add_token(TOKEN_BACKSLASH, NULL, output);

				break;

			case '.':
				flush_dynamic_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, output);
				create_and_add_token(TOKEN_DOT, NULL, output);
				break;

			case '`': // TOKEN_BACKTICK
				flush_dynamic_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, output);

				char next1 = fgetc(file);
				char next2 = fgetc(file);

				if (next1 == '`' && next2 == '`') {
					create_and_add_token(TOKEN_BACKTICK, NULL, output);
					create_and_add_token(TOKEN_BACKTICK, NULL, output);
					create_and_add_token(TOKEN_BACKTICK, NULL, output);

					while ((current_char = fgetc(file)) != EOF && current_char != '\n') {
						if (!text_buffer) {
							text_buffer_capacity = 32;
							text_buffer = malloc(text_buffer_capacity);
						}
						append_to_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, current_char);
					}
					flush_dynamic_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, output);
					create_and_add_token(TOKEN_NEWLINE, NULL, output);

					bool code_block_closed = false;
					while (!code_block_closed && (current_char = fgetc(file)) != EOF) {
						char lookahead1 = fgetc(file);
						if (lookahead1 == EOF) {
							break;
						}
						char lookahead2 = fgetc(file);
						if (lookahead2 == EOF) {
							break;
						}

						if (current_char == '`' && lookahead1 == '`' && lookahead2 == '`') {
							code_block_closed = true;
							flush_dynamic_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, output);
							create_and_add_token(TOKEN_BACKTICK, NULL, output);
							create_and_add_token(TOKEN_BACKTICK, NULL, output);
							create_and_add_token(TOKEN_BACKTICK, NULL, output);
						} else {
							ungetc(lookahead2, file);
							ungetc(lookahead1, file);
							
							if (!text_buffer) {
								text_buffer_capacity = 256;
								text_buffer = malloc(text_buffer_capacity);
							}
							append_to_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, current_char);
						}
					}
				} else {
					ungetc(next2, file);
					ungetc(next1, file);
					create_and_add_token(TOKEN_BACKTICK, NULL, output);
				}
				break;

			default:
				if (isdigit(current_char)) {
					flush_dynamic_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, output);

					if (!text_buffer) {
						text_buffer_capacity = 32;
						text_buffer = malloc(text_buffer_capacity);
					}
					append_to_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, current_char);

					while (isdigit(fpeek(file))) {
						append_to_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, fgetc(file));
					}
					if (text_buffer && text_buffer_idx > 0) {
						text_buffer[text_buffer_idx] = '\0';
						create_and_add_token(TOKEN_NUMBER, text_buffer, output);
						free(text_buffer);
						text_buffer = NULL;
						text_buffer_idx = 0;
						text_buffer_capacity = 0;
					}
				} else {
					if (!text_buffer) {
						text_buffer_capacity = 256;
						text_buffer = malloc(text_buffer_capacity);
					}
					append_to_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, current_char);
				}
				break;
		}
	}
	flush_dynamic_buffer(&text_buffer, &text_buffer_idx, &text_buffer_capacity, output);

	create_and_add_token(TOKEN_EOF, NULL, output);
}
