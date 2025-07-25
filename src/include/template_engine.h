#pragma once

#include "hash_table.h"

typedef HashTable TemplateContext;

TemplateContext* create_template_context();
void add_to_context(TemplateContext* context, const char* key, const char* value);
void free_template_context(TemplateContext* context);
const char* get_from_context(TemplateContext* context, const char* key);

void copy_context(TemplateContext* dest, const TemplateContext* src);

char* render_template(const char* layout_path, TemplateContext* context);
