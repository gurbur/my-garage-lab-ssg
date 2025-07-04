#include <stdio.h>
#include <stdlib.h>

#include "../../src/include/tokenizer.h"
#include "../../src/include/list_head.h"

const char* token_type_to_string(TokenType type) {
	switch (type) {
		case TOKEN_HASH: return "HASH";
		case TOKEN_ASTERISK: return "ASTERISK";
		case TOKEN_TEXT: return "TEXT";
		case TOKEN_NUMBER: return "NUMBER";
		case TOKEN_DOT: return "DOT";
		case TOKEN_DASH: return "DASH";
		case TOKEN_NEWLINE: return "NEWLINE";
		case TOKEN_TAB: return "TAB";
		case TOKEN_LBRACKET: return "LBRACKET";
		case TOKEN_RBRACKET: return "RBRACKET";
		case TOKEN_LPAREN: return "LPAREN";
		case TOKEN_RPAREN: return "RPAREN";
		case TOKEN_BACKTICK: return "BACKTICK";
		case TOKEN_EXCLAMATION: return "EXCLAMATION";
		case TOKEN_GREATER_THAN: return "GREATER_THAN";
		case TOKEN_BACKSLASH: return "BACKSLASH";
		case TOKEN_EOF: return "EOF";
		default: return "UNKNOWN";
	}
}

void print_tokens_to_stdout(struct list_head* head) {
	Token* current_token;
	list_for_each_entry(current_token, head, list) {
		fprintf(stdout, "[%s]", token_type_to_string(current_token->type));
		if (current_token->value) {
			fprintf(stdout, "{\"%s\"}", current_token->value);
		} 
		fprintf(stdout, "\n");
	}
}

void free_tokens(struct list_head* head) {
	Token *current_token, *temp;

	list_for_each_entry_safe(current_token, temp, head, list) {
		list_del(&current_token->list);
		if (current_token->value) {
			free(current_token->value);
		}
		free(current_token);
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <input_markdown_file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	FILE* test_file = fopen(argv[1], "r");
	if (!test_file) {
		perror("Failed to read file");
		return EXIT_FAILURE;
	}

	LIST_HEAD(token_list);
	tokenize_file(test_file, &token_list);
	fclose(test_file);

	print_tokens_to_stdout(&token_list);

	free_tokens(&token_list);

	return EXIT_SUCCESS;
}
