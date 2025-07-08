#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H

#include <stdbool.h>
#include "../include/list_head.h"
#include "../include/parser.h"

typedef struct {
	struct list_head* current_node;
	struct list_head* head;
} ParserState;

AstNode* create_ast_node(AstNodeType type, const char* data1, const char* data2);
void add_child_node(AstNode* parent, AstNode* child);

Token* peek_token(ParserState* state);
Token* consume_token(ParserState* state);
bool match_token(ParserState* state, TokenType type);
int calculate_indent(ParserState* state);
void consume_indent(ParserState* state);

const char* token_to_string(const Token* token);

#endif
