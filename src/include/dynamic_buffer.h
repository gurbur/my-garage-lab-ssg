#pragma once

#include <stddef.h>

typedef struct DynamicBuffer DynamicBuffer;

DynamicBuffer* create_dynamic_buffer(size_t initial_capacity);
void buffer_append_formatted(DynamicBuffer* buffer, const char* format, ...);
char* destroy_buffer_and_get_content(DynamicBuffer* buffer);

