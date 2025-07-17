#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

#include "include/list_head.h"
#include "include/tokenizer.h"
#include "include/parser.h"
#include "include/html_generator.h"
#include "include/template_engine.h"
#include "include/dynamic_buffer.h"
#include "include/site_context.h"
#include "include/config_loader.h"

#define MAX_IGNORE_PATTERNS 100
#define MAX_PATH_LENGTH 1024

static char* ignore_patterns[MAX_IGNORE_PATTERNS];
static int ignore_count = 0;

void load_ssgignore(const char* base_path) {
	char ssgignore_path[MAX_PATH_LENGTH];
	snprintf(ssgignore_path, sizeof(ssgignore_path), "%s/.ssgignore", base_path);

	FILE* file = fopen(ssgignore_path, "r");
	if (!file) return;

	char line[MAX_PATH_LENGTH];
	while (fgets(line, sizeof(line), file) && ignore_count < MAX_IGNORE_PATTERNS) {
		line[strcspn(line, "\n")] = 0;
		if (strlen(line) > 0) {
			ignore_patterns[ignore_count++] = strdup(line);
		}
	}
	fclose(file);
}

int is_ignored(const char* path) {
	for (int i = 0; i < ignore_count; i++) {
		if (strstr(path, ignore_patterns[i]) == path) {
			return 1;
		}
	}
	return 0;
}

void free_ignore_patterns() {
	for (int i = 0; i < ignore_count; i++) {
		free(ignore_patterns[i]);
	}
}

static char* trim_whitespace(char* str) {
	while (isspace((unsigned char)*str)) str++;
	if (*str == 0) return str;

	char *end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	end[1] = '\0';
	return str;
}

static char* parse_front_matter(FILE* file, TemplateContext* context) {
	char line[MAX_PATH_LENGTH];
	fseek(file, 0, SEEK_SET);
	if (!fgets(line, sizeof(line), file) || strncmp(line, "---", 3) != 0) {
		fseek(file, 0, SEEK_SET);
		return NULL;
	}

	while (fgets(line, sizeof(line), file) && strncmp(line, "---", 3) != 0) {
		char* key = strtok(line, ":");
		char* value = strtok(NULL, "\n");
		if (key && value) {
			add_to_context(context, trim_whitespace(key), trim_whitespace(value));
		}
	}

	DynamicBuffer* db = create_dynamic_buffer(0);
	while (fgets(line, sizeof(line), file)) {
		buffer_append_formatted(db, "%s", line);
	}
	return destroy_buffer_and_get_content(db);
}

void copy_static_files(const char* src_dir, const char* dest_dir) {
	printf("Copying static files from %s to %s...\n", src_dir, dest_dir);
	//...
}

void process_file(const char* vault_path, NavNode* current_node, SiteContext* s_context, TemplateContext* global_context) {
	printf("Processing: %s\n", current_node->full_path);

	char full_input_path[MAX_PATH_LENGTH];
	snprintf(full_input_path, sizeof(full_input_path), "%s/%s", vault_path, current_node->full_path);

	FILE* md_file = fopen(full_input_path, "r");
	if (!md_file) {
		perror("Could not open makedown file");
		return;
	}

	TemplateContext* t_context = create_template_context();
	copy_context(t_context, global_context);

	char* content_md = parse_front_matter(md_file, t_context);
	fclose(md_file);

	if (!content_md) {
		md_file = fopen(full_input_path, "r");
		DynamicBuffer* db = create_dynamic_buffer(0);
		char line[MAX_PATH_LENGTH];
		while (fgets(line, sizeof(line), md_file)) {
			buffer_append_formatted(db, "%s", line);
		}
		fclose(md_file);
		content_md = destroy_buffer_and_get_content(db);
	}

	LIST_HEAD(token_list);
	tokenize_string(content_md, &token_list);
	AstNode* ast_root = parse_tokens(&token_list, s_context, current_node->full_path);
	char* content_html = generate_html_from_ast(ast_root);
	add_to_context(t_context, "post_content", content_html);

	generate_breadcrumb_html(current_node, t_context);

	const char* layout_key = get_from_context(t_context, "layout");
	const char* default_layout = get_from_context(global_context, "default_layout");
	char layout_path[MAX_PATH_LENGTH];
	snprintf(layout_path, sizeof(layout_path), "templates/layout/%s.html", (layout_key && *layout_key) ? layout_key : "post_page_layout");

	char* final_html = render_template(layout_path, t_context);

	if (final_html) {
		char full_output_path[MAX_PATH_LENGTH];
		snprintf(full_output_path, sizeof(full_output_path), "ssg_output/%s", current_node->output_path);

		char* out_dir = strdup(full_output_path);
		char* last_slash = strrchr(out_dir, '/');
		if (last_slash) {
			*last_slash = '\0';
			mkdir(out_dir, 0755);
		}
		free(out_dir);

		FILE* out_file = fopen(full_output_path, "w");
		if (out_file) {
			fprintf(out_file, "%s", final_html);
			fclose(out_file);
		}
	}

	free(content_md);
	free(content_html);
	free(final_html);
	free_ast(ast_root);
	free_template_context(t_context);
}

void process_nodes_recursively(const char* vault_path, NavNode* node, SiteContext* s_context, TemplateContext* global_context) {
	if (is_ignored(node->full_path)) {
		return;
	}

	if (node->is_directory) {
		char output_path[MAX_PATH_LENGTH];
		snprintf(output_path, sizeof(output_path), "ssg_output/%s", node->full_path);
		mkdir(output_path, 0755);

		NavNode* child;
		list_for_each_entry(child, &node->children, sibling) {
			process_nodes_recursively(vault_path, child, s_context, global_context);
		}
	} else if (strstr(node->name, ".md")) {
		process_file(vault_path, node, s_context, global_context);
	}
}

int main(int argc, char *argv[]) {
	const char* vault_path = ".";
	if (argc > 1) {
		vault_path = argv[1];
	}

	const char* output_dir = "ssg_output";

	printf("Starting SSG build for vault: %s\n", vault_path);

	TemplateContext* global_context = create_template_context();
	load_config("config.json", global_context);

	printf("Scanning vault and creating site map...\n");
	SiteContext* site_context = create_site_context(vault_path);
	printf("Site map created.\n");

	generate_sidebar_html(site_context, global_context);

	load_ssgignore(vault_path);

	mkdir(output_dir, 0755);

	process_nodes_recursively(vault_path, site_context->root, site_context, global_context);

	const char* static_dir = get_from_context(global_context, "static_dir");
	if (static_dir) {
		char dest_static_path[MAX_PATH_LENGTH];
		snprintf(dest_static_path, sizeof(dest_static_path), "%s/%s", output_dir, static_dir);
		copy_static_files(static_dir, dest_static_path);
	}

	free_site_context(site_context);
	free_template_context(global_context);
	free_ignore_patterns();

	printf("Build finished successfully!\n");

  return EXIT_SUCCESS;
}
