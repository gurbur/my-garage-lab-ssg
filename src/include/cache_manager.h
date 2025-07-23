#pragma once

#include "hash_table.h"

#define CACHE_DIR ".ssg_cache"
#define CACHE_FILE "build.cache"

int ensure_cache_dir_exists();
HashTable* load_cache();
void save_cache(const HashTable* cache);

