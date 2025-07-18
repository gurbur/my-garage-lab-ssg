#pragma once

#include <stdbool.h>

void load_ssgignore(const char* base_path);
bool is_ignored(const char* path);
void free_ignore_patterns();
