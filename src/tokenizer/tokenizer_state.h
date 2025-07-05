#ifndef TOKENIZER_STATE_H
#define TOKENIZER_STATE_H

#include <stdio.h>
#include "../include/list_head.h"
#include "../include/tokenizer.h"

typedef struct {
    char* data;
    int index;
    int capacity;
} TextBuffer;

typedef struct {
    FILE* stream;
    struct list_head* tokens;
    TextBuffer text_buffer;
} TokenizerState;

void init_text_buffer(TextBuffer* buffer);
void append_char_to_buffer(TextBuffer* buffer, char c);
void flush_buffer_as_token(TextBuffer* buffer, TokenType type, struct list_head* tokens);
void add_token(TokenType type, const char* value, struct list_head* tokens);

#endif
