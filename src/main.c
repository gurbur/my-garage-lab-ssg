#include <stdio.h>
#include <stdlib.h>

#include "include/list_head.h"
#include "include/tokenizer.h"
#include "include/parser.h"
//#include "include/html_generator.h"
//#include "include/template_engine.h"

static void free_token_list(struct list_head* head) {
	Token *current_token, *temp;
	list_for_each_entry_safe(current_token, temp, head, list) {
		list_del(&current_token->list);
		if (current_token->value) {
			free(current_token->value);
		}
		free(current_token);
	}
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file.md>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char* input_filename = argv[1];

    FILE* input_file = fopen(input_filename, "r");
    if (!input_file) {
				perror("cannot open file.");
        return EXIT_FAILURE;
    }
    printf("openned '%s' file successfully.\n", input_filename);

    LIST_HEAD(token_list);
    
		// Tokenize
    printf("running Tokenizer module...\n");
    tokenize_file(input_file, &token_list);
    printf("Tokenization completed.\n");

    fclose(input_file);

		// Parse
    printf("running Parser module...\n");
    AstNode* ast_root = parse_tokens(&token_list);
    printf("Parsing completed.\n");

		// Generate HTML
    printf("running HTML Generator module...\n");
    // generate_html(ast_root, "output.html");
    printf("HTML generation completed.\n");

		// Build complete HTML
		printf("running template engine...\n");
		// render_template(const char* template_content, const char* content_html, FrontMatter* metadata);
		printf("HTML build completed.\n");

		// Free used memory
    printf("freeing every used memory...\n");
    free_token_list(&token_list);
    // free_ast(ast_root);
    printf("all works done successfully.\n");

    return EXIT_SUCCESS;
}
