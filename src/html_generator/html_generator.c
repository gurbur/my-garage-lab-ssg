#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/html_generator.h"
#include "../include/dynamic_buffer.h"
#include "node_renderer.h"

static bool is_inline_node(const AstNode* node) {
	switch (node->type) {
		case NODE_TEXT:
		case NODE_ITALIC:
		case NODE_BOLD:
		case NODE_ITALIC_AND_BOLD:
		case NODE_CODE:
		case NODE_LINK:
		case NODE_IMAGE_LINK:
			return true;
		default:
			return false;
	}
}

static void render_node_recursively(const AstNode* node, DynamicBuffer* buffer);

char* generate_html_from_ast(AstNode* ast_root) {
	if (!ast_root) return NULL;

	DynamicBuffer* buffer = create_dynamic_buffer(4096);
	if (!buffer) {
		perror("Failed to create dynamic buffer");
		return NULL;
	}

	render_node_recursively(ast_root, buffer);

	return destroy_buffer_and_get_content(buffer);
}

static void render_node_recursively(const AstNode* node, DynamicBuffer* buffer) {
	if (!node) return;

	if (node->type == NODE_DOCUMENT) {
		// buh.
	} else if (node->type == NODE_LINE) {
		render_self_closing_node(node, buffer);
	} else if (is_inline_node(node)) {
		render_inline_node(node, buffer);
	} else {
		render_opening_tag_for_node(node, buffer);
	}

	if (!list_empty(&node->children)) {
		AstNode* child;
		list_for_each_entry(child, &node->children, list) {
			render_node_recursively(child, buffer);
		}
	}

	if (node->type < NODE_TEXT) {
		render_closing_tag_for_node(node, buffer);
	}
}
