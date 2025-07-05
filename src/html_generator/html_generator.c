#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../include/html_generator.h"

typedef struct {
	char *buffer;
	size_t len;
	size_t capacity;
} HtmlBuffer;

static void buffer_append(HtmlBuffer *hb, const char *s) {
	size_t s_len = strlen(s);
	if (hb->len + s_len + 1 > hb->capacity) {
		size_t new_cap = hb->capacity * 2;
		if (new_cap < hb->len + s_len + 1) {
			new_cap = hb->len + s_len + 1;
		}
		hb->buffer = (char *)realloc(hb->buffer, new_cap);
		hb->capacity = new_cap;
	}
	strcat(hb->buffer, s);
	hb->len += s_len;
}

static void buffer_append_format(HtmlBuffer *hb, const char *format, ...) {
	va_list args1;
	va_start(args1, format);

	va_list args2;
	va_copy(args2, args1);

	int len = vsnprintf(NULL, 0, format, args1);
	va_end(args1);

	if (len < 0) {
		va_end(args2);
		return;
	}

	if (hb->len + len + 1 > hb->capacity) {
		size_t new_cap = hb->capacity;
		while(hb->len + len + 1 > new_cap) {
			new_cap *= 2;
		}
		hb->buffer = (char *)realloc(hb->buffer, new_cap);
		hb->capacity = new_cap;
	}

	vsnprintf(hb->buffer + hb->len, len + 1, format, args2);
	va_end(args2);

	hb->len += len;
}

static void generate_node_html(AstNode* node, HtmlBuffer* hb);

char* generate_html(AstNode* root) {
	if (!root) return NULL;

	HtmlBuffer hb;
	hb.capacity = 2048;
	hb.buffer = (char*)malloc(hb.capacity);
	if (!hb.buffer) {
		perror("Failed to allocate memory for HTML buffer");
		return NULL;
	}
	hb.buffer[0] = '\0';
	hb.len = 0;

	generate_node_html(root, &hb);

	return hb.buffer;
}

static void generate_inline_html(AstNode* node, HtmlBuffer* hb) {
	switch (node->type) {
		case NODE_TEXT:
			buffer_append_format(hb, "%s", node->data1);
			break;
		case NODE_ITALIC:
			buffer_append_format(hb, "<em>%s</em>", node->data1);
			break;
		case NODE_BOLD:
			buffer_append_format(hb, "<strong>%s</strong>", node->data1);
			break;
		case NODE_ITALIC_AND_BOLD:
			buffer_append_format(hb, "<em><strong>%s</strong></em>", node->data1);
			break;
		case NODE_CODE:
			buffer_append_format(hb, "<code>%s</code>", node->data1);
			break;
		case NODE_LINK:
			buffer_append_format(hb, "<a href=\"%s\">%s</a>", node->data2, node->data1);
			break;
		case NODE_IMAGE_LINK:
			buffer_append_format(hb, "<img src=\"%s\" alt=\"%s\">", node->data1, node->data1);
			break;
		default:
			break;
	}
}

static void generate_node_html(AstNode* node, HtmlBuffer* hb) {
	if (!node) return;

	switch (node->type) {
		case NODE_DOCUMENT:
			break;
		case NODE_HEADING1:
			buffer_append_format(hb, "<h1>%s", node->data1);
			break;
		case NODE_HEADING2:
			buffer_append_format(hb, "<h2>%s", node->data1);
			break;
		case NODE_HEADING3:
			buffer_append_format(hb, "<h3>%s", node->data1);
			break;
		case NODE_PARAGRAPH:
			buffer_append_format(hb, "<p>");
			break;
		case NODE_CODE_BLOCK:
			if (node->data2) {
				buffer_append_format(hb, "<pre><code class=\"language-%s\">", node->data2);
			} else {
				buffer_append_format(hb, "<pre><code>");
			}
			if (node->data1) buffer_append_format(hb, "%s", node->data1);
			break;
		case NODE_LINE:
			buffer_append_format(hb, "<hr>\n");
			return;
		case NODE_ORDERED_LIST:
			buffer_append_format(hb, "<ol>\n");
			break;
		case NODE_UNORDERED_LIST:
			buffer_append_format(hb, "<ul>\n");
			break;
		case NODE_LIST_ITEM:
			buffer_append_format(hb, "<li>");
			break;
		default:
			generate_inline_html(node, hb);
			return;
	}

	if (!list_empty(&node->children)) {
		AstNode* child;
		list_for_each_entry(child, &node->children, list) {
			generate_node_html(child, hb);
		}
	}

	switch (node->type) {
		case NODE_HEADING1:
			buffer_append_format(hb, "</h1>\n");
			break;
		case NODE_HEADING2:
			buffer_append_format(hb, "</h2>\n");
			break;
		case NODE_HEADING3:
			buffer_append_format(hb, "</h3>\n");
			break;
		case NODE_PARAGRAPH:
			buffer_append_format(hb, "</p>\n");
			break;
		case NODE_CODE_BLOCK:
			if (node->data2) {
				buffer_append_format(hb, "</code></pre>\n");
			} else {
				buffer_append_format(hb, "</code></pre>\n");
			}
			break;
		case NODE_ORDERED_LIST:
			buffer_append_format(hb, "</ol>\n");
			break;
		case NODE_UNORDERED_LIST:
			buffer_append_format(hb, "</ul>\n");
			break;
		case NODE_LIST_ITEM:
			buffer_append_format(hb, "</li>\n");
			break;
		default:
			break;
	}
}

