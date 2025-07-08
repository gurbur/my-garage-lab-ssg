#pragma once

#include <stddef.h>

typedef struct DynamicBuffer {
	char* content;
	size_t length;
	size_t capacity;
} DynamicBuffer;

DynamicBuffer* create_dynamic_buffer(size_t initial_capacity);
void buffer_append_formatted(DynamicBuffer* buffer, const char* format, ...);
char* destroy_buffer_and_get_content(DynamicBuffer* buffer);

