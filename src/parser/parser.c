#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../include/list_head.h"
#include "../include/parser.h"

typedef struct {
	struct list_head* current_node;
	struct list_head* head;
} ParserState;

static AstNode* create_ast_node(AstNodeType type, char* data1, char* data2) {
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

static void add_child_node(AstNode* parent, AstNode* child) {
	list_add_tail(&child->list, &parent->children);
}

static Token* peek_token(ParserState* p) {
	if (p->current_node->next == p->head) return NULL;
	return list_entry(p->current_node->next, Token, list);
}

static Token* consume_token(ParserState* p) {
	if (p->current_node->next == p->head) return NULL;
	p->current_node = p->current_node->next;
	return list_entry(p->current_node, Token, list);
}

static bool match_token(ParserState* p, TokenType type) {
	Token* token = peek_token(p);
	if (token && token->type == type) {
		consume_token(p);
		return true;
	}
	return false;
}

static int count_indent(ParserState* p) {
	int indent = 0;
	struct list_head* original_pos = p->current_node;

	struct list_head* temp_node = p->current_node;
	while (temp_node->next != p->head) {
		temp_node = temp_node->next;
		Token* token = list_entry(temp_node, Token, list);

		if (token->type == TOKEN_TAB) {
			indent++;
		} else {
			break;
		}
	}
	return indent;
}

static const char* token_to_string(Token* token) {
	if (!token) return "";
	if (token->type == TOKEN_TEXT) return token->value;
	if (token->type == TOKEN_DOT) return ".";
	if (token->type == TOKEN_NUMBER) return token->value;
	return "";
}

// ------------------------------

static void parse_inline_elements(ParserState* p, AstNode* parent_node, bool is_list_item);
static void append_string_to_buffer(char** buffer, int* index, int* capacity, const char* str);
static AstNode* parse_list(ParserState* p, int expected_indent);

AstNode* parse_heading(ParserState* p) {
	int level = 0;
	while (peek_token(p) && peek_token(p)->type == TOKEN_HASH) {
		level++;
		consume_token(p);
	}

	Token* first_text = peek_token(p);
	if (!first_text || first_text->type != TOKEN_TEXT || first_text->value[0] != ' ') {
		//printf("hash camed, but heading is not detected\n");
		//printf("%s\n", first_text->value);
		return NULL;
	}
	
	int capacity = 256;
	int index = 0;
	char* buffer = malloc(capacity);
	if (!buffer) {
		perror("Failed to allocate buffer for heading");
		exit(EXIT_FAILURE);
	}
	buffer[0] = '\0';

	consume_token(p);
	if (strlen(first_text->value) > 1) {
		append_string_to_buffer(&buffer, &index, &capacity, first_text->value + 1);
	}

	while (peek_token(p) && peek_token(p)->type != TOKEN_NEWLINE) {
		Token* current = consume_token(p);
		if (current->type == TOKEN_TEXT) append_string_to_buffer(&buffer, &index, &capacity, current->value);
		else if (current->type == TOKEN_LPAREN) append_string_to_buffer(&buffer, &index, &capacity, "(");
		else if (current->type == TOKEN_RPAREN) append_string_to_buffer(&buffer, &index, &capacity, ")");
		else if (current->type == TOKEN_NUMBER) append_string_to_buffer(&buffer, &index, &capacity, current->value);
		else if (current->type == TOKEN_LBRACKET) append_string_to_buffer(&buffer, &index, &capacity, "{");
		else if (current->type == TOKEN_RBRACKET) append_string_to_buffer(&buffer, &index, &capacity, "}");
		else if (current->type == TOKEN_ASTERISK) append_string_to_buffer(&buffer, &index, &capacity, "*");
		else if (current->type == TOKEN_DASH) append_string_to_buffer(&buffer, &index, &capacity, "-");
		else if (current->type == TOKEN_DOT) append_string_to_buffer(&buffer, &index, &capacity, ".");
		else if (current->type == TOKEN_EXCLAMATION) append_string_to_buffer(&buffer, &index, &capacity, "!");
		else if (current->type == TOKEN_HASH) append_string_to_buffer(&buffer, &index, &capacity, "#");
	}

	AstNodeType heading_type;
	if (level == 1) heading_type = NODE_HEADING1;
	else if (level == 2) heading_type = NODE_HEADING2;
	else heading_type = NODE_HEADING3;

	AstNode* heading_node = create_ast_node(heading_type, NULL, NULL);
	AstNode* text_node = create_ast_node(NODE_TEXT, buffer, NULL);
	add_child_node(heading_node, text_node);

	free(buffer);

	match_token(p, TOKEN_NEWLINE);

	return heading_node;
}

static AstNode* parse_standard_link(ParserState* p) {
	struct list_head* start_pos = p->current_node;

	if (!match_token(p, TOKEN_LBRACKET)) return NULL;

	Token* text_token = peek_token(p);
	if (text_token && text_token->type == TOKEN_TEXT) {
		consume_token(p);
		if (match_token(p, TOKEN_RBRACKET) && match_token(p, TOKEN_LPAREN)) {
			int capacity = 256;
			int index = 0;
			char* url_buffer = malloc(capacity);
			url_buffer[0] = '\0';

			while(peek_token(p) && peek_token(p)->type != TOKEN_RPAREN) {
				Token* current = consume_token(p);
				append_string_to_buffer(&url_buffer, &index, &capacity, token_to_string(current));
			}

			if (match_token(p, TOKEN_RPAREN)) {
				AstNode* link_node = create_ast_node(NODE_LINK, text_token->value, url_buffer);
				free(url_buffer);
				return link_node;
			}
			free(url_buffer);
		}
	}
	p->current_node = start_pos;
	return NULL;
}

static AstNode* parse_obsidian_link(ParserState* p, bool is_image) {
	struct list_head* start_pos = p->current_node;

	if (is_image) {
		if (!match_token(p, TOKEN_EXCLAMATION)) return NULL;
	}

	if (!match_token(p, TOKEN_LBRACKET)) { p->current_node = start_pos; return NULL; }
	if (!match_token(p, TOKEN_LBRACKET)) { p->current_node = start_pos; return NULL; }

	int capacity = 256;
	int index = 0;
	char* filename_buffer = malloc(capacity);
	filename_buffer[0] = '\0';

	while(peek_token(p)) {
		Token* t1 = peek_token(p);
		Token* t2 = (t1->list.next != p->head) ? list_entry(t1->list.next, Token, list) : NULL;

		if (t1->type == TOKEN_RBRACKET && t2 && t2->type == TOKEN_RBRACKET) {
			break;
		}
		Token* current = consume_token(p);
		append_string_to_buffer(&filename_buffer, &index, &capacity, token_to_string(current));
	}

	if (match_token(p, TOKEN_RBRACKET) && match_token(p, TOKEN_RBRACKET)) {
		AstNode* link_node;
		if (is_image) {
			link_node = create_ast_node(NODE_IMAGE_LINK, filename_buffer, NULL);
		} else {
			link_node = create_ast_node(NODE_LINK, filename_buffer, filename_buffer);
		}
		free(filename_buffer);
		return link_node;
	}

	free(filename_buffer);
	p->current_node = start_pos;
	return NULL;
}

static AstNode* parse_line(ParserState* p) {
	Token* t1 = peek_token(p);
	if (t1->type != TOKEN_DASH && t1->type != TOKEN_ASTERISK) return NULL;
	TokenType line_type = t1->type;

	int count = 0;
	struct list_head* start_pos = p->current_node;

	while (peek_token(p) && peek_token(p)->type == line_type) {
		count++;
		consume_token(p);
	}

	if (count >= 3 && (peek_token(p) == NULL || peek_token(p)->type == TOKEN_NEWLINE || peek_token(p)->type == TOKEN_EOF)) {
		return create_ast_node(NODE_LINE, NULL, NULL);
	}
	p->current_node = start_pos;
	return NULL;
}

static AstNode* parse_code_block(ParserState* p) {
	struct list_head* start_pos = p->current_node;

	if (!(match_token(p, TOKEN_BACKTICK) && match_token(p, TOKEN_BACKTICK) && match_token(p, TOKEN_BACKTICK))) {
		p->current_node = start_pos;
		return NULL;
	}

	char* lang = NULL;
	Token* lang_token = peek_token(p);
	if (lang_token && lang_token->type == TOKEN_TEXT) {
		lang = lang_token->value;
		consume_token(p);
	}
	match_token(p, TOKEN_NEWLINE);

	char* code_content = NULL;
	Token* content_token = peek_token(p);
	if (content_token && content_token->type == TOKEN_TEXT) {
		code_content = content_token->value;
		consume_token(p);
	}

	if (!(match_token(p, TOKEN_BACKTICK) && match_token(p, TOKEN_BACKTICK) && match_token(p, TOKEN_BACKTICK))) {
		p->current_node = start_pos;
		return NULL;
	}

	return create_ast_node(NODE_CODE_BLOCK, code_content, lang);
}

static AstNode* parse_list_item(ParserState* p, int current_indent) {
	AstNode* item_node = create_ast_node(NODE_LIST_ITEM, NULL, NULL);

	consume_token(p);
	if (peek_token(p) && peek_token(p)->type == TOKEN_DOT) {
		consume_token(p);
	}

	parse_inline_elements(p, item_node, true);
	match_token(p, TOKEN_NEWLINE);

	int next_indent = count_indent(p);
	if (next_indent > current_indent) {
		AstNode* sub_list = parse_list(p, next_indent);
		if (sub_list) {
			add_child_node(item_node, sub_list);
		}
	}

	return item_node;
}

static AstNode* parse_list(ParserState* p, int expected_indent) {
	struct list_head* temp_pos = p->current_node;
	for (int i = 0; i < expected_indent; i++) {
		if (temp_pos->next != p->head) {
			temp_pos = temp_pos->next;
		}
	}
	Token* start_token = list_entry(temp_pos->next, Token, list);

	AstNodeType list_type = (start_token->type == TOKEN_DASH || start_token->type == TOKEN_ASTERISK)
		? NODE_UNORDERED_LIST
		: NODE_ORDERED_LIST;

	AstNode* list_node = create_ast_node(list_type, NULL, NULL);

	while (peek_token(p)) {
		int current_indent = count_indent(p);
		if (current_indent != expected_indent) {
			break;
		}

		for (int i = 0; i < current_indent; i++) {
			consume_token(p);
		}

		Token* marker = peek_token(p);
		if (!marker) break;

		bool type_match = false;
		if (list_type == NODE_UNORDERED_LIST && (marker->type == TOKEN_DASH || marker->type == TOKEN_ASTERISK)) {
			type_match = true;
		} else if (list_type == NODE_ORDERED_LIST && marker->type == TOKEN_NUMBER) {
			type_match = true;
		}

		if (type_match) {
			AstNode* item_node = parse_list_item(p, current_indent);
			add_child_node(list_node, item_node);
		} else {
			break;
		}
	}
	return list_node;
}

static void append_string_to_buffer(char** buffer, int* index, int* capacity, const char* str) {
	int str_len = strlen(str);
	while(*index + str_len >= *capacity - 1) {
		*capacity *= 2;
		char* new_buffer = realloc(*buffer, *capacity);
		if (!new_buffer) {
			perror("Failed to reallocate buffer for paragraph");
			free(*buffer);
			exit(EXIT_FAILURE);
		}
		*buffer = new_buffer;
	}
	strcat(*buffer, str);
	*index += str_len;
}

static AstNode* parse_inline_code(ParserState* p) {
	struct list_head* start_pos = p->current_node;
	consume_token(p);

	Token* text_token = peek_token(p);
	if (text_token && text_token->type == TOKEN_TEXT) {
		consume_token(p);

		if (peek_token(p) && peek_token(p)->type == TOKEN_BACKTICK) {
			consume_token(p);
			return create_ast_node(NODE_CODE, text_token->value, NULL);
		}
	}
	p->current_node = start_pos;
	return NULL;
}

static AstNode* parse_emphasis(ParserState* p) {
	struct list_head* start_pos = p->current_node;

	int level = 0;
	while (peek_token(p) && peek_token(p)->type == TOKEN_ASTERISK) {
		level++;
		consume_token(p);
	}

	if (level == 0 || level > 3) {
		p->current_node = start_pos;
		return NULL;
	}

	Token* text_token = peek_token(p);
	if (text_token && text_token->type == TOKEN_TEXT) {
		consume_token(p);

		int closing_level = 0;
		while(peek_token(p) && peek_token(p)->type == TOKEN_ASTERISK) {
			closing_level++;
			consume_token(p);
		}

		if (level == closing_level) {
			AstNodeType type;
			if (level == 1) type = NODE_ITALIC;
			else if (level == 2) type = NODE_BOLD;
			else type = NODE_ITALIC_AND_BOLD;
			return create_ast_node(type, text_token->value, NULL);
		}
	}

	p->current_node = start_pos;
	return NULL;
}

static void parse_inline_elements(ParserState* p, AstNode* parent_node, bool is_list_item) {
	while(peek_token(p) && peek_token(p)->type != TOKEN_EOF) {
		Token* t1 = peek_token(p);

		if (is_list_item) {
			if (t1->type == TOKEN_NEWLINE) {
				break;
			}
		} else {
			Token* t2 = (t1->list.next != p->head) ? list_entry(t1->list.next, Token, list) : NULL;
				if (t1->type == TOKEN_NEWLINE && t2 && t2->type == TOKEN_NEWLINE) {
					// end of this paragraph.
					// new paragraph detected(\n\n)
					break;
				}
		}

		AstNode* new_node = NULL;
		TokenType type = t1->type;

		if (type == TOKEN_ASTERISK) new_node = parse_emphasis(p);
		else if (type == TOKEN_BACKTICK) new_node = parse_inline_code(p);
		else if (type == TOKEN_EXCLAMATION) new_node = parse_obsidian_link(p, true);
		else if (type == TOKEN_LBRACKET) {
			Token* lookahead = (t1->list.next != p->head) ? list_entry(t1->list.next, Token, list) : NULL;
			if (lookahead && lookahead->type == TOKEN_LBRACKET) new_node = parse_obsidian_link(p, false);
			else new_node = parse_standard_link(p);
		}

		if (new_node) {
			add_child_node(parent_node, new_node);
			continue;
		}

		Token* current_token = consume_token(p);
		if (type == TOKEN_TEXT)
			new_node = create_ast_node(NODE_TEXT, current_token->value, NULL);
		else if (type == TOKEN_NEWLINE)
			new_node = create_ast_node(NODE_TEXT, " ", NULL);
		else {
			if (type == TOKEN_ASTERISK) new_node = create_ast_node(NODE_TEXT, "*", NULL);
			else if (type == TOKEN_HASH) new_node = create_ast_node(NODE_TEXT, "#", NULL);
			else if (type == TOKEN_DASH) new_node = create_ast_node(NODE_TEXT, "-", NULL);
			else if (type == TOKEN_DOT) new_node = create_ast_node(NODE_TEXT, ".", NULL);
			else if (type == TOKEN_TAB) new_node = create_ast_node(NODE_TEXT, "\t", NULL);
			else if (type == TOKEN_LBRACKET) new_node = create_ast_node(NODE_TEXT, "[", NULL);
			else if (type == TOKEN_RBRACKET) new_node = create_ast_node(NODE_TEXT, "]", NULL);
			else if (type == TOKEN_LPAREN) new_node = create_ast_node(NODE_TEXT, "(", NULL);
			else if (type == TOKEN_RPAREN) new_node = create_ast_node(NODE_TEXT, ")", NULL);
			else if (type == TOKEN_BACKTICK) new_node = create_ast_node(NODE_TEXT, "`", NULL);
			else if (type == TOKEN_EXCLAMATION) new_node = create_ast_node(NODE_TEXT, "!", NULL);
			else if (type == TOKEN_GREATER_THAN) new_node = create_ast_node(NODE_TEXT, ">", NULL);
			else if (type == TOKEN_BACKSLASH) new_node = create_ast_node(NODE_TEXT, "\\", NULL);
		}
		if (new_node) add_child_node(parent_node, new_node);
	}
}

AstNode* parse_paragraph(ParserState* p) {
	AstNode* paragraph_node = create_ast_node(NODE_PARAGRAPH, NULL, NULL);

	parse_inline_elements(p, paragraph_node, false);

	return paragraph_node;
}

AstNode* parse_block(ParserState* p) {
	Token* current = peek_token(p);
	if (!current) return NULL;

	struct list_head* start_pos = p->current_node;

	int indent = count_indent(p);
	if (indent > 0) {
		return parse_paragraph(p);
	}

	switch (current->type) {
		case TOKEN_HASH: {
			AstNode* node = parse_heading(p);
			if (node) return node;
			break;
		}
		case TOKEN_DASH:
		case TOKEN_ASTERISK: {
			AstNode* node = parse_line(p);
			if (node) return node;

			p->current_node = start_pos;
			return parse_list(p, 0);
		}
		case TOKEN_NUMBER:
			return parse_list(p, 0);
		case TOKEN_BACKTICK: {
			AstNode* node = parse_code_block(p);
			if (node) return node;
			break;
		}
		default:
			break;
	}

	p->current_node = start_pos;
	return parse_paragraph(p);
}

AstNode* parse_tokens(struct list_head* token_head) {
	ParserState p_state;
	p_state.head = token_head;
	p_state.current_node = token_head;

	AstNode* doc_node = create_ast_node(NODE_DOCUMENT, NULL, NULL);

	while (peek_token(&p_state) && peek_token(&p_state)->type != TOKEN_EOF) {

		while(peek_token(&p_state) && peek_token(&p_state)->type == TOKEN_NEWLINE) {
			consume_token(&p_state);
		}

		if (!peek_token(&p_state) || peek_token(&p_state)->type == TOKEN_EOF) {
			break;
		}

		AstNode* block = parse_block(&p_state);
		if (block) {
			add_child_node(doc_node, block);
		}
	}
	return doc_node;
}

void free_ast(AstNode* root) {
	if (!root) return;

	if (!list_empty(&root->children)) {
		AstNode *child, *temp;
		list_for_each_entry_safe(child, temp, &root->children, list) {
			list_del(&child->list);
			free_ast(child);
		}
	}

	if (root->data1) {
		free(root->data1);
	}

	if (root->data2) {
		free(root->data2);
	}

	free(root);
}

