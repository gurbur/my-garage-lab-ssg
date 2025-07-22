#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../include/file_utils.h"

#define MAX_PATH_LENGTH 1024

char* read_file_into_string(const char* filepath) {
	FILE* file = fopen(filepath, "r");
	if (!file) return NULL;
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* buffer = malloc(length + 1);
	if (buffer) {
		fread(buffer, 1, length, file);
		buffer[length] = '\0';
	}
	fclose(file);
	return buffer;
}

void mkdir_p(const char* path) {
	char tmp[MAX_PATH_LENGTH];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", path);
	len = strlen(tmp);
	if(len > 0 && tmp[len - 1] == '/') {
		tmp[len - 1] = 0;
	}
	for(p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = 0;
			mkdir(tmp, 0755);
			*p = '/';
		}
	}
	mkdir(tmp, 0755);
}

void create_parent_directories(const char* file_path) {
	char parent_dir[MAX_PATH_LENGTH];
	strncpy(parent_dir, file_path, sizeof(parent_dir) - 1);
	parent_dir[sizeof(parent_dir) - 1] = '\0';

	char* last_slash = strrchr(parent_dir, '/');
	if (last_slash) {
		*last_slash = '\0';
		mkdir_p(parent_dir);
	}
}

void copy_static_files(const char* src_dir, const char* dest_dir) {
	printf("Copying static files from %s to %s...\n", src_dir, dest_dir);
	mkdir_p(dest_dir);

	DIR* dir = opendir(src_dir);
	if (!dir) {
		fprintf(stderr, "[ERROR] Cannot open source directory: %s\n", src_dir);
		return;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.') continue;

		char src_path[MAX_PATH_LENGTH];
		snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
		char dest_path[MAX_PATH_LENGTH];
		snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

		struct stat path_stat;
		stat(src_path, &path_stat);

		if (S_ISDIR(path_stat.st_mode)) {
			copy_static_files(src_path, dest_path);
		} else {
			printf("[DEBUG] Copying file: %s\n", src_path);
			FILE* src_file = fopen(src_path, "rb");
			FILE* dest_file = fopen(dest_path, "wb");
			if (src_file && dest_file) {
				char buffer[4096];
				size_t n;
				while ((n = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
					fwrite(buffer, 1, n, dest_file);
				}
				fclose(src_file);
				fclose(dest_file);
			} else {
				if (!src_file) fprintf(stderr, "[ERROR] Failed to open source file: %s\n", src_path);
				if (!dest_file) fprintf(stderr, "[ERROR] Failed to open destination file: %s\n", dest_path);
			}
		}
	}
	closedir(dir);
}


