#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <stdbool.h>

#include "include/ignore_handler.h"
#include "include/list_head.h"
#include "include/tokenizer.h"
#include "include/parser.h"
#include "include/html_generator.h"
#include "include/template_engine.h"
#include "include/dynamic_buffer.h"
#include "include/site_context.h"
#include "include/config_loader.h"

#define MAX_PATH_LENGTH 1024

static char* trim_whitespace(char* str) {
	while (isspace((unsigned char)*str)) str++;
	if (*str == 0) return str;

	char *end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	end[1] = '\0';
	return str;
}

static char* read_file_into_string(const char* filepath) {
	FILE* file = fopen(filepath, "r");
	if (!file) return NULL;
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* buffer = malloc(length + 1);
	if (buffer) {
		fread(buffer, 1, length, file);
		buffer[length] = '\0';
	}
	fclose(file);
	return buffer;
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

static void create_directories_recursively(const char* path) {
	char tmp[MAX_PATH_LENGTH];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", path);
	len = strlen(tmp);

	if (tmp[len - 1] == '/') tmp[len - 1] = 0;

	char* last_slash = strrchr(tmp, '/');
	if (last_slash) {
		*last_slash = '\0';
	} else {
		return;
	}

	for (p = tmp; *p; p++) {
		if (*p == '/') {
			*p = 0;
			mkdir(tmp, 0755);
			*p = '/';
		}
	}
	mkdir(tmp, 0755);
}

void copy_static_files(const char* src_dir, const char* dest_dir) {
	printf("Copying static files from %s to %s...\n", src_dir, dest_dir);
	create_directories_recursively(dest_dir);

	DIR* dir = opendir(src_dir);
	if (!dir) return;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.') continue;

		char src_path[MAX_PATH_LENGTH];
		snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
		char dest_path[MAX_PATH_LENGTH];
		snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

		struct stat path_stat;
		stat(src_path, &path_stat);

		if (S_ISDIR(path_stat.st_mode)) {
			copy_static_files(src_path, dest_path);
		} else {
			FILE* src_file = fopen(src_path, "rb");
			FILE* dest_file = fopen(dest_path, "wb");
			if (src_file && dest_file) {
				char buffer[4096];
				size_t n;
				while ((n = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
					fwrite(buffer, 1, n, dest_file);
				}
				fclose(src_file);
				fclose(dest_file);
			}
		}
	}
	closedir(dir);
}

void process_file(const char* vault_path, NavNode* current_node, SiteContext* s_context, TemplateContext* global_context) {
	printf("Processing: %s\n", current_node->full_path);

	char full_input_path[MAX_PATH_LENGTH];
	snprintf(full_input_path, sizeof(full_input_path), "%s/%s", vault_path, current_node->full_path);

	FILE* md_file = fopen(full_input_path, "r");
	if (!md_file) {
		perror("	[ERROR] Could not open makedown file");
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
	char* content_html_partial = generate_html_from_ast(ast_root);
	add_to_context(t_context, "post_content", content_html_partial);

	generate_breadcrumb_html(current_node, t_context);

	if (get_from_context(t_context, "title") == NULL) {
		char* title_no_ext = strdup(current_node->name);
		char* dot = strrchr(title_no_ext, '.');
		if (dot) *dot = '\0';
		add_to_context(t_context, "title", title_no_ext);
		free(title_no_ext);
	}

	const char* layout_key = get_from_context(t_context, "layout");
	const char* default_layout = get_from_context(global_context, "default_layout");
	char layout_path[MAX_PATH_LENGTH];
	snprintf(layout_path, sizeof(layout_path), "templates/layout/%s.html", (layout_key && *layout_key) ? layout_key : (default_layout ? default_layout : "post_page_layout"));
	char* content_html_full = render_template(layout_path, t_context);

	add_to_context(t_context, "content", content_html_full);

	char* final_html = render_template("templates/layout/base.html", t_context);

	if (final_html) {
		char full_output_path[MAX_PATH_LENGTH];
		snprintf(full_output_path, sizeof(full_output_path), "ssg_output/%s", current_node->output_path);

		create_directories_recursively(full_output_path);

		FILE* out_file = fopen(full_output_path, "w");
		if (out_file) {
			fprintf(out_file, "%s", final_html);
			fclose(out_file);
			printf("[SUCCESS] Created: %s\n", full_output_path);
		} else {
			fprintf(stderr, "	[ERROR] Failed to write to: %s\n", full_output_path);
		}
	}

	free(content_md);
	free(content_html_partial);
	free(content_html_full);
	free(final_html);
	free_ast(ast_root);
	free_template_context(t_context);
}

void build_site_recursively(const char* vault_path, NavNode* node, SiteContext* s_context, TemplateContext* global_context) {
	if (is_ignored(node->full_path) || (strlen(node->name) > 0 && node->name[0] == '.')) {
		printf("[SKIP] Ignoring path: %s\n", node->full_path);
		return;
	}

	if (node->is_directory) {
		printf("[DIR] Entering directory: %s\n", node->full_path);

		char* card_template_str = read_file_into_string("templates/components/card.html");
		DynamicBuffer* post_list_buffer = create_dynamic_buffer(1024);
		NavNode* child;

		list_for_each_entry(child, &node->children, sibling) {
			if (!child->is_directory && strstr(child->name, ".md") && !is_ignored(child->full_path)) {
				TemplateContext* card_context = create_template_context();
				char* title_no_ext = strdup(child->name);
				char* dot = strrchr(title_no_ext, '.');
				if (dot) *dot = '\0';

				add_to_context(card_context, "card_item_title", title_no_ext);
				add_to_context(card_context, "card_item_link", child->output_path);
				add_to_context(card_context, "card_item_content", "...");

				char* rendered_card = render_template("templates/components/card.html", card_context);
				buffer_append_formatted(post_list_buffer, "%s", rendered_card);

				free(rendered_card);
				free(title_no_ext);
				free_template_context(card_context);
			}
		}
		if (card_template_str) free(card_template_str);
		char* post_list_html = destroy_buffer_and_get_content(post_list_buffer);

		TemplateContext* page_context = create_template_context();
		copy_context(page_context, global_context);
		add_to_context(page_context, "title", strlen(node->name) > 0 ? node->name : "Home");

		add_to_context(page_context, "post_list_content", post_list_html);
		char* content_html = render_template("templates/layout/post_list_layout.html", page_context);

		add_to_context(page_context, "content", content_html);

		char* final_html = render_template("templates/layout/base.html", page_context);

		char output_path[MAX_PATH_LENGTH];
		if (strlen(node->full_path) == 0) {
			snprintf(output_path, sizeof(output_path), "ssg_output/index.html");
		} else {
			snprintf(output_path, sizeof(output_path), "ssg_output/%s/index.html", node->full_path);
			create_directories_recursively(output_path);
		}

		FILE* out_file = fopen(output_path, "w");
		if (out_file) {
			fprintf(out_file, "%s", final_html);
			fclose(out_file);
			printf("Generated index page: %s\n", output_path);
		}

		free(post_list_html);
		free(final_html);
		free_template_context(page_context);

		list_for_each_entry(child, &node->children, sibling) {
			build_site_recursively(vault_path, child, s_context, global_context);
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

	printf("----- STARTING SSG BUILD -----\n");
	printf("Vault Path: %s\n", vault_path);
	printf("Output Dir: %s\n", output_dir);
	printf("------------------------------\n\n");

	printf("[STEP 1] Loading global context from config.json...\n");
	TemplateContext* global_context = create_template_context();
	load_config("config.json", global_context);

	printf("[STEP 2] Scanning vault and creating site context...\n");
	SiteContext* site_context = create_site_context(vault_path);

	printf("[STEP 3] Generating sidebar...\n");
	generate_sidebar_html(site_context, global_context);

	printf("[STEP 4] Loading .ssgignore and preparing output directory...\n");
	load_ssgignore(vault_path);
	mkdir(output_dir, 0755);

	printf("\n---- STARTING SITE GENERATION ----\n");
	build_site_recursively(vault_path, site_context->root, site_context, global_context);
	printf("---- SITE GENERATION FINISHED ----\n\n");

	printf("[STEP 6] Copying static files...\n");
	const char* static_dir = get_from_context(global_context, "static_dir");
	if (static_dir) {
		char dest_static_path[MAX_PATH_LENGTH];
		snprintf(dest_static_path, sizeof(dest_static_path), "%s/%s", output_dir, static_dir);
		copy_static_files(static_dir, dest_static_path);
	}

	printf("[STEP 7] Cleaning up...\n");
	free_site_context(site_context);
	free_template_context(global_context);
	free_ignore_patterns();

	printf("---- BUILD FINISHED SUCCESSFULLY! ----\n");

	return EXIT_SUCCESS;
}
