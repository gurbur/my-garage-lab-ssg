#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "tokenizer_state.h"

void handle_punctuation(TokenizerState* state, char c);
void handle_fenced_code_block(TokenizerState* state);
void handle_number(TokenizerState* state, char first_char);

static bool is_punctuation(char c) {
	return strchr("#*-[]()!>\\.\n\t`", c) != NULL;
}

void tokenize_file(FILE* file, struct list_head* output) {
	TokenizerState state;
	state.stream = file;
	state.tokens = output;
	state.text_buffer = create_dynamic_buffer(256);
	if (!state.text_buffer) {
		perror("Failed to create dynamic buffer");
		exit(EXIT_FAILURE);
	}

	char current_char;
	while ((current_char = fgetc(state.stream)) != EOF) {
		if (current_char == '`') {
			int next1 = fgetc(state.stream);
			if (next1 == '`') {
				int next2 = fgetc(state.stream);
				if (next2 == '`') {
					handle_fenced_code_block(&state);
				} else {
					if(next2 != EOF) ungetc(next2, state.stream);
					if(next1 != EOF) ungetc(next1, state.stream);
					handle_punctuation(&state, current_char);
				}
			} else {
				if(next1 != EOF) ungetc(next1, state.stream);
				handle_punctuation(&state, current_char);
			}
		} else if (is_punctuation(current_char)) {
			handle_punctuation(&state, current_char);
		} else if (isdigit(current_char)) {
			handle_number(&state, current_char);
		} else {
			append_char_to_buffer(state.text_buffer, current_char);
		}
	}

	flush_buffer_as_token(state.text_buffer, TOKEN_TEXT, state.tokens);
	char* final_content = destroy_buffer_and_get_content(state.text_buffer);
	free(final_content);
	
	add_token(TOKEN_EOF, NULL, state.tokens);
}
