#include <stdio.h>
#include <stdlib.h>

#include "../include/list_head.h"
#include "../include/tokenizer.h"
#include "../include/parser.h"

const char* test_markdown_content =
	"# Main Title\n\n"
	"This is a paragraph with *italic text* and also **bold text**.\n"
	"This is the same paragraph, with a `inline code` snippet.\n"
	"\n"
	"## Links Section\n"
	"A standard link: [Google](https://google.com)\n"
	"An internal link: [[My Note]]\n"
	"An image link: ![[image.png]]\n"
	"\n"
	"```c\n"
	"// This is a code block\n"
	"int main(void) {\n"
	"  return 0;\n"
	"}\n"
	"```\n"
	"\n"
	"---\n\n"
	"- Unordered List Item 1\n"
	"- Item 2\n"
	"\t- Nested Item\n"
	"1. Ordered List Item 1\n"
	"2. Item 2\n";

static const char* token_type_to_string(TokenType type) {
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

static void print_tokens(struct list_head* head) {
    Token* current_token;
    int count = 1;
    printf("\n--- Tokenizer Result ---\n");
    list_for_each_entry(current_token, head, list) {
        printf("Token %2d: [%-12s]", count++, token_type_to_string(current_token->type));
        if (current_token->value) {
            printf(" Value: \"%s\"\n", current_token->value);
        } else {
            printf("\n");
        }
    }
    printf("------------------------\n");
}

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
		default: return "UNKNOWN";
	}
}

static void print_ast_tree(AstNode* node, int indent) {
	if (!node) return;
	for (int i = 0; i < indent; ++i) printf("\t");

	printf("-> %s", ast_node_type_to_string(node->type));
	if (node->data1) printf(" | data1: \"%s\"", node->data1);
	if (node->data2) printf(" | data2: \"%s\"", node->data2);
	printf("\n");

	if (!list_empty(&node->children)) {
		AstNode* child;
		list_for_each_entry(child, &node->children, list) {
			print_ast_tree(child, indent + 1);
		}
	}
}

int main() {
	// create test file
	const char* filename = "test_parser.tmp.md";
	FILE* test_file = fopen(filename, "w");
	if (!test_file) {
		perror("Failed to create temporary test file");
		return EXIT_FAILURE;
	}
	fputs(test_markdown_content, test_file);
	fclose(test_file);

	// run tokenizer
	LIST_HEAD(token_list);
	test_file = fopen(filename, "r");
	if (!test_file) {
		perror("Failed to open temporary test file for reading");
		remove(filename);
		return EXIT_FAILURE;
	}
	printf("--- running Tokenizer ---\n");
	tokenize_file(test_file, &token_list);
	printf("Tokenizer work done.\n");
	fclose(test_file);

	print_tokens(&token_list);

	// run parser
	printf("\n--- runnig Parser ---\n");
	AstNode* ast_root = parse_tokens(&token_list);
	printf("Parser work done.\n");

	// print AST
	printf("\n--- Abstract Syntax Tree (AST) Result ---\n");
	print_ast_tree(ast_root, 0);
	printf("-------------------------------------------\n");

	// clean
	printf("\n--- free memory ---\n");
	free_token_list(&token_list);
	printf("Token list memory freed.\n");
	free_ast(ast_root);
	printf("AST memory freed.\n");


	remove(filename);
	printf("temporary file removed.\n");

	return EXIT_SUCCESS;
}
