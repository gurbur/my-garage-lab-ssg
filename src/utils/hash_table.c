#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

static unsigned long hash(const char* str) {
	unsigned long hash = 5381;
	int c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

HashTable* ht_create(size_t size) {
	HashTable* ht = malloc(sizeof(HashTable));
	ht->size = size;
	ht->entries = calloc(ht->size, sizeof(HashEntry*));
	return ht;
}

void ht_set(HashTable* ht, const char* key, void* value) {
	unsigned long slot = hash(key) % ht->size;
	HashEntry* entry = ht->entries[slot];

	while(entry != NULL) {
		if (strcmp(entry->key, key) == 0) {
			entry->value = value;
			return;
		}
		entry = entry->next;
	}

	entry = malloc(sizeof(HashEntry));
	entry->key = strdup(key);
	entry->value = value;
	entry->next = ht->entries[slot];
	ht->entries[slot] = entry;
}

void* ht_get(HashTable* ht, const char* key) {
	unsigned long slot = hash(key) % ht->size;
	HashEntry* entry = ht->entries[slot];

	while (entry != NULL) {
		if (strcmp(entry->key, key) == 0) {
			return entry->value;
		}
		entry = entry->next;
	}
	return NULL;
}

void ht_destroy(HashTable* ht, void (*free_value)(void*)) {
	for (size_t i = 0; i < ht->size; i++) {
		HashEntry* entry = ht->entries[i];
		while (entry != NULL) {
			HashEntry* next = entry->next;
			if (free_value && entry->value) {
				free_value(entry->value);
			}
			free(entry->key);
			free(entry);
			entry = next;
		}
	}
	free(ht->entries);
	free(ht);
}
