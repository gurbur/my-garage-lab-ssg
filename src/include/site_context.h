#pragma once

#include "list_head.h"
#include "hash_table.h"
#include "template_engine.h"

typedef struct NavNode {
	char* name;
	char* full_path;
	char* output_path;
	char* slug;
	bool is_directory;

	struct list_head children;
	struct list_head sibling;
} NavNode;

typedef struct {
	NavNode* root;
	HashTable* fast_lookup_by_name;
	HashTable* fast_lookup_by_path;
} SiteContext;

SiteContext* create_site_context(const char* vault_path);
void free_site_context(SiteContext* context);

void generate_sidebar_html(SiteContext* s_context, TemplateContext* global_context);
void generate_breadcrumb_html(NavNode* current_node, TemplateContext* local_context, SiteContext* s_context);

