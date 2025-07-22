#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "template_utils.h"
#include "../include/dynamic_buffer.h"

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
