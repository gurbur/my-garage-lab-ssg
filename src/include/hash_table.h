#pragma once

#include <stddef.h>

typedef struct HashEntry {
	char* key;
	void* value;
	struct HashEntry* next;
} HashEntry;

typedef struct {
	HashEntry** entries;
	size_t size;
} HashTable;

HashTable* ht_create(size_t size);
void ht_set(HashTable* ht, const char* key, void* value);
void* ht_get(HashTable* ht, const char* key);
void ht_destroy(HashTable* ht, void (*free_value)(void*));

