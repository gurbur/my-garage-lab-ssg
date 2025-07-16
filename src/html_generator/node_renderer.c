#include "node_renderer.h"

void render_opening_tag_for_node(const AstNode* node, DynamicBuffer* buffer) {
	switch (node->type) {
		case NODE_HEADING1:         buffer_append_formatted(buffer, "<h1>%s", node->data1); break;
		case NODE_HEADING2:         buffer_append_formatted(buffer, "<h2>%s", node->data1); break;
		case NODE_HEADING3:         buffer_append_formatted(buffer, "<h3>%s", node->data1); break;
		case NODE_PARAGRAPH:        buffer_append_formatted(buffer, "<p>"); break;
		case NODE_ORDERED_LIST:     buffer_append_formatted(buffer, "<ol>\n"); break;
		case NODE_UNORDERED_LIST:   buffer_append_formatted(buffer, "<ul>\n"); break;
		case NODE_LIST_ITEM:        buffer_append_formatted(buffer, "<li>"); break;
		case NODE_CODE_BLOCK:
			if (node->data2) {
				buffer_append_formatted(buffer, "<pre><code class=\"language-%s\">", node->data2);
			} else {
				buffer_append_formatted(buffer, "<pre><code>");
			}
			if (node->data1) buffer_append_formatted(buffer, "%s", node->data1);
			break;
		default: break;
	}
}

void render_closing_tag_for_node(const AstNode* node, DynamicBuffer* buffer) {
	switch (node->type) {
		case NODE_HEADING1:         buffer_append_formatted(buffer, "</h1>\n"); break;
		case NODE_HEADING2:         buffer_append_formatted(buffer, "</h2>\n"); break;
		case NODE_HEADING3:         buffer_append_formatted(buffer, "</h3>\n"); break;
		case NODE_PARAGRAPH:        buffer_append_formatted(buffer, "</p>\n"); break;
		case NODE_ORDERED_LIST:     buffer_append_formatted(buffer, "</ol>\n"); break;
		case NODE_UNORDERED_LIST:   buffer_append_formatted(buffer, "</ul>\n"); break;
		case NODE_LIST_ITEM:        buffer_append_formatted(buffer, "</li>\n"); break;
		case NODE_CODE_BLOCK:       buffer_append_formatted(buffer, "</code></pre>\n"); break;
		default: break;
	}
}

void render_inline_node(const AstNode* node, DynamicBuffer* buffer) {
	switch (node->type) {
		case NODE_TEXT:             buffer_append_formatted(buffer, "%s", node->data1); break;
		case NODE_ITALIC:           buffer_append_formatted(buffer, "<em>%s</em>", node->data1); break;
		case NODE_BOLD:             buffer_append_formatted(buffer, "<strong>%s</strong>", node->data1); break;
		case NODE_ITALIC_AND_BOLD:  buffer_append_formatted(buffer, "<em><strong>%s</strong></em>", node->data1); break;
		case NODE_CODE:             buffer_append_formatted(buffer, "<code>%s</code>", node->data1); break;
		case NODE_LINK:             buffer_append_formatted(buffer, "<a href=\"%s\">%s</a>", node->data2, node->data1); break;
		case NODE_IMAGE_LINK:       buffer_append_formatted(buffer, "<img src=\"%s\" alt=\"%s\">", node->data2, node->data1); break;
		default: break;
	}
}

void render_self_closing_node(const AstNode* node, DynamicBuffer* buffer) {
	switch (node->type) {
		case NODE_LINE: buffer_append_formatted(buffer, "<hr>\n"); break;
		default: break;
	}
}
