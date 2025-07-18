#pragma once

#include "../include/parser.h"
#include "../include/dynamic_buffer.h"
#include "../include/template_engine.h"

void render_opening_tag_for_node(const AstNode* node, DynamicBuffer* buffer);
void render_closing_tag_for_node(const AstNode* node, DynamicBuffer* buffer);
void render_inline_node(const AstNode* node, DynamicBuffer* buffer, TemplateContext* context);
void render_self_closing_node(const AstNode* node, DynamicBuffer* buffer);

