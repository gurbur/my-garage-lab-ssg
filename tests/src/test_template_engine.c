#include <stdio.h>
#include <stdlib.h>

#include "../../src/include/list_head.h"
#include "../../src/include/tokenizer.h"
#include "../../src/include/parser.h"
#include "../../src/include/html_generator.h"
#include "../../src/include/template_engine.h"
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

	FILE* md_file = fopen(input_filename, "r");
	if (!md_file) {
		perror("Unable to open markdown file");
		free_site_context(s_context);
		return EXIT_FAILURE;
	}

	DynamicBuffer* db = create_dynamic_buffer(0);
	char line[1024];
	while (fgets(line, sizeof(line), md_file)) {
		buffer_append_formatted(db, "%s", line);
	}
	fclose(md_file);
	char* content_md = destroy_buffer_and_get_content(db);

	LIST_HEAD(token_list);
	tokenize_string(content_md, &token_list);
	AstNode* ast_root = parse_tokens(&token_list, s_context, input_filename);
	char* content_html = generate_html_from_ast(ast_root);
	free(content_md);

	TemplateContext* context = create_template_context();

	add_to_context(context, "title", "생성된 테스트 페이지");
	add_to_context(context, "writer", "Test Runner");
	add_to_context(context, "date", "2025-07-11");
	add_to_context(context, "breadcrumb", "테스트 / 자동 생성된 페이지");

	add_to_context(context, "post_content", content_html);

	add_to_context(context, "table_of_contents", "<a href=\"#\">생성된 목차</a>");
	add_to_context(context, "prev_post_link", "#");
	add_to_context(context, "next_post_link", "#");


	const char* layout_path = "templates/layout/post_page_layout.html";
	char* final_html = render_template(layout_path, context);

	if (final_html) {
		printf("%s", final_html);
	}

	free(content_html);
	free(final_html);
	free_ast(ast_root);
	free_token_list(&token_list);
	free_template_context(context);
	free_site_context(s_context);

	return EXIT_SUCCESS;
}
