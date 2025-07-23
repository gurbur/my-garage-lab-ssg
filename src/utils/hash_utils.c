#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/hash_utils.h"
#include "../libs/sha256/sha256.h"

#define HASH_BUFFER_SIZE 4096

char* generate_file_hash(const char* file_path) {
	FILE* file = fopen(file_path, "rb");
	if (!file) {
		fprintf(stderr, "Error: Could not open file %s for hashing.\n", file_path);
		return NULL;
	}

	SHA256_CTX ctx;
	sha256_init(&ctx);

	unsigned char buffer[HASH_BUFFER_SIZE];
	size_t bytes_read;

	while ((bytes_read = fread(buffer, 1, HASH_BUFFER_SIZE, file))) {
		sha256_update(&ctx, buffer, bytes_read);
	}
	fclose(file);

	BYTE digest[SHA256_BLOCK_SIZE];
	sha256_final(&ctx, digest);

	char* hash_str = malloc(sizeof(char) * (SHA256_BLOCK_SIZE * 2 + 1));
	if (!hash_str) {
		fprintf(stderr, "Error: Memory allocation failed for hash string.\n");
		return NULL;
	}

	for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
		sprintf(hash_str + (i * 2), "%02x", digest[i]);
	}

	hash_str[SHA256_BLOCK_SIZE * 2] = '\0';

	return hash_str;
}
