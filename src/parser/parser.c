#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../include/list_head.h"
#include "../include/parser.h"

// =============================================================================

typedef struct {
    struct list_head* current_node;
    struct list_head* head;
} ParserState;

static AstNode* create_ast_node(AstNodeType type, const char* data1, const char* data2);
static void add_child_node(AstNode* parent, AstNode* child);
static Token* peek_token(ParserState* p);
static Token* consume_token(ParserState* p);
static bool match_token(ParserState* p, TokenType type);
static const char* token_to_string(const Token* token);
static void append_string_to_buffer(char** buffer, int* index, int* capacity, const char* str);
static void flush_buffer_if_needed(AstNode* parent_node, char* buffer, int* index_ptr);

static int calculate_indent(ParserState* p);
static void consume_indent(ParserState* p);

static AstNode* parse_block(ParserState* p);
static AstNode* parse_paragraph(ParserState* p);
static AstNode* parse_heading(ParserState* p);
static AstNode* parse_line(ParserState* p);
static AstNode* parse_code_block(ParserState* p);
static AstNode* parse_list(ParserState* p, int expected_indent);
static AstNode* parse_list_item(ParserState* p, int item_indent);

static void parse_inline_elements(ParserState* p, AstNode* parent_node, bool is_list_item);
static AstNode* parse_emphasis(ParserState* p);
static AstNode* parse_inline_code(ParserState* p);
static AstNode* parse_standard_link(ParserState* p);
static AstNode* parse_obsidian_link(ParserState* p, bool is_image);


static AstNode* create_ast_node(AstNodeType type, const char* data1, const char* data2) {
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
    if (parent && child) {
        list_add_tail(&child->list, &parent->children);
    }
}

static Token* peek_token(ParserState* p) {
    if (!p || !p->current_node || p->current_node->next == p->head) return NULL;
    return list_entry(p->current_node->next, Token, list);
}

