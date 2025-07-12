#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer_state.h"

void add_token(TokenType type, const char* value, struct list_head* tokens) {
	Token* new_token = (Token*)malloc(sizeof(Token));
	if (!new_token) {
		perror("Failed to allocate memory for token");
		exit(EXIT_FAILURE);
	}
	new_token->type = type;
	new_token->value = value ? strdup(value) : NULL;
	list_add_tail(&new_token->list, tokens);
}

void append_char_to_buffer(DynamicBuffer* buffer, char c) {
	buffer_append_formatted(buffer, "%c", c);
}

void flush_buffer_as_token(DynamicBuffer* buffer, TokenType type, struct list_head* tokens) {
	if (buffer->length > 0) {
		add_token(type, buffer->content, tokens);
		buffer->length = 0;
		buffer->content[0] = '\0';
	}
}

void handle_punctuation(TokenizerState* state, char c) {
	flush_buffer_as_token(state->text_buffer, TOKEN_TEXT, state->tokens);
	switch (c) {
		case '#': add_token(TOKEN_HASH, NULL, state->tokens); break;
		case '*': add_token(TOKEN_ASTERISK, NULL, state->tokens); break;
		case '-': add_token(TOKEN_DASH, NULL, state->tokens); break;
		case '\n': add_token(TOKEN_NEWLINE, NULL, state->tokens); break;
		case '\t': add_token(TOKEN_TAB, NULL, state->tokens); break;
		case '[': add_token(TOKEN_LBRACKET, NULL, state->tokens); break;
		case ']': add_token(TOKEN_RBRACKET, NULL, state->tokens); break;
		case '(': add_token(TOKEN_LPAREN, NULL, state->tokens); break;
		case ')': add_token(TOKEN_RPAREN, NULL, state->tokens); break;
		case '!': add_token(TOKEN_EXCLAMATION, NULL, state->tokens); break;
		case '>': add_token(TOKEN_GREATER_THAN, NULL, state->tokens); break;
		case '\\': add_token(TOKEN_BACKSLASH, NULL, state->tokens); break;
		case '.': add_token(TOKEN_DOT, NULL, state->tokens); break;
		case '`': add_token(TOKEN_BACKTICK, NULL, state->tokens); break;
	}
	state->current++;
}

void handle_fenced_code_block(TokenizerState* state) {
	flush_buffer_as_token(state->text_buffer, TOKEN_TEXT, state->tokens);
	add_token(TOKEN_BACKTICK, NULL, state->tokens);
	add_token(TOKEN_BACKTICK, NULL, state->tokens);
	add_token(TOKEN_BACKTICK, NULL, state->tokens);
	state->current += 3;

	while (*state->current && *state->current != '\n') {
		append_char_to_buffer(state->text_buffer, *state->current++);
	}
	flush_buffer_as_token(state->text_buffer, TOKEN_TEXT, state->tokens);
	if (*state->current == '\n') {
		add_token(TOKEN_NEWLINE, NULL, state->tokens);
		state->current++;
	}

	while (*state->current) {
		if (strncmp(state->current, "```", 3) == 0) {
			flush_buffer_as_token(state->text_buffer, TOKEN_TEXT, state->tokens);
			add_token(TOKEN_BACKTICK, NULL, state->tokens);
			add_token(TOKEN_BACKTICK, NULL, state->tokens);
			add_token(TOKEN_BACKTICK, NULL, state->tokens);
			state->current += 3;
			return;
		}
		append_char_to_buffer(state->text_buffer, *state->current++);
	}
}

void handle_number(TokenizerState* state) {
	flush_buffer_as_token(state->text_buffer, TOKEN_TEXT, state->tokens);
	
	const char* start = state->current;
	while (isdigit(*state->current)) {
		state->current++;
	}
	
	size_t len = state->current - start;
	char* value = malloc(len + 1);
	strncpy(value, start, len);
	value[len] = '\0';

	add_token(TOKEN_NUMBER, value, state->tokens);
	free(value);
}
