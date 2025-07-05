#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer_state.h"


void add_token(TokenType type, const char* value, struct list_head* tokens) {
    Token* new_token = (Token*)malloc(sizeof(Token));
    if (!new_token) {
        perror("Failed to allocate memory for token");
        exit(EXIT_FAILURE);
    }
    new_token->type = type;
    new_token->value = value ? strdup(value) : NULL;
    list_add_tail(&new_token->list, tokens);
}

void init_text_buffer(TextBuffer* buffer) {
    buffer->capacity = 256;
    buffer->data = (char*)malloc(buffer->capacity);
    if (!buffer->data) {
        perror("Failed to allocate text buffer");
        exit(EXIT_FAILURE);
    }
    buffer->index = 0;
    buffer->data[0] = '\0';
}

void append_char_to_buffer(TextBuffer* buffer, char c) {
    if (buffer->index >= buffer->capacity - 1) {
        buffer->capacity *= 2;
        buffer->data = (char*)realloc(buffer->data, buffer->capacity);
        if (!buffer->data) {
            perror("Failed to reallocate buffer");
            exit(EXIT_FAILURE);
        }
    }
    buffer->data[buffer->index++] = c;
    buffer->data[buffer->index] = '\0';
}

void flush_buffer_as_token(TextBuffer* buffer, TokenType type, struct list_head* tokens) {
    if (buffer->index > 0) {
        add_token(type, buffer->data, tokens);
        buffer->index = 0;
        buffer->data[0] = '\0';
    }
}


void handle_punctuation(TokenizerState* state, char c) {
    flush_buffer_as_token(&state->text_buffer, TOKEN_TEXT, state->tokens);
    switch (c) {
        case '#': add_token(TOKEN_HASH, NULL, state->tokens); break;
        case '*': add_token(TOKEN_ASTERISK, NULL, state->tokens); break;
        case '-': add_token(TOKEN_DASH, NULL, state->tokens); break;
        case '\n': add_token(TOKEN_NEWLINE, NULL, state->tokens); break;
        case '\t': add_token(TOKEN_TAB, NULL, state->tokens); break;
        case '[': add_token(TOKEN_LBRACKET, NULL, state->tokens); break;
        case ']': add_token(TOKEN_RBRACKET, NULL, state->tokens); break;
        case '(': add_token(TOKEN_LPAREN, NULL, state->tokens); break;
        case ')': add_token(TOKEN_RPAREN, NULL, state->tokens); break;
        case '!': add_token(TOKEN_EXCLAMATION, NULL, state->tokens); break;
        case '>': add_token(TOKEN_GREATER_THAN, NULL, state->tokens); break;
        case '\\': add_token(TOKEN_BACKSLASH, NULL, state->tokens); break;
        case '.': add_token(TOKEN_DOT, NULL, state->tokens); break;
        case '`': add_token(TOKEN_BACKTICK, NULL, state->tokens); break;
    }
}

void handle_fenced_code_block(TokenizerState* state) {
    flush_buffer_as_token(&state->text_buffer, TOKEN_TEXT, state->tokens);
    add_token(TOKEN_BACKTICK, NULL, state->tokens);
    add_token(TOKEN_BACKTICK, NULL, state->tokens);
    add_token(TOKEN_BACKTICK, NULL, state->tokens);

    char current_char;
    while ((current_char = fgetc(state->stream)) != EOF && current_char != '\n') {
        append_char_to_buffer(&state->text_buffer, current_char);
    }
    flush_buffer_as_token(&state->text_buffer, TOKEN_TEXT, state->tokens);
    if (current_char == '\n') add_token(TOKEN_NEWLINE, NULL, state->tokens);

    while ((current_char = fgetc(state->stream)) != EOF) {
        if (current_char == '`') {
            char next1 = fgetc(state->stream);
            char next2 = fgetc(state->stream);
            if (next1 == '`' && next2 == '`') {
                flush_buffer_as_token(&state->text_buffer, TOKEN_TEXT, state->tokens);
                add_token(TOKEN_BACKTICK, NULL, state->tokens);
                add_token(TOKEN_BACKTICK, NULL, state->tokens);
                add_token(TOKEN_BACKTICK, NULL, state->tokens);
                return;
            }
            ungetc(next2, state->stream);
            ungetc(next1, state->stream);
        }
        append_char_to_buffer(&state->text_buffer, current_char);
    }
}

void handle_number(TokenizerState* state, char first_char) {
    flush_buffer_as_token(&state->text_buffer, TOKEN_TEXT, state->tokens);
    append_char_to_buffer(&state->text_buffer, first_char);
    
    int next_char;
    while ((next_char = fgetc(state->stream)) != EOF && isdigit(next_char)) {
        append_char_to_buffer(&state->text_buffer, next_char);
    }
    if (next_char != EOF) {
        ungetc(next_char, state->stream);
    }
    
    flush_buffer_as_token(&state->text_buffer, TOKEN_NUMBER, state->tokens);
}
