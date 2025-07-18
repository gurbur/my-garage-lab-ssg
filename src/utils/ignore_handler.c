#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/ignore_handler.h"

#define MAX_IGNORE_PATTERNS 100
#define MAX_PATH_LENGTH 1024

static char* ignore_patterns[MAX_IGNORE_PATTERNS];
static int ignore_count = 0;

void load_ssgignore(const char* base_path) {
	char ssgignore_path[MAX_PATH_LENGTH];
	snprintf(ssgignore_path, sizeof(ssgignore_path), "%s/.ssgignore", base_path);

	FILE* file = fopen(ssgignore_path, "r");
	if (!file) return;

	char line[MAX_PATH_LENGTH];
	while (fgets(line, sizeof(line), file) && ignore_count < MAX_IGNORE_PATTERNS) {
		line[strcspn(line, "\n")] = 0;
		if (strlen(line) > 0) {
			ignore_patterns[ignore_count++] = strdup(line);
		}
	}
	fclose(file);
}

bool is_ignored(const char* path) {
	for (int i = 0; i < ignore_count; i++) {
		if (strstr(path, ignore_patterns[i]) == path) {
			return true;
		}
	}
	return false;
}

void free_ignore_patterns() {
	for (int i = 0; i < ignore_count; i++) {
		free(ignore_patterns[i]);
	}
}

