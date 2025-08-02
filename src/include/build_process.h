#pragma once

#include "site_context.h"
#include "template_engine.h"
#include "list_head.h"

typedef struct {
	NavNode* node;
	int id;
	int order;
	char* date;
	bool has_order;
	struct list_head list;
} PostSortInfo;

void build_site(const char* vault_path, SiteContext* s_context, TemplateContext* global_context, HashTable* old_cache, HashTable* new_cache, struct list_head* all_posts);
int compare_posts(const void* a, const void* b);

