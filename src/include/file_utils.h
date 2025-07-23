#pragma once

char* read_file_into_string(const char* filepath);
int mkdir_p(const char* path);
void create_parent_directories(const char* file_path);
void copy_static_files(const char* src_dir, const char* dest_dir);
int check_path_type(const char* path);

