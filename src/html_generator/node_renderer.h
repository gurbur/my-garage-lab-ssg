#pragma once

#include "../include/parser.h"
#include "../include/dynamic_buffer.h"

void render_opening_tag_for_node(const AstNode* node, DynamicBuffer* buffer);
void render_closing_tag_for_node(const AstNode* node, DynamicBuffer* buffer);
void render_inline_node(const AstNode* node, DynamicBuffer* buffer);
void render_self_closing_node(const AstNode* node, DynamicBuffer* buffer);

