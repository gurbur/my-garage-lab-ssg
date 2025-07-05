#include <stdlib.h>
#include "parser_utils.h"
#include "block_parser.h"

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
            if (peek_token(&p_state)) {
                 consume_token(&p_state);
            }
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
    if(root->data1) free(root->data1);
    if(root->data2) free(root->data2);
    free(root);
}
