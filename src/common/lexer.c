#include "lexer.h"

#include <ctype.h>
#include <string.h>

static int is_ident_start(int c) {
    return isalpha(c) || c == '_';
}

static int is_ident_part(int c) {
    return isalnum(c) || c == '_';
}

static Token make_token(Lexer *lx, TokenType type, size_t start, size_t end) {
    Token t;
    t.type = type;
    t.start = lx->src + start;
    t.len = end - start;
    t.pos = start;
    return t;
}

static int match_kw(const char *s, size_t len, const char *kw) {
    size_t kw_len = strlen(kw);
    if (len != kw_len) {
        return 0;
    }
    return memcmp(s, kw, kw_len) == 0;
}

static TokenType keyword_type(const char *s, size_t len) {
    if (match_kw(s, len, "class")) return TOK_KW_CLASS;
    if (match_kw(s, len, "public")) return TOK_KW_PUBLIC;
    if (match_kw(s, len, "static")) return TOK_KW_STATIC;
    if (match_kw(s, len, "import")) return TOK_KW_IMPORT;
    if (match_kw(s, len, "int")) return TOK_KW_INT;
    if (match_kw(s, len, "new")) return TOK_KW_NEW;
    if (match_kw(s, len, "return")) return TOK_KW_RETURN;
    if (match_kw(s, len, "void")) return TOK_KW_VOID;
    return TOK_IDENT;
}

void lexer_init(Lexer *lx, const char *src, size_t len, Diag *diag) {
    lx->src = src;
    lx->len = len;
    lx->pos = 0;
    lx->diag = diag;
}

static void skip_ws_and_comments(Lexer *lx) {
    for (;;) {
        while (lx->pos < lx->len && isspace((unsigned char)lx->src[lx->pos])) {
            lx->pos++;
        }
        if (lx->pos + 1 < lx->len && lx->src[lx->pos] == '/' && lx->src[lx->pos + 1] == '/') {
            lx->pos += 2;
            while (lx->pos < lx->len && lx->src[lx->pos] != '\n') {
                lx->pos++;
            }
            continue;
        }
        if (lx->pos + 1 < lx->len && lx->src[lx->pos] == '/' && lx->src[lx->pos + 1] == '*') {
            lx->pos += 2;
            while (lx->pos + 1 < lx->len) {
                if (lx->src[lx->pos] == '*' && lx->src[lx->pos + 1] == '/') {
                    lx->pos += 2;
                    break;
                }
                lx->pos++;
            }
            continue;
        }
        break;
    }
}

Token lexer_next(Lexer *lx) {
    skip_ws_and_comments(lx);

    if (lx->pos >= lx->len) {
        return make_token(lx, TOK_EOF, lx->pos, lx->pos);
    }

    size_t start = lx->pos;
    char c = lx->src[lx->pos++];

    if (is_ident_start((unsigned char)c)) {
        while (lx->pos < lx->len && is_ident_part((unsigned char)lx->src[lx->pos])) {
            lx->pos++;
        }
        TokenType type = keyword_type(lx->src + start, lx->pos - start);
        return make_token(lx, type, start, lx->pos);
    }

    if (isdigit((unsigned char)c)) {
        while (lx->pos < lx->len && isdigit((unsigned char)lx->src[lx->pos])) {
            lx->pos++;
        }
        return make_token(lx, TOK_INT_LIT, start, lx->pos);
    }

    switch (c) {
        case '{': return make_token(lx, TOK_LBRACE, start, lx->pos);
        case '}': return make_token(lx, TOK_RBRACE, start, lx->pos);
        case '(': return make_token(lx, TOK_LPAREN, start, lx->pos);
        case ')': return make_token(lx, TOK_RPAREN, start, lx->pos);
        case '[': return make_token(lx, TOK_LBRACKET, start, lx->pos);
        case ']': return make_token(lx, TOK_RBRACKET, start, lx->pos);
        case ';': return make_token(lx, TOK_SEMI, start, lx->pos);
        case ',': return make_token(lx, TOK_COMMA, start, lx->pos);
        case '.': return make_token(lx, TOK_DOT, start, lx->pos);
        case '=': return make_token(lx, TOK_EQ, start, lx->pos);
        case '+': return make_token(lx, TOK_PLUS, start, lx->pos);
        case '-': return make_token(lx, TOK_MINUS, start, lx->pos);
        case '*': return make_token(lx, TOK_STAR, start, lx->pos);
        case '/': return make_token(lx, TOK_SLASH, start, lx->pos);
        default:
            diag_error(lx->diag, start, "unexpected character '%c'", c);
            return make_token(lx, TOK_EOF, lx->pos, lx->pos);
    }
}
