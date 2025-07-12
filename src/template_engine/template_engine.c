#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/template_engine.h"
#include "../include/dynamic_buffer.h"

typedef struct {
	char* key;
	char* value;
	struct list_head list;
} TemplateData;

static char* read_file_into_string(const char* filepath) {
	FILE* file = fopen(filepath, "r");
	if (!file) {
		fprintf(stderr, "Could not open template file: %s\n", filepath);
		return NULL;
	}
	DynamicBuffer* db = create_dynamic_buffer(0);
	if (!db) {
		fclose(file);
		return NULL;
	}

	char line[1024];
	while (fgets(line, sizeof(line), file)) {
		buffer_append_formatted(db, "%s", line);
	}
	fclose(file);

	char* content = destroy_buffer_and_get_content(db);
	return content;
}

static char* replace_all_str(const char* orig, const char* rep, const char* with) {
	if (!orig || !rep || !with) return NULL;

	DynamicBuffer* db = create_dynamic_buffer(strlen(orig) * 1.5);
	const char* p = orig;
	size_t rep_len = strlen(rep);

	while(*p) {
		if (strstr(p, rep) == p) {
			buffer_append_formatted(db, "%s", with);
			p += rep_len;
		} else {
			char temp[2] = { *p, '\0' };
			buffer_append_formatted(db, "%s", temp);
			p++;
		}
	}
	return destroy_buffer_and_get_content(db);
}

TemplateContext* create_template_context() {
	TemplateContext* context = (TemplateContext*)malloc(sizeof(TemplateContext));
	if (context) {
		INIT_LIST_HEAD(context);
	}
	return context;
}

void add_to_context(TemplateContext* context, const char* key, const char* value) {
	TemplateData* data = (TemplateData*)malloc(sizeof(TemplateData));
	if (!data) return;

	data->key = strdup(key);
	data->value = strdup(value);
	list_add_tail(&data->list, context);
}

void free_template_context(TemplateContext* context) {
	TemplateData *pos, *n;
	list_for_each_entry_safe(pos, n, context, list) {
		free(pos->key);
		free(pos->value);
		list_del(&pos->list);
		free(pos);
	}
	free(context);
}

const char* get_from_context(TemplateContext* context, const char* key) {
	TemplateData *pos;
	list_for_each_entry(pos, context, list) {
		if (strcmp(pos->key, key) == 0) {
			return pos->value;
		}
	}
	return NULL;
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

char* render_template(const char* layout_path, TemplateContext* context) {
	char* current_html = read_file_into_string(layout_path);
	if (!current_html) return NULL;

	char* component_tag_start;
	while ((component_tag_start = strstr(current_html, "{{ component:"))) {
		char* component_tag_end = strstr(component_tag_start, "}}");
		if (!component_tag_end) break;

		size_t tag_len = (component_tag_end - component_tag_start) + strlen("}}");

		char* component_path = create_component_path(component_tag_start);
		if (!component_path) break;

		char* component_content = read_file_into_string(component_path);
		free(component_path);

		if (!component_content) component_content = strdup("");

		char* full_tag = malloc(tag_len + 1);
		strncpy(full_tag, component_tag_start, tag_len);
		full_tag[tag_len] = '\0';

		char* new_html = replace_all_str(current_html, full_tag, component_content);

		free(full_tag);
		free(component_content);
		free(current_html);
		current_html = new_html;
	}

	TemplateData* pos;
	list_for_each_entry(pos, context, list) {
		char placeholder[256];
		snprintf(placeholder, sizeof(placeholder), "{{ %s }}", pos->key);

		char* new_html = replace_all_str(current_html, placeholder, pos->value);
		free(current_html);
		current_html = new_html;
	}

	return current_html;
}
