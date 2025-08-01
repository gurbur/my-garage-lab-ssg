#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/site_context.h"
#include "../include/dynamic_buffer.h"
#include "../include/ignore_handler.h"

#define MAX_PATH_LENGTH 1024

static void scan_recursively(NavNode* parent, HashTable* name_lookup, HashTable* path_lookup, const char* base_path, const char* current_subpath);
static void build_sidebar_html_recursively(NavNode* node, DynamicBuffer* buffer, const char* base_url, TemplateContext* context);

static NavNode* create_nav_node(const char* name, const char* path, bool is_dir) {
	NavNode* node = malloc(sizeof(NavNode));
	node->name = strdup(name);
	node->full_path = strdup(path);
	node->is_directory = is_dir;
	node->slug = NULL;

	char output_path_buffer[MAX_PATH_LENGTH];
	strcpy(output_path_buffer, path);
	if (!is_dir) {
		char* dot = strrchr(output_path_buffer, '.');
		if (dot && strcmp(dot, ".md") == 0) {
			strcpy(dot, ".html");
		}
	}
	node->output_path = strdup(output_path_buffer);

	INIT_LIST_HEAD(&node->children);
	INIT_LIST_HEAD(&node->sibling);
	return node;
}

static void free_nav_node_recursively(NavNode* node) {
	if (!node) return;
	struct list_head *pos, *n;
	NavNode *child_node;

	list_for_each_safe(pos, n, &node->children) {
		child_node = list_entry(pos, NavNode, sibling);
		free_nav_node_recursively(child_node);
	}

	free(node->name);
	free(node->full_path);
	free(node->output_path);
	free(node->slug);
	free(node);
}

SiteContext* create_site_context(const char* vault_path) {
	SiteContext* context = malloc(sizeof(SiteContext));
	if (!context) return NULL;

	context->root = create_nav_node("Home", "", true);
	context->fast_lookup_by_name = ht_create(512);
	context->fast_lookup_by_path = ht_create(512);

	scan_recursively(context->root, context->fast_lookup_by_name, context->fast_lookup_by_path, vault_path, "");

	return context;
}

void free_site_context(SiteContext* context) {
	if (!context) return;
	free_nav_node_recursively(context->root);
	ht_destroy(context->fast_lookup_by_name, NULL);
	ht_destroy(context->fast_lookup_by_path, NULL);
	free(context);
}

static char* extract_slug_from_file(const char* file_path) {
	FILE* file = fopen(file_path, "r");
	if (!file) return NULL;

	char line[MAX_PATH_LENGTH];
	if (!fgets(line, sizeof(line), file) || strncmp(line, "---", 3) != 0) {
		fclose(file);
		return NULL;
	}

	char* slug = NULL;
	while (fgets(line, sizeof(line), file)) {
		if (strncmp(line, "---", 3) == 0) break;

		if (strncmp(line, "slug:", 5) == 0) {
			char* value = line + 5;
			while (*value && isspace((unsigned char)*value)) {
				value++;
			}

			size_t len = strlen(value);
			while (len > 0 && isspace((unsigned char)value[len - 1])) {
				value[--len] = '\0';
			}
			slug = strdup(value);
			break;
		}
	}
	fclose(file);
	return slug;
}

static void scan_recursively(NavNode* parent, HashTable* name_lookup, HashTable* path_lookup, const char* base_path, const char* current_subpath) {
	char current_full_path[MAX_PATH_LENGTH];
	snprintf(current_full_path, sizeof(current_full_path), "%s/%s", base_path, current_subpath);

	DIR* dir = opendir(current_full_path);
	if (!dir) return;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.') continue;

		char entry_relative_path[MAX_PATH_LENGTH];
		snprintf(entry_relative_path, sizeof(entry_relative_path), "%s%s%s", current_subpath, (strlen(current_subpath) > 0 ? "/" : ""), entry->d_name);

		char entry_full_path[MAX_PATH_LENGTH];
		int required_len = snprintf(entry_full_path, sizeof(entry_full_path), "%s/%s", current_full_path, entry->d_name);

		if (required_len >= sizeof(entry_full_path)) {
			fprintf(stderr, "Warning: Path is too long and was truncated: %s\n", current_full_path);
			continue;
		}

		struct stat entry_stat;
		if (stat(entry_full_path, &entry_stat) != 0) continue;

		bool is_dir = S_ISDIR(entry_stat.st_mode);
		NavNode* new_node = create_nav_node(entry->d_name, entry_relative_path, is_dir);

		if (!is_dir && strstr(new_node->name, ".md")) {
			new_node->slug = extract_slug_from_file(entry_full_path);

			if (!new_node->slug) {
				char* name_copy = strdup(new_node->name);
				char* dot = strrchr(name_copy, '.');
				if (dot) *dot = '\0';
				new_node->slug = name_copy;
			}
		}

		list_add_tail(&new_node->sibling, &parent->children);

		ht_set(name_lookup, new_node->name, new_node);
		ht_set(path_lookup, new_node->full_path, new_node);

		if (is_dir) {
			scan_recursively(new_node, name_lookup, path_lookup, base_path, entry_relative_path);
		}
	}
	closedir(dir);
}

