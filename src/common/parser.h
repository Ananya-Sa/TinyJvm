#ifndef TINYJVM_PARSER_H
#define TINYJVM_PARSER_H

#include <stddef.h>
#include "ast.h"
#include "arena.h"
#include "lexer.h"
#include "diag.h"

typedef struct {
    Lexer lx;
    Token current;
    Diag *diag;
    Arena *arena;
} Parser;

void parser_init(Parser *ps, const char *src, size_t len, Diag *diag, Arena *arena);
Ast *parse_compilation_unit(Parser *ps);

#endif
