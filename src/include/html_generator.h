#pragma once

#include "parser.h"

char* generate_toc_from_ast(AstNode* ast_root);
char* generate_html_from_ast(AstNode* ast_root, TemplateContext* context);