void generate_sidebar_html(SiteContext* s_context, TemplateContext* global_context) {
	const char* base_url = get_from_context(global_context, "base_url");
	if (!base_url) base_url = "";

	DynamicBuffer* buffer = create_dynamic_buffer(1024);
	buffer_append_formatted(buffer, "<ul>\n");
	build_sidebar_html_recursively(s_context->root, buffer, base_url, global_context);
	buffer_append_formatted(buffer, "</ul>\n");

	char* sidebar_html = destroy_buffer_and_get_content(buffer);
	add_to_context(global_context, "sidebar_list", sidebar_html);
	free(sidebar_html);
}

void generate_breadcrumb_html(NavNode* current_node, TemplateContext* local_context, SiteContext* s_context) {
	const char* base_url = get_from_context(local_context, "base_url");
	if (!base_url) base_url = "";

	DynamicBuffer* buffer = create_dynamic_buffer(256);
	buffer_append_formatted(buffer, "<a href=\"%s/\">Home</a>", base_url);

	char* path_copy = strdup(current_node->full_path);
	char* token = strtok(path_copy, "/");
	DynamicBuffer* current_path_buffer = create_dynamic_buffer(256);

	while (token != NULL) {
		if (current_path_buffer->length > 0) {
			buffer_append_formatted(current_path_buffer, "/");
		}
		buffer_append_formatted(current_path_buffer, "%s", token);

		NavNode* node = ht_get(s_context->fast_lookup_by_path, current_path_buffer->content);
		if (!node) {
			token = strtok(NULL, "/");
			continue;
		}

		if (node->is_directory) {
			char category_key[MAX_PATH_LENGTH];
			snprintf(category_key, sizeof(category_key), "category_slugs.%s", node->name);
			const char* category_slug = get_from_context(local_context, category_key);

			if (category_slug) {
				buffer_append_formatted(buffer, " &gt; <a href=\"%s/%s\">%s</a>", base_url, category_slug, node->name);
			}
		} else {
			char* display_name = strdup(node->name);
			char* dot = strrchr(display_name, '.');
			if (dot) *dot = '\0';

			buffer_append_formatted(buffer, " &gt; <a href=\"%s/%s\">%s</a>", base_url, node->slug, display_name);
			free(display_name);
		}

		token = strtok(NULL, "/");
	}
	free(path_copy);
	destroy_buffer_and_get_content(current_path_buffer);

	char* breadcrumb_html = destroy_buffer_and_get_content(buffer);
	add_to_context(local_context, "breadcrumb", breadcrumb_html);
	free(breadcrumb_html);
}

static void build_sidebar_html_recursively(NavNode* node, DynamicBuffer* buffer, const char* base_url, TemplateContext* context) {
	NavNode* child;

	const char* static_dir = get_from_context(context, "build.static_dir");
	const char* image_dir = get_from_context(context, "build.image_dir");

	list_for_each_entry(child, &node->children, sibling) {
		if (is_ignored(child->full_path)) {
			continue;
		}

		if (child->is_directory) {
			if ((static_dir && strcmp(child->name, static_dir) == 0) || (image_dir && strcmp(child->name, image_dir) == 0)) {
				continue;
			}

			char category_key[MAX_PATH_LENGTH];
			snprintf(category_key, sizeof(category_key), "category_slugs.%s", child->name);
			const char* category_slug = get_from_context(context, category_key);

			if (category_slug) {
				buffer_append_formatted(buffer, "<li><a href=\"%s/%s\">%s</a>\n", base_url, category_slug, child->name);

				if (!list_empty(&child->children)) {
					buffer_append_formatted(buffer, "<ul>\n");
					build_sidebar_html_recursively(child, buffer, base_url, context);
					buffer_append_formatted(buffer, "</ul>\n");
				}
				buffer_append_formatted(buffer, "</li>\n");
			}
		}
	}
}

