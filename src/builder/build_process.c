#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdbool.h>

#include "../include/build_process.h"
#include "../include/list_head.h"
#include "../include/tokenizer.h"
#include "../include/parser.h"
#include "../include/html_generator.h"
#include "../include/dynamic_buffer.h"
#include "../include/ignore_handler.h"
#include "../include/file_utils.h"
#include "../include/hash_utils.h"
#include "../include/hash_table.h"

#define MAX_PATH_LENGTH 1024

typedef struct {
	NavNode* node;
	int id;
	int order;
	char* date;
	bool has_order;
} PostSortInfo;

static char* trim_whitespace(char* str) {
	while (isspace((unsigned char)*str)) str++;
	if (*str == 0) return str;

	char *end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	end[1] = '\0';
	return str;
}

int compare_posts(const void* a, const void* b) {
	const PostSortInfo* postA = (const PostSortInfo*)a;
	const PostSortInfo* postB = (const PostSortInfo*)b;

	if (postA->has_order != postB->has_order) {
		return postB->has_order - postA->has_order;
	}

	if (postA->has_order) {
		if (postA->order != postB->order) {
			return postA->order - postB->order;
		}
	}

	if (postA->id != postB->id) {
		return postA->id - postB->id;
	}

	if (postA->date && postB->date) {
		return strcmp(postA->date, postB->date);
	}

	return 0;
}

void extract_sort_info(const char* file_path, PostSortInfo* info) {
	info->id = 0;
	info->order = 0;
	info->date = strdup("9999-99-99");
	info->has_order = false;

	FILE* file = fopen(file_path, "r");
	if (!file) return;

	char line[MAX_PATH_LENGTH];
	if (!fgets(line, sizeof(line), file) || strncmp(line, "---", 3) != 0) {
		fclose(file);
		return;
	}

	while (fgets(line, sizeof(line), file) && strncmp(line, "---", 3) != 0) {
		char* key = strtok(line, ":");
		char* value_str = strtok(NULL, "\n");
		if (key && value_str) {
			char* trimmed_key = trim_whitespace(key);
			char* trimmed_value = trim_whitespace(value_str);
			if (strcmp(trimmed_key, "id") == 0) info->id = atoi(trimmed_value);
			if (strcmp(trimmed_key, "order") == 0) {
				info->order = atoi(trimmed_value);
				info->has_order = true;
			}
			if (strcmp(trimmed_key, "date") == 0) {
				free(info->date);
				info->date = strdup(trimmed_value);
			}
		}
	}
	fclose(file);
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

static void build_site_recursively(const char* vault_path, NavNode* node, SiteContext* s_context, TemplateContext* global_context, HashTable* old_cache, HashTable* new_cache);
static void process_file(const char* vault_path, NavNode* current_node, SiteContext* s_context, TemplateContext* global_context, HashTable* old_cache, HashTable* new_cache);

void build_site(const char* vault_path, SiteContext* s_context, TemplateContext* global_context, HashTable* old_cache, HashTable* new_cache) {
	printf("\n---- STARTING SITE GENERATION ----\n");
	build_site_recursively(vault_path, s_context->root, s_context, global_context, old_cache, new_cache);
	printf("\n---- SITE GENERATION FINISHED ----\n\n");
}

void build_site_recursively(const char* vault_path, NavNode* node, SiteContext* s_context, TemplateContext* global_context, HashTable* old_cache, HashTable* new_cache) {
	if (is_ignored(node->full_path) || (strlen(node->name) > 0 && node->name[0] == '.')) {
		printf("[SKIP] Ignoring path: %s\n", node->full_path);
		return;
	}

	if (node->is_directory) {
		const char* static_dir = get_from_context(global_context, "build.static_dir");
		const char* image_dir = get_from_context(global_context, "build.image_dir");

		if ((static_dir && strcmp(node->name, static_dir) == 0) || (image_dir && strcmp(node->name, image_dir) == 0)) {
			printf("[SKIP] Skipping index generation for static/image directory: %s\n", node->full_path);
			return;
		}

		printf("[DIR] Generating index for: %s\n", node->full_path);

		create_parent_directories(node->output_path);

		int post_count = 0;
		NavNode* child;
		list_for_each_entry(child, &node->children, sibling) {
			if (!child->is_directory && strstr(child->name, ".md") && !is_ignored(child->full_path)) {
				post_count++;
			}
		}

		PostSortInfo* sort_array = malloc(post_count * sizeof(PostSortInfo));
		int current_index = 0;
		list_for_each_entry(child, &node->children, sibling) {
			if (!child->is_directory && strstr(child->name, ".md") && !is_ignored(child->full_path)) {
				sort_array[current_index].node = child;
				char full_input_path[MAX_PATH_LENGTH];
				snprintf(full_input_path, sizeof(full_input_path), "%s/%s", vault_path, child->full_path);
				extract_sort_info(full_input_path, &sort_array[current_index]);
				current_index++;
			}
		}

		qsort(sort_array, post_count, sizeof(PostSortInfo), compare_posts);

		char* card_template_str = read_file_into_string("templates/components/card.html");
		DynamicBuffer* post_list_buffer = create_dynamic_buffer(1024);

		for (int i = 0; i < post_count; i++) {
			NavNode* sorted_child = sort_array[i].node;
			TemplateContext* card_context = create_template_context();
			char* title_no_ext = strdup(sorted_child->name);
			char* dot = strrchr(title_no_ext, '.');
			if (dot) *dot = '\0';

			char* link_filename = strdup(sorted_child->output_path);
			char* last_slash = strrchr(link_filename, '/');
			const char* final_link = last_slash ? last_slash + 1 : link_filename;

			add_to_context(card_context, "card_item_title", title_no_ext);
			add_to_context(card_context, "card_item_link", final_link);
			add_to_context(card_context, "card_item_content", "내용 예시");

			char* rendered_card = render_template("templates/components/card.html", card_context);
			buffer_append_formatted(post_list_buffer, "%s", rendered_card);

			free(rendered_card);
			free(title_no_ext);
			free(link_filename);
			free_template_context(card_context);
		}
		for (int i = 0; i < post_count; i++) {
			free(sort_array[i].date);
		}
		free(sort_array);

		if (card_template_str) free(card_template_str);
		char* post_list_html = destroy_buffer_and_get_content(post_list_buffer);

		TemplateContext* page_context = create_template_context();
		copy_context(page_context, global_context);

		generate_breadcrumb_html(node, page_context, s_context);
		add_to_context(page_context, "list_title", strlen(node->name) > 0 ? node->name : "Home");
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
			create_parent_directories(output_path);
		}

		FILE* out_file = fopen(output_path, "w");
		if (out_file) {
			fprintf(out_file, "%s", final_html);
			fclose(out_file);
			printf("Generated index page: %s\n", output_path);
		}

		free(post_list_html);
		free(content_html);
		free(final_html);
		free_template_context(page_context);

		list_for_each_entry(child, &node->children, sibling) {
			build_site_recursively(vault_path, child, s_context, global_context, old_cache, new_cache);
		}
	} else if (strstr(node->name, ".md")) {
		process_file(vault_path, node, s_context, global_context, old_cache, new_cache);
	}
}

