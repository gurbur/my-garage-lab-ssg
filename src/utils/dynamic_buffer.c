#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../include/dynamic_buffer.h"

DynamicBuffer* create_dynamic_buffer(size_t initial_capacity) {
	DynamicBuffer* buffer = malloc(sizeof(DynamicBuffer));
	if (!buffer) return NULL;

	buffer->capacity = initial_capacity > 0 ? initial_capacity : 1024;
	buffer->content = malloc(buffer->capacity);
	if (!buffer->content) {
		free(buffer);
		return NULL;
	}

	buffer->content[0] = '\0';
	buffer->length = 0;
	return buffer;
}

static void ensure_buffer_capacity(DynamicBuffer* buffer, size_t additional_length) {
	if (buffer->length + additional_length + 1 > buffer->capacity) {
		size_t new_capacity = buffer->capacity;
		while (buffer->length + additional_length + 1 > new_capacity) {
			new_capacity *= 2;
		}
		buffer->content = realloc(buffer->content, new_capacity);
		buffer->capacity = new_capacity;
	}
}

void buffer_append_formatted(DynamicBuffer* buffer, const char* format, ...) {
	va_list args1;
	va_start(args1, format);
	va_list args2;
	va_copy(args2, args1);

	int required_len = vsnprintf(NULL, 0, format, args1);
	va_end(args1);

	if (required_len < 0) {
		va_end(args2);
		return;
	}

	ensure_buffer_capacity(buffer, required_len);
	vsnprintf(buffer->content + buffer->length, required_len + 1, format, args2);
	va_end(args2);
	buffer->length += required_len;
}

char* destroy_buffer_and_get_content(DynamicBuffer* buffer) {
	if (!buffer) return NULL;
	char* final_content = buffer->content;
	free(buffer);
	return final_content;
}
