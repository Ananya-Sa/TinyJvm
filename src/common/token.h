#ifndef TINYJVM_TOKEN_H
#define TINYJVM_TOKEN_H

#include <stddef.h>
#include "str.h"

typedef enum {
    TOK_EOF = 0,
    TOK_IDENT,
    TOK_INT_LIT,
    TOK_STRING_LIT,

    TOK_KW_CLASS,
    TOK_KW_PUBLIC,
    TOK_KW_STATIC,
    TOK_KW_IMPORT,
    TOK_KW_INT,
    TOK_KW_NEW,
    TOK_KW_RETURN,
    TOK_KW_VOID,

    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_SEMI,
    TOK_COMMA,
    TOK_DOT,
    TOK_EQ,
    TOK_PLUS,
    TOK_PLUS_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    size_t len;
    size_t pos;
} Token;

static inline Str token_text(Token t) {
    Str s;
    s.data = t.start;
    s.len = t.len;
    return s;
}

#endif
