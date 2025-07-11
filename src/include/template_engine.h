#pragma once

#include "list_head.h"

typedef struct list_head TemplateContext;

TemplateContext* create_template_context();
void add_to_context(TemplateContext* context, const char* key, const char* value);
void free_template_context(TemplateContext* context);
const char* get_from_context(TemplateContext* context, const char* key);
char* render_template(const char* layout_path, TemplateContext* context);

