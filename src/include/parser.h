#pragma once

#include "list_head.h"
#include "tokenizer.h"

typedef enum {
	NODE_DOCUMENT = 0,
	NODE_HEADING1,
	NODE_HEADING2,
	NODE_HEADING3,
	NODE_PARAGRAPH,
	NODE_CODE_BLOCK,
	NODE_LINE,
	NODE_ORDERED_LIST,
	NODE_UNORDERED_LIST,
	NODE_LIST_ITEM,
	NODE_IMAGE_LINK,
	NODE_TEXT,
	NODE_ITALIC,
	NODE_BOLD,
	NODE_ITALIC_AND_BOLD,
	NODE_CODE,
	NODE_LINK,
} AstNodeType;

typedef struct {
	struct list_head list;
	struct list_head children;
	
	AstNodeType type;
	char* data1; // main data: string data, code block, etc.
	char* data2; // sub data: code lang, link, etc.
} AstNode;


AstNode* parse_tokens(struct list_head* tokens);
void free_ast(AstNode* root);

