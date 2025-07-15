#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "template_utils.h"
#include "../include/dynamic_buffer.h"

char* read_file_into_string(const char* filepath) {
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

char* replace_all_str(const char* orig, const char* rep, const char* with) {
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
