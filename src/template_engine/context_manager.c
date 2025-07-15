#include <stdlib.h>
#include <string.h>
#include "../include/template_engine.h"
#include "../utils/hash_table.h"

TemplateContext* create_template_context() {
	return ht_create(128);
}

void add_to_context(TemplateContext* context, const char* key, const char* value) {
	ht_set(context, key, strdup(value));
}

void free_template_context(TemplateContext* context) {
	ht_destroy(context, free);
}

const char* get_from_context(TemplateContext* context, const char* key) {
	return (const char*)ht_get(context, key);
}