void process_file(const char* vault_path, NavNode* current_node, SiteContext* s_context, TemplateContext* global_context, HashTable* old_cache, HashTable* new_cache) {
	printf("Processing: %s\n", current_node->full_path);

	char full_input_path[MAX_PATH_LENGTH];
	snprintf(full_input_path, sizeof(full_input_path), "%s/%s", vault_path, current_node->full_path);

	char* current_hash = generate_file_hash(full_input_path);
	char* old_cache_value = old_cache ? (char*)ht_get(old_cache, full_input_path) : NULL;

	if (old_cache_value) {
		char* old_hash_only = strdup(old_cache_value);
		char* delimiter = strrchr(old_hash_only, ':');
		if (delimiter) *delimiter = '\0';

		if (current_hash && strcmp(current_hash, old_hash_only) == 0) {
			char* output_path_from_cache = strrchr(old_cache_value, ':');
			if (output_path_from_cache) {
				output_path_from_cache++;
			}

			if (output_path_from_cache && check_path_type(output_path_from_cache) == 1) {
				printf("Skipping (cached): %s\n", current_node->full_path);
				ht_set(new_cache, strdup(full_input_path), strdup(old_cache_value));
				free(old_hash_only);
				free(current_hash);
				return;
			} else {
				printf("Rebuilding (output missing): %s\n", current_node->full_path);
			}
		}
		free(old_hash_only);
	}

	printf("Building: %s\n", current_node->full_path);

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
	char* content_html_partial = generate_html_from_ast(ast_root, t_context);
	add_to_context(t_context, "post_content", content_html_partial);

	generate_breadcrumb_html(current_node, t_context, s_context);

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
		const char* output_dir = get_from_context(global_context, "build.output_dir");
		if (!output_dir) output_dir = "ssg_output";
		char full_output_path[MAX_PATH_LENGTH];
		snprintf(full_output_path, sizeof(full_output_path), "%s/%s", output_dir, current_node->output_path);

		create_parent_directories(full_output_path);

		FILE* out_file = fopen(full_output_path, "w");
		if (out_file) {
			fprintf(out_file, "%s", final_html);
			fclose(out_file);
			printf("[SUCCESS] Created: %s\n", full_output_path);
		} else {
			fprintf(stderr, "	[ERROR] Failed to write to: %s\n", full_output_path);
		}

		if (current_hash) {
			DynamicBuffer* db = create_dynamic_buffer(0);
			buffer_append_formatted(db, "%s:%s", current_hash, full_output_path);
			char* cache_value_str = destroy_buffer_and_get_content(db);
			ht_set(new_cache, strdup(full_input_path), cache_value_str);
		}
	}

	free(content_md);
	free(content_html_partial);
	free(content_html_full);
	free(final_html);
	free_ast(ast_root);
	free_template_context(t_context);
	if (current_hash) free(current_hash);
}


