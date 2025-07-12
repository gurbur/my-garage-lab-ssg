#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/tokenizer.h"
#include "tokenizer_state.h"

void handle_punctuation(TokenizerState* state, char c);
void handle_fenced_code_block(TokenizerState* state);
void handle_number(TokenizerState* state);

static bool is_punctuation(char c) {
	return strchr("#*-[]()!>\\.\n\t`", c) != NULL;
}

void tokenize_string(const char* content, struct list_head* output) {
	TokenizerState state;
	state.current = content;
	state.tokens = output;
	state.text_buffer = create_dynamic_buffer(256);

	if (!state.text_buffer) {
		perror("Failed to create dynamic buffer");
		exit(EXIT_FAILURE);
	}

	while (*state.current != '\0') {
		if (strncmp(state.current, "```", 3) == 0) {
			handle_fenced_code_block(&state);
		} else if (is_punctuation(*state.current)) {
			handle_punctuation(&state, *state.current);
		} else if (isdigit(*state.current)) {
			handle_number(&state);
		} else {
			append_char_to_buffer(state.text_buffer, *state.current);
			state.current++;
		}
	}

	flush_buffer_as_token(state.text_buffer, TOKEN_TEXT, state.tokens);
	char* final_content = destroy_buffer_and_get_content(state.text_buffer);
	free(final_content);
	
	add_token(TOKEN_EOF, NULL, state.tokens);

}

void tokenize_file(FILE* file, struct list_head* output) {
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = malloc(length + 1);
	if (!buffer) {
		perror("Failed to allocate buffer for file content");
		exit(EXIT_FAILURE);
	}

	fread(buffer, 1, length, file);
	buffer[length] = '\0';

	tokenize_string(buffer, output);

	free(buffer);
}
