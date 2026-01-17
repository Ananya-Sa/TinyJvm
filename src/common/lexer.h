#ifndef TINYJVM_LEXER_H
#define TINYJVM_LEXER_H

#include <stddef.h>
#include "token.h"
#include "diag.h"

typedef struct {
    const char *src;
    size_t len;
    size_t pos;
    Diag *diag;
} Lexer;

void lexer_init(Lexer *lx, const char *src, size_t len, Diag *diag);
Token lexer_next(Lexer *lx);

#endif
