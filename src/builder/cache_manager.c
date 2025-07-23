#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/cache_manager.h"
#include "../include/file_utils.h"

#define INITIAL_CACHE_SIZE 1024

int ensure_cache_dir_exists() {
	int path_type = check_path_type(CACHE_DIR);

	switch (path_type) {
		case 0:
			printf("Cache directory '%s' not found.. Creating...\n", CACHE_DIR);
			if (mkdir_p(CACHE_DIR) != 0) {
				fprintf(stderr, "Error: Failed to create cache directory '%s'.\n", CACHE_DIR);
				return -1;
			}
			break;

		case 1:
			fprintf(stderr, "Error: '%s' already exists as a file. Cannot create cache directory.\n", CACHE_DIR);
			return -1;
		case 2:
			// directory already exists
			break;
		default:
			fprintf(stderr, "Error: Could not check path for cache directory '%s'.\n", CACHE_DIR);
			return -1;
	}

	return 0;
}

HashTable* load_cache() {
	HashTable* cache = ht_create(INITIAL_CACHE_SIZE);
	if (!cache) {
		return NULL;
	}

	char cache_path[256];
	snprintf(cache_path, sizeof(cache_path), "%s/%s", CACHE_DIR, CACHE_FILE);

	char* content = read_file_into_string(cache_path);
	if (!content) {
		return cache;
	}

	char* content_copy = strdup(content);
	char* line = strtok(content_copy, "\n");
	while (line != NULL) {
		char* delimiter = strchr(line, ':');
		if (delimiter) {
			*delimiter = '\0';
			char* key = line;
			char* value = delimiter + 1;

			char* key_copy = strdup(key);
			char* value_copy = strdup(value);
			ht_set(cache, key_copy, value_copy);
		}
		line = strtok(NULL, "\n");
	}

	free(content_copy);
	free(content);
	return cache;
}

void save_cache(const HashTable* cache) {
	if (!cache) {
		return;
	}

	char cache_path[256];
	snprintf(cache_path, sizeof(cache_path), "%s/%s", CACHE_DIR, CACHE_FILE);

	FILE* file = fopen(cache_path, "w");
	if (!file) {
		fprintf(stderr, "Error: Could not open cache file for writing: %s\n", cache_path);
		return;
	}
	
	for (int i = 0; i < cache->size; ++i) {
		HashEntry* entry = cache->entries[i];
		while (entry) {
			fprintf(file, "%s:%s\n", entry->key, (char*)entry->value);
			entry = entry->next;
		}
	}

	fclose(file);
}
