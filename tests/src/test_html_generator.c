#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "../../src/include/list_head.h"
#include "../../src/include/tokenizer.h"
#include "../../src/include/parser.h"
#include "../../src/include/html_generator.h"
#include "../../src/include/site_context.h"
#include "../../src/include/dynamic_buffer.h"

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

	SiteContext* s_context = create_site_context(".");
	TemplateContext* t_context = create_template_context();

	FILE* test_file = fopen(input_filename, "r");
	if (!test_file) {
		perror("unable to open file");
		free_site_context(s_context);
		return EXIT_FAILURE;
	}

	DynamicBuffer* db = create_dynamic_buffer(0);
	char line[1024];
	while(fgets(line, sizeof(line), test_file)) {
		buffer_append_formatted(db, "%s", line);
	}
	fclose(test_file);
	char* content_md = destroy_buffer_and_get_content(db);

	LIST_HEAD(token_list);
	tokenize_string(content_md, &token_list);
	free(content_md);
	AstNode* ast_root = parse_tokens(&token_list, s_context, input_filename);

	char* html_output = generate_html_from_ast(ast_root, t_context);

	if (html_output) {
		printf("%s", html_output);
	}

	free(html_output);
	free_ast(ast_root);
	free_token_list(&token_list);
	free_site_context(s_context);
	free_template_context(t_context);

	return EXIT_SUCCESS;
}
