#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "../../src/include/list_head.h"
#include "../../src/include/tokenizer.h"
#include "../../src/include/parser.h"
#include "../../src/include/dynamic_buffer.h"


static void free_token_list(struct list_head* head) {
	Token *current_token, *temp;
	list_for_each_entry_safe(current_token, temp, head, list) {
		list_del(&current_token->list);
		if (current_token->value) free(current_token->value);
		free(current_token);
	}
}

static const char* ast_node_type_to_string(AstNodeType type) {
	switch (type) {
		case NODE_DOCUMENT: return "DOCUMENT";
		case NODE_HEADING1: return "HEADING1";
		case NODE_HEADING2: return "HEADING2";
		case NODE_HEADING3: return "HEADING3";
		case NODE_PARAGRAPH: return "PARAGRAPH";
		case NODE_CODE_BLOCK: return "CODE_BLOCK";
		case NODE_LINE: return "LINE";
		case NODE_ORDERED_LIST: return "ORDERED_LIST";
		case NODE_UNORDERED_LIST: return "UNORDERED_LIST";
		case NODE_LIST_ITEM: return "LIST_ITEM";
		case NODE_IMAGE_LINK: return "IMAGE_LINK";
		case NODE_TEXT: return "TEXT";
		case NODE_ITALIC: return "ITALIC";
		case NODE_BOLD: return "BOLD";
		case NODE_ITALIC_AND_BOLD: return "ITALIC_AND_BOLD";
		case NODE_CODE: return "CODE";
		case NODE_LINK: return "LINK";
		case NODE_SOFT_BREAK: return "SOFT_BREAK";
		default: return "UNKNOWN";
	}
}

static void print_ast_stdout(AstNode* node, int indent) {
	if (!node) return;
	for (int i = 0; i < indent; ++i) fprintf(stdout, "\t");

	fprintf(stdout, "-> %s", ast_node_type_to_string(node->type));
	if (node->data1) fprintf(stdout, " | data1: \"%s\"", node->data1);
	if (node->data2) fprintf(stdout, " | data2: \"%s\"", node->data2);
	fprintf(stdout, "\n");

	if (!list_empty(&node->children)) {
		AstNode* child;
		list_for_each_entry(child, &node->children, list) {
			print_ast_stdout(child, indent + 1);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <input_markdown_file>\n", argv[0]);
		return EXIT_FAILURE;
	}
	const char* input_filename = argv[1];

	SiteContext* s_context = create_site_context(".");
	
	// create test file
	FILE* test_file = fopen(argv[1], "r");
	if (!test_file) {
		perror("Failed to open file");
		return EXIT_FAILURE;
	}

	DynamicBuffer* db = create_dynamic_buffer(0);
	char line[1024];
	while(fgets(line, sizeof(line), test_file)) {
		buffer_append_formatted(db, "%s", line);
	}
	fclose(test_file);
	char* content_md = destroy_buffer_and_get_content(db);

	// run tokenizer
	LIST_HEAD(token_list);
	tokenize_string(content_md, &token_list);

	// run parser
	AstNode* ast_root = parse_tokens(&token_list, s_context, input_filename);

	// print AST
	print_ast_stdout(ast_root, 0);

	// clean
	free_token_list(&token_list);
	free_ast(ast_root);
	free_site_context(s_context);

	return EXIT_SUCCESS;
}
