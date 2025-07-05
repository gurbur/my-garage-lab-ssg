#include <stdio.h>
#include "../include/html_generator.h"

static void generate_node_html(AstNode* node, FILE* file);

void generate_html(AstNode* root, const char* output_filename) {
	FILE* file = fopen(output_filename, "w");
	if (!file) {
		perror("unable to read HTML file");
		return ;
	}

	generate_node_html(root, file);

	fclose(file);
}

static void generate_inline_html(AstNode* node, FILE* file) {
	switch (node->type) {
		case NODE_TEXT:
			fprintf(file, "%s", node->data1);
			break;
		case NODE_ITALIC:
			fprintf(file, "<em>%s</em>", node->data1);
			break;
		case NODE_BOLD:
			fprintf(file, "<strong>%s</strong>", node->data1);
			break;
		case NODE_ITALIC_AND_BOLD:
			fprintf(file, "<em><strong>%s</strong></em>", node->data1);
			break;
		case NODE_CODE:
			fprintf(file, "<code>%s</code>", node->data1);
			break;
		case NODE_LINK:
			fprintf(file, "<a href=\"%s\">%s</a>", node->data2, node->data1);
			break;
		case NODE_IMAGE_LINK:
			fprintf(file, "<img src=\"%s\" alt=\"%s\">", node->data1, node->data1);
			break;
		default:
			break;
	}
}

static void generate_node_html(AstNode* node, FILE* file) {
	if (!node) return;

	switch (node->type) {
		case NODE_DOCUMENT:
			break;
		case NODE_HEADING1:
			fprintf(file, "<h1>");
			break;
		case NODE_HEADING2:
			fprintf(file, "<h2>");
			break;
		case NODE_HEADING3:
			fprintf(file, "<h3>");
			break;
		case NODE_PARAGRAPH:
			fprintf(file, "<p>");
			break;
		case NODE_CODE_BLOCK:
			if (node->data2) {
				fprintf(file, "<pre><code class=\"language-%s\">", node->data2);
			} else {
				fprintf(file, "<pre><code>");
			}
			if (node->data1) fprintf(file, "%s", node->data1);
			break;
		case NODE_LINE:
			fprintf(file, "<hr>\n");
			return;
		case NODE_ORDERED_LIST:
			fprintf(file, "<ol>\n");
			break;
		case NODE_UNORDERED_LIST:
			fprintf(file, "<ul>\n");
			break;
		case NODE_LIST_ITEM:
			fprintf(file, "<li>");
			break;
		default:
			generate_inline_html(node, file);
			return;
	}

	if (!list_empty(&node->children)) {
		AstNode* child;
		list_for_each_entry(child, &node->children, list) {
			generate_node_html(child, file);
		}
	}

	switch (node->type) {
		case NODE_HEADING1:
			fprintf(file, "</h1>\n");
			break;
		case NODE_HEADING2:
			fprintf(file, "</h2>\n");
			break;
		case NODE_HEADING3:
			fprintf(file, "</h3>\n");
			break;
		case NODE_CODE_BLOCK:
			if (node->data2) {
				fprintf(file, "</code></pre>\n");
			} else {
				fprintf(file, "</code></pre>\n");
			}
			break;
		case NODE_ORDERED_LIST:
			fprintf(file, "</ol>\n");
			break;
		case NODE_UNORDERED_LIST:
			fprintf(file, "</li>\n");
			break;
		default:
			break;
	}
}

