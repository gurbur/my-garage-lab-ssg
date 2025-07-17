#include <stdlib.h>
#include <string.h>
#include "../include/template_engine.h"
#include "../include/hash_table.h"

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

void copy_context(TemplateContext* dest, const TemplateContext* src) {
	if(!dest || !src) return;

	for (size_t i = 0; i < src->size; i++) {
		HashEntry* entry = src->entries[i];

		while (entry != NULL) {
			add_to_context(dest, entry->key, (const char*)entry->value);
			entry = entry->next;
		}
	}
}
