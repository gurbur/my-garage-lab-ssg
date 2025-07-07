#include <stdlib.h>
#include <string.h>
#include "inline_parser.h"

static AstNode* parse_emphasis(ParserState* state);
static AstNode* parse_inline_code(ParserState* state);
static AstNode* parse_standard_link(ParserState* state);
static AstNode* parse_obsidian_link(ParserState* state, bool is_image);

void parse_inline_elements(ParserState* state, AstNode* parent_node, bool is_list_item) {
	int capacity = 256;
	int index = 0;
	char* text_buffer = malloc(capacity);
	text_buffer[0] = '\0';

	while (peek_token(state) && peek_token(state)->type != TOKEN_EOF) {
		Token* t1 = peek_token(state);

		if (t1->type == TOKEN_NEWLINE) {
			if (is_list_item) {
				break;
			} else {
				Token* t2 = (t1->list.next != state->head) ? list_entry(t1->list.next, Token, list) : NULL;
				if (t2 && t2->type == TOKEN_NEWLINE) {
					break;
				}
			}
		}

		AstNode* new_node = NULL;
		if (t1->type == TOKEN_ASTERISK) new_node = parse_emphasis(state);
		else if (t1->type == TOKEN_BACKTICK) new_node = parse_inline_code(state);
		else if (t1->type == TOKEN_EXCLAMATION) new_node = parse_obsidian_link(state, true);
		else if (t1->type == TOKEN_LBRACKET) {
			Token* lookahead = (t1->list.next != state->head) ? list_entry(t1->list.next, Token, list) : NULL;
			if (lookahead && lookahead->type == TOKEN_LBRACKET) new_node = parse_obsidian_link(state, false);
			else new_node = parse_standard_link(state);
		}

		if (new_node) {
			flush_buffer_if_needed(parent_node, text_buffer, &index);
			add_child_node(parent_node, new_node);
			continue;
		}

		Token* current_token = consume_token(state);
		if (current_token->type == TOKEN_NEWLINE) {
			append_string_to_buffer(&text_buffer, &index, &capacity, " ");
		} else {
			append_string_to_buffer(&text_buffer, &index, &capacity, token_to_string(current_token));
		}
	}
	flush_buffer_if_needed(parent_node, text_buffer, &index);
	free(text_buffer);
}

static AstNode* parse_emphasis(ParserState* state) {
	struct list_head* start_pos = state->current_node;
	int level = 0;

	while (peek_token(state) && peek_token(state)->type == TOKEN_ASTERISK) {
		level++;
		consume_token(state);
	}
	if (level == 0 || level > 3) {
		state->current_node = start_pos;
		return NULL;
	}

	int capacity = 256;
	int index = 0;
	char* content_buffer = malloc(capacity);
	if (!content_buffer) {
		perror("Failed to allocate buffer for emphasis");
		exit(EXIT_FAILURE);
	}
	content_buffer[0] = '\0';

	while (peek_token(state)) {
		Token* current = peek_token(state);

		if (current->type == TOKEN_ASTERISK) {
			int closing_level = 0;
			struct list_head* temp_pos = state->current_node;

			while(peek_token(state) && peek_token(state)->type == TOKEN_ASTERISK) {
				closing_level++;
				consume_token(state);
			}

			if (level == closing_level) {
				AstNodeType type = (level == 1) ? NODE_ITALIC : (level == 2) ? NODE_BOLD : NODE_ITALIC_AND_BOLD;
				AstNode* node = create_ast_node(type, content_buffer, NULL);
				free(content_buffer);
				return node;
			}

			state->current_node = temp_pos;
		}

		Token* token_to_add = consume_token(state);
		if (!token_to_add || token_to_add->type == TOKEN_NEWLINE || token_to_add->type == TOKEN_EOF) {
			break;
		}
		append_string_to_buffer(&content_buffer, &index, &capacity, token_to_string(token_to_add));
	}

	free(content_buffer);
	state->current_node = start_pos;
	return NULL;
}

static AstNode* parse_inline_code(ParserState* state) {
	struct list_head* start_pos = state->current_node;
	if (!peek_token(state) || peek_token(state)->type != TOKEN_BACKTICK) return NULL;
	consume_token(state);

	Token* text_token = peek_token(state);
	if (text_token && text_token->type == TOKEN_TEXT) {
		consume_token(state);
		if (peek_token(state) && peek_token(state)->type == TOKEN_BACKTICK) {
			consume_token(state);
			return create_ast_node(NODE_CODE, text_token->value, NULL);
		}
	}
	state->current_node = start_pos;
	return NULL;
}

static AstNode* parse_standard_link(ParserState* state) {
	struct list_head* start_pos = state->current_node;
	if (!match_token(state, TOKEN_LBRACKET)) return NULL;

	Token* text_token = peek_token(state);
	if (text_token && text_token->type == TOKEN_TEXT) {
		consume_token(state);
		if (match_token(state, TOKEN_RBRACKET) && match_token(state, TOKEN_LPAREN)) {
			int capacity = 256;
			int index = 0;
			char* url_buffer = malloc(capacity);
			url_buffer[0] = '\0';
			while(peek_token(state) && peek_token(state)->type != TOKEN_RPAREN) {
				Token* current = consume_token(state);
				append_string_to_buffer(&url_buffer, &index, &capacity, token_to_string(current));
			}
			if (match_token(state, TOKEN_RPAREN)) {
				AstNode* link_node = create_ast_node(NODE_LINK, text_token->value, url_buffer);
				free(url_buffer);
				return link_node;
			}
			free(url_buffer);
		}
	}
	state->current_node = start_pos;
	return NULL;
}

static AstNode* parse_obsidian_link(ParserState* state, bool is_image) {
	struct list_head* start_pos = state->current_node;
	if (is_image) {
		if (!match_token(state, TOKEN_EXCLAMATION)) return NULL;
	}
	if (!match_token(state, TOKEN_LBRACKET)) { state->current_node = start_pos; return NULL; }
	if (!match_token(state, TOKEN_LBRACKET)) { state->current_node = start_pos; return NULL; }

	int capacity = 256;
	int index = 0;
	char* filename_buffer = malloc(capacity);
	filename_buffer[0] = '\0';
	while(peek_token(state)) {
		Token* t1 = peek_token(state);
		if (t1->list.next != state->head) {
			Token* t2 = list_entry(t1->list.next, Token, list);
			if (t1->type == TOKEN_RBRACKET && t2 && t2->type == TOKEN_RBRACKET) break;
		} else {
			break;
		}
		Token* current = consume_token(state);
		append_string_to_buffer(&filename_buffer, &index, &capacity, token_to_string(current));
	}

	if (match_token(state, TOKEN_RBRACKET) && match_token(state, TOKEN_RBRACKET)) {
		AstNode* link_node = create_ast_node(is_image ? NODE_IMAGE_LINK : NODE_LINK, filename_buffer, is_image ? NULL : filename_buffer);
		free(filename_buffer);
		return link_node;
	}
	free(filename_buffer);
	state->current_node = start_pos;
	return NULL;
}
