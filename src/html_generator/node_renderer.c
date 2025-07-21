#include <string.h>

#include "node_renderer.h"

static void append_escaped_html(DynamicBuffer* buffer, const char* text) {
	if (!text) return;
	for (size_t i = 0; i < strlen(text); i++) {
		switch (text[i]) {
			case '<':
				buffer_append_formatted(buffer, "&lt;");
				break;
			case '>':
				buffer_append_formatted(buffer, "&gt;");
				break;
			case '&':
				buffer_append_formatted(buffer, "&amp;");
				break;
			default:
				buffer_append_formatted(buffer, "%c", text[i]);
				break;
		}
	}
}

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
			append_escaped_html(buffer, node->data1);
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

void render_inline_node(const AstNode* node, DynamicBuffer* buffer, TemplateContext* context) {
	switch (node->type) {
		case NODE_TEXT:             buffer_append_formatted(buffer, "%s", node->data1); break;
		case NODE_ITALIC:           buffer_append_formatted(buffer, "<em>%s</em>", node->data1); break;
		case NODE_BOLD:             buffer_append_formatted(buffer, "<strong>%s</strong>", node->data1); break;
		case NODE_ITALIC_AND_BOLD:  buffer_append_formatted(buffer, "<em><strong>%s</strong></em>", node->data1); break;
		case NODE_CODE:
			buffer_append_formatted(buffer, "<code>");
			append_escaped_html(buffer, node->data1);
			buffer_append_formatted(buffer, "</code>");
			break;
		case NODE_LINK: {
			const char* url = node->data2;
			const char* base_url = get_from_context(context, "base_url");
			if (!base_url) base_url = "";

			if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
				buffer_append_formatted(buffer, "<a href=\"%s\">%s</a>", url, node->data1);
			} else {
				buffer_append_formatted(buffer, "<a href=\"%s%s\">%s</a>", base_url, url, node->data1);
			}
			break;
		}
		case NODE_IMAGE_LINK: {
			const char* src = node->data2;
			const char* base_url = get_from_context(context, "base_url");
			if (!base_url) base_url = "";

			if (strncmp(src, "http://", 7) == 0 || strncmp(src, "https://", 8) == 0) {
				buffer_append_formatted(buffer, "<img src=\"%s\" alt=\"%s\">", src, node->data1);
			} else {
				buffer_append_formatted(buffer, "<img src=\"%s%s\" alt=\"%s\">", base_url, src, node->data1);
			}
			break;
		}
		case NODE_SOFT_BREAK: {
			const char* hard_breaks = get_from_context(context, "hard_line_breaks");

			if (hard_breaks && strcmp(hard_breaks, "true") == 0) {
				buffer_append_formatted(buffer, "<br>\n");
			} else {
				buffer_append_formatted(buffer, " ");
			}
			break;
		}
		default: break;
	}
}

void render_self_closing_node(const AstNode* node, DynamicBuffer* buffer) {
	switch (node->type) {
		case NODE_LINE: buffer_append_formatted(buffer, "<hr>\n"); break;
		default: break;
	}
}