static Token* consume_token(ParserState* p) {
    if (!p || !p->current_node || p->current_node->next == p->head) return NULL;
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

static int calculate_indent(ParserState* p) {
    int indent = 0;
    struct list_head* temp_node = p->current_node->next;
    while (temp_node != p->head) {
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

static void consume_indent(ParserState* p) {
    while (peek_token(p) && peek_token(p)->type == TOKEN_TAB) {
        consume_token(p);
    }
}

static const char* token_to_string(const Token* token) {
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

static void append_string_to_buffer(char** buffer, int* index, int* capacity, const char* str) {
    if (!str) return;
    size_t str_len = strlen(str);
    while (*index + str_len >= (size_t)*capacity) {
        *capacity *= 2;
        char* new_buffer = realloc(*buffer, *capacity);
        if (!new_buffer) {
            perror("Failed to reallocate buffer");
            free(*buffer);
            exit(EXIT_FAILURE);
        }
        *buffer = new_buffer;
    }
    strcat(*buffer, str);
    *index += str_len;
}

static void flush_buffer_if_needed(AstNode* parent_node, char* buffer, int* index_ptr) {
    if (*index_ptr > 0) {
        buffer[*index_ptr] = '\0';
        add_child_node(parent_node, create_ast_node(NODE_TEXT, buffer, NULL));
        *index_ptr = 0;
        buffer[0] = '\0';
    }
}


// =============================================================================

void parse_inline_elements(ParserState* p, AstNode* parent_node, bool is_list_item) {
    int capacity = 256;
    int index = 0;
    char* text_buffer = malloc(capacity);
    text_buffer[0] = '\0';

    while (peek_token(p) && peek_token(p)->type != TOKEN_EOF) {
        Token* t1 = peek_token(p);

        if (t1->type == TOKEN_NEWLINE) {
            if (is_list_item) {
                break;
            } else {
                Token* t2 = (t1->list.next != p->head) ? list_entry(t1->list.next, Token, list) : NULL;
                if (t2 && t2->type == TOKEN_NEWLINE) {
                    break;
                }
            }
        }
        
        AstNode* new_node = NULL;
        if (t1->type == TOKEN_ASTERISK) new_node = parse_emphasis(p);
        else if (t1->type == TOKEN_BACKTICK) new_node = parse_inline_code(p);
        else if (t1->type == TOKEN_EXCLAMATION) new_node = parse_obsidian_link(p, true);
        else if (t1->type == TOKEN_LBRACKET) {
            Token* lookahead = (t1->list.next != p->head) ? list_entry(t1->list.next, Token, list) : NULL;
            if (lookahead && lookahead->type == TOKEN_LBRACKET) new_node = parse_obsidian_link(p, false);
            else new_node = parse_standard_link(p);
        }
        
        if (new_node) {
            flush_buffer_if_needed(parent_node, text_buffer, &index);
            add_child_node(parent_node, new_node);
            continue;
        }

        Token* current_token = consume_token(p);
        if (current_token->type == TOKEN_NEWLINE) {
             append_string_to_buffer(&text_buffer, &index, &capacity, " ");
        } else {
             append_string_to_buffer(&text_buffer, &index, &capacity, token_to_string(current_token));
        }
    }
    flush_buffer_if_needed(parent_node, text_buffer, &index);
    free(text_buffer);
}

AstNode* parse_list_item(ParserState* p, int item_indent) {
    AstNode* item_node = create_ast_node(NODE_LIST_ITEM, NULL, NULL);
    consume_token(p);
    if (peek_token(p) && peek_token(p)->type == TOKEN_DOT) {
        consume_token(p);
    }

    parse_inline_elements(p, item_node, true);

    match_token(p, TOKEN_NEWLINE);

    int next_indent = calculate_indent(p);
    if (next_indent > item_indent) {
        AstNode* sub_list = parse_list(p, next_indent);
        if (sub_list) {
            add_child_node(item_node, sub_list);
        }
    }
    return item_node;
}


AstNode* parse_list(ParserState* p, int expected_indent) {
    struct list_head* temp_pos = p->current_node->next;
    for (int i = 0; i < expected_indent; i++) {
        if (temp_pos == p->head) return NULL;
        temp_pos = temp_pos->next;
    }
    if (temp_pos == p->head) return NULL;
    Token* start_token = list_entry(temp_pos, Token, list);

    AstNodeType list_type;
    if (start_token->type == TOKEN_DASH || start_token->type == TOKEN_ASTERISK) {
        list_type = NODE_UNORDERED_LIST;
    } else if (start_token->type == TOKEN_NUMBER) {
        list_type = NODE_ORDERED_LIST;
    } else {
        return NULL;
    }

    AstNode* list_node = create_ast_node(list_type, NULL, NULL);

    while (peek_token(p)) {
        int current_indent = calculate_indent(p);
        if (current_indent != expected_indent) {
            break;
        }

        struct list_head* marker_pos = p->current_node->next;
        for (int i = 0; i < current_indent; i++) {
            if (marker_pos == p->head) return list_node;
            marker_pos = marker_pos->next;
        }
        if (marker_pos == p->head) return list_node;
        Token* marker = list_entry(marker_pos, Token, list);

        bool type_match = (list_type == NODE_UNORDERED_LIST && (marker->type == TOKEN_DASH || marker->type == TOKEN_ASTERISK)) ||
                          (list_type == NODE_ORDERED_LIST && marker->type == TOKEN_NUMBER);

        if (type_match) {
            consume_indent(p);
            AstNode* item_node = parse_list_item(p, current_indent);
            add_child_node(list_node, item_node);
        } else {
            break;
        }
    }
    return list_node;
}

AstNode* parse_paragraph(ParserState* p) {
    AstNode* paragraph_node = create_ast_node(NODE_PARAGRAPH, NULL, NULL);
    parse_inline_elements(p, paragraph_node, false);
    return paragraph_node;
}

AstNode* parse_block(ParserState* p) {
    struct list_head* start_pos = p->current_node;
    int indent = calculate_indent(p);

    struct list_head* temp_node = p->current_node->next;
    for (int i = 0; i < indent; ++i) {
        if (temp_node != p->head) {
            temp_node = temp_node->next;
        } else {
            return parse_paragraph(p);
        }
    }

    if (temp_node == p->head) return NULL;
    Token* start_token = list_entry(temp_node, Token, list);

    switch (start_token->type) {
        case TOKEN_HASH:
            consume_indent(p);
            return parse_heading(p);

        case TOKEN_DASH:
        case TOKEN_ASTERISK: {
            consume_indent(p);
            AstNode* line_node = parse_line(p);
            if (line_node) return line_node;

            p->current_node = start_pos;

            if (calculate_indent(p) == indent) {
                if (temp_node->next != p->head) {
                    Token* after_marker = list_entry(temp_node->next, Token, list);
                    if (after_marker->type == TOKEN_TEXT && after_marker->value && after_marker->value[0] == ' ') {
                        return parse_list(p, indent);
                    }
                }
            }
            break;
        }

        case TOKEN_NUMBER: {
            if (calculate_indent(p) == indent) {
                 return parse_list(p, indent);
            }
            break;
        }
        
        case TOKEN_BACKTICK: {
            consume_indent(p);
            AstNode* code_node = parse_code_block(p);
            if (code_node) return code_node;
            
            p->current_node = start_pos;
            break;
        }
    }

    p->current_node = start_pos;
    return parse_paragraph(p);
}
// =============================================================================

AstNode* parse_tokens(struct list_head* token_head) {
    ParserState p_state;
    p_state.head = token_head;
    p_state.current_node = token_head;

    AstNode* doc_node = create_ast_node(NODE_DOCUMENT, NULL, NULL);

    while (peek_token(&p_state)) {
        if (peek_token(&p_state)->type == TOKEN_NEWLINE) {
            consume_token(&p_state);
            continue;
        }
        if (peek_token(&p_state)->type == TOKEN_EOF) {
            break;
        }

        AstNode* block = parse_block(&p_state);
        if (block) {
            bool is_empty_list = (block->type == NODE_ORDERED_LIST || block->type == NODE_UNORDERED_LIST) && list_empty(&block->children);
            if (is_empty_list) {
                free_ast(block);
            } else {
                add_child_node(doc_node, block);
            }
        } else {
            if (peek_token(&p_state)) consume_token(&p_state);
        }
    }
    return doc_node;
}

void free_ast(AstNode* root) {
    if (!root) return;
    struct list_head *pos, *q;
    AstNode *entry;
    list_for_each_safe(pos, q, &root->children) {
        entry = list_entry(pos, AstNode, list);
        free_ast(entry);
    }
    free(root->data1);
    free(root->data2);
    free(root);
}

// -----------------------------------------------------------------------------

AstNode* parse_heading(ParserState* p) {
    int level = 0;
    struct list_head* start_pos = p->current_node;
    while (peek_token(p) && peek_token(p)->type == TOKEN_HASH) {
        level++;
        consume_token(p);
    }

    Token* first_text = peek_token(p);
    if (!first_text || first_text->type != TOKEN_TEXT || !first_text->value || first_text->value[0] != ' ') {
        p->current_node = start_pos;
        return NULL;
    }

    int capacity = 256;
    int index = 0;
    char* buffer = malloc(capacity);
    buffer[0] = '\0';

    consume_token(p);
    if (strlen(first_text->value) > 1) {
        append_string_to_buffer(&buffer, &index, &capacity, first_text->value + 1);
    }

    while (peek_token(p) && peek_token(p)->type != TOKEN_NEWLINE) {
        Token* current = consume_token(p);
        append_string_to_buffer(&buffer, &index, &capacity, token_to_string(current));
    }

    AstNodeType heading_type = (level == 1) ? NODE_HEADING1 : (level == 2) ? NODE_HEADING2 : NODE_HEADING3;
    AstNode* heading_node = create_ast_node(heading_type, buffer, NULL);
    free(buffer);
    return heading_node;
}

AstNode* parse_line(ParserState* p) {
    struct list_head* start_pos = p->current_node;
    Token* t1 = peek_token(p);
    if (!t1 || (t1->type != TOKEN_DASH && t1->type != TOKEN_ASTERISK)) return NULL;

    TokenType line_type = t1->type;
    int count = 0;

    while (peek_token(p) && peek_token(p)->type == line_type) {
        count++;
        consume_token(p);
    }

    Token* final_token = peek_token(p);
    if (count >= 3 && (final_token == NULL || final_token->type == TOKEN_NEWLINE || final_token->type == TOKEN_EOF)) {
        return create_ast_node(NODE_LINE, NULL, NULL);
    }
    
    p->current_node = start_pos;
    return NULL;
}

AstNode* parse_standard_link(ParserState* p) {
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

AstNode* parse_obsidian_link(ParserState* p, bool is_image) {
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
        if (t1->list.next != p->head) {
		    Token* t2 = list_entry(t1->list.next, Token, list);
		    if (t1->type == TOKEN_RBRACKET && t2 && t2->type == TOKEN_RBRACKET) break;
        } else {
            break;
        }
		Token* current = consume_token(p);
		append_string_to_buffer(&filename_buffer, &index, &capacity, token_to_string(current));
	}

	if (match_token(p, TOKEN_RBRACKET) && match_token(p, TOKEN_RBRACKET)) {
		AstNode* link_node = create_ast_node(is_image ? NODE_IMAGE_LINK : NODE_LINK, filename_buffer, is_image ? NULL : filename_buffer);
		free(filename_buffer);
		return link_node;
	}
	free(filename_buffer);
	p->current_node = start_pos;
	return NULL;
}

AstNode* parse_code_block(ParserState* p) {
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

AstNode* parse_inline_code(ParserState* p) {
	struct list_head* start_pos = p->current_node;
	if (!peek_token(p) || peek_token(p)->type != TOKEN_BACKTICK) return NULL;
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

AstNode* parse_emphasis(ParserState* p) {
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
			AstNodeType type = (level == 1) ? NODE_ITALIC : (level == 2) ? NODE_BOLD : NODE_ITALIC_AND_BOLD;
			return create_ast_node(type, text_token->value, NULL);
		}
	}
	p->current_node = start_pos;
	return NULL;
}
