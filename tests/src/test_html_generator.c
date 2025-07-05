#include <stdio.h>
#include <stdlib.h>

#include "../../src/include/list_head.h"
#include "../../src/include/tokenizer.h"
#include "../../src/include/parser.h"
#include "../../src/include/html_generator.h"

static void free_token_list(struct list_head* head) {
	Token *current_token, *temp;
	list_for_each_entry_safe(current_token, temp, head, list) {
		list_del(&current_token->list);
		if (current_token->value) free(current_token->value);
		free(current_token);
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <input_markdown_file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char* input_filename = argv[1];

	FILE* test_file = fopen(input_filename, "r");
	if (!test_file) {
		perror("unable to open file");
		return EXIT_FAILURE;
	}

	LIST_HEAD(token_list);
	tokenize_file(test_file, &token_list);
	fclose(test_file);

	AstNode* ast_root = parse_tokens(&token_list);

	char* html_output = generate_html_from_ast(ast_root);

	if (html_output) {
		printf("%s", html_output);
	}

	free(html_output);
	free_ast(ast_root);
	free_token_list(&token_list);

	return EXIT_SUCCESS;
}
