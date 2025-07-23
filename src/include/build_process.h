#pragma once

#include "../include/site_context.h"
#include "../include/template_engine.h"

void build_site(const char* vault_path, SiteContext* s_context, TemplateContext* global_context, HashTable* old_cache, HashTable* new_cache);

