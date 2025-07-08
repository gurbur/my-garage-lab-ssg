#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser_utils.h"

AstNode* create_ast_node(AstNodeType type, const char* data1, const char* data2) {
	AstNode* node = (AstNode*)malloc(sizeof(AstNode));
	if (!node) {
		perror("Failed to allocate AstNode");
		exit(EXIT_FAILURE);
	}
	node->type = type;
	node->data1 = data1 ? strdup(data1) : NULL;
	node->data2 = data2 ? strdup(data2) : NULL;
	INIT_LIST_HEAD(&node->list);
	INIT_LIST_HEAD(&node->children);
	return node;
}

void add_child_node(AstNode* parent, AstNode* child) {
	if (parent && child) {
		list_add_tail(&child->list, &parent->children);
	}
}

Token* peek_token(ParserState* state) {
	if (!state || !state->current_node || state->current_node->next == state->head) return NULL;
	return list_entry(state->current_node->next, Token, list);
}

Token* consume_token(ParserState* state) {
	if (!state || !state->current_node || state->current_node->next == state->head) return NULL;
	state->current_node = state->current_node->next;
	return list_entry(state->current_node, Token, list);
}

bool match_token(ParserState* state, TokenType type) {
	Token* token = peek_token(state);
	if (token && token->type == type) {
		consume_token(state);
		return true;
	}
	return false;
}

int calculate_indent(ParserState* state) {
	int indent = 0;
	struct list_head* temp_node = state->current_node->next;
	while (temp_node != state->head) {
		Token* token = list_entry(temp_node, Token, list);
		if (token->type == TOKEN_TAB) {
			indent++;
			temp_node = temp_node->next;
		} else {
			break;
		}
	}
	return indent;
}

void consume_indent(ParserState* state) {
	while (peek_token(state) && peek_token(state)->type == TOKEN_TAB) {
		consume_token(state);
	}
}

const char* token_to_string(const Token* token) {
	if (!token) return "";
	if (token->value) return token->value;
	switch (token->type) {
		case TOKEN_DOT: return "."; case TOKEN_HASH: return "#";
		case TOKEN_ASTERISK: return "*"; case TOKEN_DASH: return "-";
		case TOKEN_LPAREN: return "("; case TOKEN_RPAREN: return ")";
		case TOKEN_LBRACKET: return "["; case TOKEN_RBRACKET: return "]";
		case TOKEN_BACKTICK: return "`"; case TOKEN_EXCLAMATION: return "!";
		case TOKEN_GREATER_THAN: return ">"; case TOKEN_BACKSLASH: return "\\";
		default: return "";
	}
}

