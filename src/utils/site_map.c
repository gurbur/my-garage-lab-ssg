#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../include/site_map.h"

#define MAX_PATH_LENGTH 1024

static void scan_directory_recursively(SiteMap* site_map, const char* base_path, const char* current_subpath) {
	char current_full_path[MAX_PATH_LENGTH];
	snprintf(current_full_path, sizeof(current_full_path), "%s/%s", base_path, current_subpath);

	DIR* dir = opendir(current_full_path);
	if (!dir) return;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

		char entry_relative_path[MAX_PATH_LENGTH];
		snprintf(entry_relative_path, sizeof(entry_relative_path), "%s%s%s", current_subpath, (strlen(current_subpath) > 0 ? "/" : ""), entry->d_name);

		char entry_full_path[MAX_PATH_LENGTH];
		int required_len = snprintf(entry_full_path, sizeof(entry_full_path), "%s/%s", current_full_path, entry->d_name);

		if (required_len >= sizeof(entry_full_path)) {
			fprintf(stderr, "Warning: Path is too long and was truncated: %s\n", entry_full_path);
			continue;
		}

		struct stat entry_stat;
		if (stat(entry_full_path, &entry_stat) != 0) continue;

		if (S_ISDIR(entry_stat.st_mode)) {
			scan_directory_recursively(site_map, base_path, entry_relative_path);
		} else if (S_ISREG(entry_stat.st_mode)) {
			if (strstr(entry->d_name, ".md") || strstr(entry->d_name, ".png") || strstr(entry->d_name, ".jpg")) {
				FileInfo* info = malloc(sizeof(FileInfo));
				info->original_path = strdup(entry_relative_path);

				char output_path_buffer[MAX_PATH_LENGTH];
				strcpy(output_path_buffer, entry_relative_path);
				char* dot = strrchr(output_path_buffer, '.');
				if (dot && strcmp(dot, ".md") == 0) {
					strcpy(dot, ".html");
				}
				info->output_path = strdup(output_path_buffer);

				ht_set(site_map, entry->d_name, info);
			}
		}
	}
	closedir(dir);
}

SiteMap* create_site_map() {
	return ht_create(512);
}

void populate_site_map(SiteMap* site_map, const char* vault_path) {
	scan_directory_recursively(site_map, vault_path, "");
}

static void free_file_info(void* data) {
	FileInfo* info = (FileInfo*)data;
	free(info->original_path);
	free(info->output_path);
	free(info);
}

void free_site_map(SiteMap* site_map) {
	ht_destroy(site_map, free_file_info);
}

