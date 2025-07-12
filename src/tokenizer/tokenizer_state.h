#ifndef TOKENIZER_STATE_H
#define TOKENIZER_STATE_H

#include "../include/list_head.h"
#include "../include/tokenizer.h"
#include "../include/dynamic_buffer.h"

typedef struct {
	const char* current;
	struct list_head* tokens;
	DynamicBuffer* text_buffer;
} TokenizerState;

void append_char_to_buffer(DynamicBuffer* buffer, char c);
void flush_buffer_as_token(DynamicBuffer* buffer, TokenType type, struct list_head* tokens);
void add_token(TokenType type, const char* value, struct list_head* tokens);

#endif
