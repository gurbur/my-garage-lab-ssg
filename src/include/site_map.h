#pragma once

#include "../utils/hash_table.h"

typedef struct {
	char* original_path;
	char* output_path;
} FileInfo;

typedef HashTable SiteMap;

SiteMap* create_site_map();
void populate_site_map(SiteMap* site_map, const char* vault_path);
void free_site_map(SiteMap* site_map);

