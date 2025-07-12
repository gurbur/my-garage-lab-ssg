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

void process_file(const char* input_path, const char* output_path) {
	printf("Processing: %s\n", input_path);

	FILE* md_file = fopen(input_path, "r");
	if (!md_file) {
		perror("Could not open makedown file");
		return;
	}

	TemplateContext* context = create_template_context();
	char* content_md = parse_front_matter(md_file, context);
	fclose(md_file);

	if (!content_md) {
		md_file = fopen(input_path, "r");
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
	AstNode* ast_root = parse_tokens(&token_list);
	char* content_html = generate_html_from_ast(ast_root);
	add_to_context(context, "post_content", content_html);

	const char* layout_key = get_from_context(context, "layout");
	char layout_path[MAX_PATH_LENGTH];
	snprintf(layout_path, sizeof(layout_path), "templates/layout/%s.html", (layout_key && *layout_key) ? layout_key : "post_page_layout");

	char* final_html = render_template(layout_path, context);

	if (final_html) {
		char* out_dir = strdup(output_path);
		char* last_slash = strrchr(out_dir, '/');
		if (last_slash) {
			*last_slash = '\0';
			mkdir(out_dir, 0755);
		}
		free(out_dir);

		FILE* out_file = fopen(output_path, "w");
		if (out_file) {
			fprintf(out_file, "%s", final_html);
			fclose(out_file);
		}
	}

	free(content_md);
	free(content_html);
	free(final_html);
	free_ast(ast_root);
	free_template_context(context);
}

void walk_directory(const char* base_path, const char* current_path, const char* output_dir) {
	char full_path[MAX_PATH_LENGTH];
	snprintf(full_path, sizeof(full_path), "%s/%s", base_path, current_path);

	DIR* dir = opendir(full_path);
	if (!dir) return;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

		char path_relative_to_base[MAX_PATH_LENGTH];
		snprintf(path_relative_to_base, sizeof(path_relative_to_base), "%s/%s", current_path, entry->d_name);

		char entry_full_path[MAX_PATH_LENGTH];
		snprintf(entry_full_path, sizeof(entry_full_path), "%s/%s", full_path, entry->d_name);

		if (is_ignored(path_relative_to_base + 1)) continue;

		struct stat entry_stat;
		stat(entry_full_path, &entry_stat);

		if (S_ISDIR(entry_stat.st_mode)) {
			walk_directory(base_path, path_relative_to_base, output_dir);
		} else if (S_ISREG(entry_stat.st_mode) && strstr(entry->d_name, ".md")) {
			char output_path[MAX_PATH_LENGTH];
			snprintf(output_path, sizeof(output_path), "%s%s", output_dir, path_relative_to_base);

			char* dot = strrchr(output_path, '.');
			if (dot && strcmp(dot, ".md") == 0) {
				strcpy(dot, ".html");
			}
			process_file(entry_full_path, output_path);
		}
	}
	closedir(dir);
}


static void free_token_list(struct list_head* head) {
	Token *current_token, *temp;
	list_for_each_entry_safe(current_token, temp, head, list) {
		list_del(&current_token->list);
		if (current_token->value) {
			free(current_token->value);
		}
		free(current_token);
	}
}

int main(int argc, char *argv[]) {
	const char* vault_path = ".";
	if (argc > 1) {
		vault_path = argv[1];
	}

	const char* output_dir = "ssg_output";

	printf("Starting SSG build for vault: %s\n", vault_path);

	load_ssgignore(vault_path);

	mkdir(output_dir, 0755);

	walk_directory(vault_path, "", output_dir);

	free_ignore_patterns();

	printf("Build finished successfully!\n");

  return EXIT_SUCCESS;
}
