#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/tokenizer.h"
#include "../include/list_head.h"

const char* test_markdown_content =
    "# Test Document\n"
    "This is the first paragraph.\n"
    "\n"
    "```c\n"
    "printf(\"Hello, Code Block!\\n\");\n"
    "```\n"
    "- list item 1\n"
    "\t- nested list item\n"
    "*italic* and **bold**\n";

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

void print_tokens(struct list_head* head) {
    Token* current_token;
    int count = 1;
    printf("--- Tokenization Result ---\n");
    list_for_each_entry(current_token, head, list) {
        printf("Token %2d: [%-12s]", count++, token_type_to_string(current_token->type));
        if (current_token->value) {
            if (strcmp(current_token->value, "\n") == 0) {
                 printf(" Value: \"\\n\"\n");
            } else {
                 printf(" Value: \"%s\"\n", current_token->value);
            }
        } else {
            printf("\n");
        }
    }
    printf("---------------------------\n");
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

int main() {
    const char* filename = "test_markdown.tmp.md";
    FILE* test_file = fopen(filename, "w");
    if (!test_file) {
        perror("Failed to create temporary test file");
        return EXIT_FAILURE;
    }
    fputs(test_markdown_content, test_file);
    fclose(test_file);

    LIST_HEAD(token_list);

    test_file = fopen(filename, "r");
    if (!test_file) {
        perror("Failed to open temporary test file for reading");
        remove(filename);
        return EXIT_FAILURE;
    }

    printf("Starting tokenization of %s...\n", filename);
    tokenize_file(test_file, &token_list);
    printf("Tokenization complete.\n\n");
    fclose(test_file);

    print_tokens(&token_list);

    free_tokens(&token_list);
    printf("All token memory has been freed.\n");

    if (remove(filename) == 0) {
        printf("Temporary file %s has been deleted.\n", filename);
    } else {
        perror("Failed to delete temporary file");
    }

    return EXIT_SUCCESS;
}
