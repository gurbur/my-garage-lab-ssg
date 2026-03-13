#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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
		case NODE_MATH:
		case NODE_SOFT_BREAK:
			return true;
		default:
			return false;
	}
}

static void render_node_recursively(const AstNode* node, DynamicBuffer* buffer, TemplateContext* context);

static void build_toc_recursively(const AstNode* node, DynamicBuffer* buffer) {
	if (!node) return;

	if (node->type == NODE_HEADING1 || node->type == NODE_HEADING2 || node->type == NODE_HEADING3) {
		char* anchor_id = generate_anchor_id(node->data1);
		int level = (node->type == NODE_HEADING1) ? 1 : ((node->type == NODE_HEADING2) ? 2 : 3);

		int margin_left = (level - 1) * 15;

		buffer_append_formatted(buffer,
				"<li class=\"toc-level-%d\" style=\"margin-left: %dpx;\"><a href=\"#%s\">%s</a></li>\n",
				level, margin_left, anchor_id, node->data1);

		free(anchor_id);
	}

	if (!list_empty(&node->children)) {
		AstNode* child;
		list_for_each_entry(child, &node->children, list) {
			build_toc_recursively(child, buffer);
		}
	}
}

char* generate_toc_from_ast(AstNode* ast_root) {
	if (!ast_root) return NULL;

	DynamicBuffer* buffer = create_dynamic_buffer(1024);
	buffer_append_formatted(buffer, "<ul class=\"toc-list\">\n");

	build_toc_recursively(ast_root, buffer);
	buffer_append_formatted(buffer, "</ul>\n");

	if (buffer->length <= 25) {
		destroy_buffer_and_get_content(buffer);
		return strdup("");
	}

	return destroy_buffer_and_get_content(buffer);
}

char* generate_html_from_ast(AstNode* ast_root, TemplateContext* context) {
	if (!ast_root) return NULL;

	DynamicBuffer* buffer = create_dynamic_buffer(4096);
	if (!buffer) {
		perror("Failed to create dynamic buffer");
		return NULL;
	}

	render_node_recursively(ast_root, buffer, context);

	return destroy_buffer_and_get_content(buffer);
}

static void render_node_recursively(const AstNode* node, DynamicBuffer* buffer, TemplateContext* context) {
	if (!node) return;

	if (node->type == NODE_DOCUMENT) {
		// buh.
	} else if (node->type == NODE_LINE) {
		render_self_closing_node(node, buffer);
	} else if (is_inline_node(node)) {
		render_inline_node(node, buffer, context);
	} else {
		render_opening_tag_for_node(node, buffer);
	}

	if (!list_empty(&node->children)) {
		AstNode* child;
		list_for_each_entry(child, &node->children, list) {
			render_node_recursively(child, buffer, context);
		}
	}

	if (node->type < NODE_TEXT) {
		render_closing_tag_for_node(node, buffer);
	}
}
