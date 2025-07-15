#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/template_engine.h"
#include "../include/dynamic_buffer.h"
#include "../include/list_head.h"
#include "template_utils.h"

typedef struct {
	const char* path;
	struct list_head list;
} DependencyNode;

static bool is_in_dependency_stack(const char* path, struct list_head* dependency_stack) {
	DependencyNode* pos;
	list_for_each_entry(pos, dependency_stack, list) {
		if (strcmp(pos->path, path) == 0) {
			return true;
		}
	}
	return false;
}

static char* create_component_path(const char* placeholder_start) {
	const char* start = placeholder_start + strlen("{{ component:");
	while (*start == ' ') start++;

	const char* end = strstr(start, "}}");
	if (!end) return NULL;

	const char* name_end = end;
	while (name_end > start && *(name_end - 1) == ' ') name_end--;

	size_t name_len = name_end - start;
	if (name_len == 0) return NULL;

	char* path = malloc(strlen("templates/components/") + name_len + strlen(".html") + 1);
	sprintf(path, "templates/components/%.*s.html", (int)name_len, start);

	return path;
}

static char* render_components(char* html_content, struct list_head* dependency_stack) {
	char* component_tag_start;
	while ((component_tag_start = strstr(html_content, "{{ component:"))) {
		char* component_tag_end = strstr(component_tag_start, "}}");
		if (!component_tag_end) break;

		size_t tag_len = (component_tag_end - component_tag_start) + strlen("}}");
		char* component_path = create_component_path(component_tag_start);
		if (!component_path) break;

		if (is_in_dependency_stack(component_path, dependency_stack)) {
			fprintf(stderr, "Error: circular component dependency detected! %s is already in the render stack.\n", component_path);
			free(component_path);
			char* next_html = replace_all_str(html_content, component_tag_start, "");
			free(html_content);
			html_content = next_html;
			continue;
		}

		DependencyNode new_dep = { .path = component_path };
		list_add_tail(&new_dep.list, dependency_stack);

		char* component_content = read_file_into_string(component_path);
		if (!component_content) component_content = strdup("");

		char* rendered_sub_component = render_components(component_content, dependency_stack);

		list_del(&new_dep.list);

		char* full_tag = malloc(tag_len + 1);
		strncpy(full_tag, component_tag_start, tag_len);
		full_tag[tag_len] = '\0';

		char* next_html = replace_all_str(html_content, full_tag, rendered_sub_component);

		free(full_tag);
		free(rendered_sub_component);
		free(component_path);
		free(html_content);
		html_content = next_html;
	}
	return html_content;
}

static char* render_data(char* html_content, TemplateContext* context) {
	DynamicBuffer* output_buffer = create_dynamic_buffer(strlen(html_content) * 1.5);
	const char* p = html_content;

	while(*p) {
		if (strncmp(p, "{{ ", 3) == 0) {
			const char* key_start = p + 3;
			const char* key_end = strstr(key_start, " }}");

			if (key_end) {
				size_t full_placeholder_len = (key_end - p) + 3;

				while (key_start < key_end && isspace((unsigned char)*key_start)) key_start++;
				const char* temp_end = key_end;
				while (temp_end > key_start && isspace((unsigned char)*(temp_end - 1))) temp_end--;

				size_t key_len = temp_end - key_start;

				if (key_len > 0) {
					char key[256];
					if (key_len < sizeof(key)) {
						strncpy(key, key_start, key_len);
						key[key_len] = '\0';
						const char* value = get_from_context(context, key);
						if (value) {
							buffer_append_formatted(output_buffer, "%s", value);
						} else {
							buffer_append_formatted(output_buffer, "%.*s", (int)full_placeholder_len, p);
						}
					}
				} else {
					buffer_append_formatted(output_buffer, "%.*s", (int)full_placeholder_len, p);
				}
				p = key_end + 3;
				continue;
			}
		}
		buffer_append_formatted(output_buffer, "%c", *p);
		p++;
	}

	free(html_content);
	return destroy_buffer_and_get_content(output_buffer);
}

char* render_template(const char* layout_path, TemplateContext* context) {
	char* final_html = read_file_into_string(layout_path);
	if (!final_html) return NULL;

	LIST_HEAD(dependency_stack);
	final_html = render_components(final_html, &dependency_stack);

	final_html = render_data(final_html, context);

	return final_html;
}
