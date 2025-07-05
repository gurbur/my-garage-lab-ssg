#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block_parser.h"
#include "inline_parser.h"

static AstNode* parse_paragraph(ParserState* state);
static AstNode* parse_heading(ParserState* state);
static AstNode* parse_line(ParserState* state);
static AstNode* parse_code_block(ParserState* state);
static AstNode* parse_list(ParserState* state, int expected_indent);
static AstNode* parse_list_item(ParserState* state, int item_indent);

AstNode* parse_block(ParserState* state) {
	struct list_head* start_pos = state->current_node;
	int indent = calculate_indent(state);

	struct list_head* temp_node = state->current_node->next;
	for (int i = 0; i < indent; ++i) {
		if (temp_node != state->head) temp_node = temp_node->next;
		else return parse_paragraph(state);
	}

	if (temp_node == state->head) return NULL;
	Token* start_token = list_entry(temp_node, Token, list);

	switch (start_token->type) {
		case TOKEN_HASH:
			consume_indent(state);
			return parse_heading(state);

		case TOKEN_DASH:
		case TOKEN_ASTERISK: {
			consume_indent(state);
			AstNode* line_node = parse_line(state);
			if (line_node) return line_node;

			state->current_node = start_pos;
			if (calculate_indent(state) == indent) {
				if (temp_node->next != state->head) {
					Token* after_marker = list_entry(temp_node->next, Token, list);
					if (after_marker->type == TOKEN_TEXT && after_marker->value && after_marker->value[0] == ' ') {
						return parse_list(state, indent);
					}
				}
			}
			break;
		}

		case TOKEN_NUMBER: {
			if (calculate_indent(state) == indent) {
				return parse_list(state, indent);
			}
			break;
		}

		case TOKEN_BACKTICK: {
			consume_indent(state);
			AstNode* code_node = parse_code_block(state);
			if (code_node) return code_node;
			state->current_node = start_pos;
			break;
		}
		default:
			break;
	}

	state->current_node = start_pos;
	return parse_paragraph(state);
}

static AstNode* parse_paragraph(ParserState* state) {
	AstNode* paragraph_node = create_ast_node(NODE_PARAGRAPH, NULL, NULL);
	parse_inline_elements(state, paragraph_node, false);
	return paragraph_node;
}

static AstNode* parse_list_item(ParserState* state, int item_indent) {
	AstNode* item_node = create_ast_node(NODE_LIST_ITEM, NULL, NULL);
	consume_token(state);
	if (peek_token(state) && peek_token(state)->type == TOKEN_DOT) {
		consume_token(state);
	}
	parse_inline_elements(state, item_node, true);
	match_token(state, TOKEN_NEWLINE);

	int next_indent = calculate_indent(state);
	if (next_indent > item_indent) {
		AstNode* sub_list = parse_list(state, next_indent);
		if (sub_list) {
			add_child_node(item_node, sub_list);
		}
	}
	return item_node;
}

static AstNode* parse_list(ParserState* state, int expected_indent) {
	struct list_head* temp_pos = state->current_node->next;
	for (int i = 0; i < expected_indent; i++) {
		if (temp_pos == state->head) return NULL;
		temp_pos = temp_pos->next;
	}
	if (temp_pos == state->head) return NULL;
	Token* start_token = list_entry(temp_pos, Token, list);

	AstNodeType list_type = (start_token->type == TOKEN_DASH || start_token->type == TOKEN_ASTERISK)
												? NODE_UNORDERED_LIST : NODE_ORDERED_LIST;

	AstNode* list_node = create_ast_node(list_type, NULL, NULL);

	while (peek_token(state)) {
		int current_indent = calculate_indent(state);
		if (current_indent != expected_indent) break;

		struct list_head* marker_pos = state->current_node->next;
		for (int i = 0; i < current_indent; i++) {
			if (marker_pos == state->head) return list_node;
			marker_pos = marker_pos->next;
		}
		if (marker_pos == state->head) return list_node;
		Token* marker = list_entry(marker_pos, Token, list);

		bool type_match = (list_type == NODE_UNORDERED_LIST && (marker->type == TOKEN_DASH || marker->type == TOKEN_ASTERISK)) ||
											(list_type == NODE_ORDERED_LIST && marker->type == TOKEN_NUMBER);

		if (type_match) {
			consume_indent(state);
			AstNode* item_node = parse_list_item(state, current_indent);
			add_child_node(list_node, item_node);
		} else {
			break;
		}
	}
	return list_node;
}

static AstNode* parse_heading(ParserState* state) {
	int level = 0;
	struct list_head* start_pos = state->current_node;
	while (peek_token(state) && peek_token(state)->type == TOKEN_HASH) {
		level++;
		consume_token(state);
	}
	Token* first_text = peek_token(state);
	if (!first_text || first_text->type != TOKEN_TEXT || !first_text->value || first_text->value[0] != ' ') {
		state->current_node = start_pos;
		return NULL;
	}

	int capacity = 256;
	int index = 0;
	char* buffer = malloc(capacity);
	buffer[0] = '\0';
	consume_token(state);
	if (strlen(first_text->value) > 1) {
		append_string_to_buffer(&buffer, &index, &capacity, first_text->value + 1);
	}
	while (peek_token(state) && peek_token(state)->type != TOKEN_NEWLINE) {
		Token* current = consume_token(state);
		append_string_to_buffer(&buffer, &index, &capacity, token_to_string(current));
	}

	AstNodeType heading_type = (level == 1) ? NODE_HEADING1 : (level == 2) ? NODE_HEADING2 : NODE_HEADING3;
	AstNode* heading_node = create_ast_node(heading_type, buffer, NULL);
	free(buffer);
	return heading_node;
}

static AstNode* parse_line(ParserState* state) {
	struct list_head* start_pos = state->current_node;
	Token* t1 = peek_token(state);
	if (!t1 || (t1->type != TOKEN_DASH && t1->type != TOKEN_ASTERISK)) return NULL;

	TokenType line_type = t1->type;
	int count = 0;
	while (peek_token(state) && peek_token(state)->type == line_type) {
		count++;
		consume_token(state);
	}
	Token* final_token = peek_token(state);
	if (count >= 3 && (final_token == NULL || final_token->type == TOKEN_NEWLINE || final_token->type == TOKEN_EOF)) {
		return create_ast_node(NODE_LINE, NULL, NULL);
	}
	state->current_node = start_pos;
	return NULL;
}

static AstNode* parse_code_block(ParserState* state) {
	struct list_head* start_pos = state->current_node;
	if (!(match_token(state, TOKEN_BACKTICK) && match_token(state, TOKEN_BACKTICK) && match_token(state, TOKEN_BACKTICK))) {
		state->current_node = start_pos;
		return NULL;
	}

	char* lang = NULL;
	Token* lang_token = peek_token(state);
	if (lang_token && lang_token->type == TOKEN_TEXT) {
		lang = lang_token->value;
		consume_token(state);
	}
	match_token(state, TOKEN_NEWLINE);

	char* code_content = NULL;
	Token* content_token = peek_token(state);
	if (content_token && content_token->type == TOKEN_TEXT) {
		code_content = content_token->value;
		consume_token(state);
	}

	if (!(match_token(state, TOKEN_BACKTICK) && match_token(state, TOKEN_BACKTICK) && match_token(state, TOKEN_BACKTICK))) {
		state->current_node = start_pos;
		return NULL;
	}
	return create_ast_node(NODE_CODE_BLOCK, code_content, lang);
}
