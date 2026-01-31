#include "parser.h"

#include <stdlib.h>
#include <string.h>

static Ast *ast_new(Parser *ps, AstKind kind, Token tok) {
    Ast *node = (Ast *)arena_alloc(ps->arena, sizeof(Ast));
    if (!node) {
        diag_error(ps->diag, tok.pos, "out of memory");
        return NULL;
    }
    node->kind = kind;
    node->tok = tok;
    node->next = NULL;
    return node;
}

static void ps_advance(Parser *ps) {
    ps->current = lexer_next(&ps->lx);
}

static int ps_match(Parser *ps, TokenType type) {
    if (ps->current.type == type) {
        ps_advance(ps);
        return 1;
    }
    return 0;
}

static Token ps_expect(Parser *ps, TokenType type, const char *msg) {
    Token t = ps->current;
    if (t.type != type) {
        diag_error(ps->diag, t.pos, "%s", msg);
        return t;
    }
    ps_advance(ps);
    return t;
}

void parser_init(Parser *ps, const char *src, size_t len, Diag *diag, Arena *arena) {
    lexer_init(&ps->lx, src, len, diag);
    ps->diag = diag;
    ps->arena = arena;
    ps->current = lexer_next(&ps->lx);
}

static Token ps_peek(Parser *ps) {
    Lexer saved = ps->lx;
    Token cur = ps->current;
    Token next = lexer_next(&saved);
    (void)cur;
    return next;
}

static Ast *parse_expr(Parser *ps);
static Ast *parse_call_args(Parser *ps);

static Ast *parse_primary(Parser *ps) {
    Token t = ps->current;
    if (ps_match(ps, TOK_INT_LIT)) {
        Ast *node = ast_new(ps, AST_INT_LIT, t);
        if (!node) return NULL;
        int value = 0;
        for (size_t i = 0; i < t.len; i++) {
            value = value * 10 + (t.start[i] - '0');
        }
        node->as.int_lit.value = value;
        return node;
    }
    if (ps_match(ps, TOK_STRING_LIT)) {
        Ast *node = ast_new(ps, AST_STRING_LIT, t);
        if (!node) return NULL;
        Str s;
        s.data = t.start + 1;
        s.len = t.len >= 2 ? t.len - 2 : 0;
        node->as.string_lit.value = s;
        return node;
    }
    if (ps_match(ps, TOK_IDENT)) {
        size_t start = t.pos;
        size_t end = t.pos + t.len;
        int dotted = 0;
        while (ps_match(ps, TOK_DOT)) {
            Token part = ps_expect(ps, TOK_IDENT, "expected identifier after '.'");
            dotted = 1;
            end = part.pos + part.len;
        }
        Str name;
        name.data = ps->lx.src + start;
        name.len = end - start;
        if (ps_match(ps, TOK_LPAREN)) {
            Ast *call = ast_new(ps, AST_CALL, t);
            if (!call) return NULL;
            call->as.call.callee = name;
            call->as.call.args = parse_call_args(ps);
            ps_expect(ps, TOK_RPAREN, "expected ')' after call arguments");
            return call;
        }
        if (dotted) {
            diag_error(ps->diag, t.pos, "expected '(' after qualified name");
            return NULL;
        }
        Ast *node = ast_new(ps, AST_IDENT, t);
        if (!node) return NULL;
        node->as.ident.name = name;
        return node;
    }
    if (ps_match(ps, TOK_KW_NEW)) {
        Token name = ps_expect(ps, TOK_IDENT, "expected class name after 'new'");
        ps_expect(ps, TOK_LPAREN, "expected '(' after class name");
        ps_expect(ps, TOK_RPAREN, "expected ')' after 'new' constructor");
        Ast *node = ast_new(ps, AST_NEW, name);
        if (!node) return NULL;
        node->as.new_expr.class_name = token_text(name);
        return node;
    }
    if (ps_match(ps, TOK_LPAREN)) {
        Ast *expr = parse_expr(ps);
        ps_expect(ps, TOK_RPAREN, "expected ')' after expression");
        return expr;
    }
    diag_error(ps->diag, t.pos, "expected expression");
    return NULL;
}

static Ast *parse_call_args(Parser *ps) {
    if (ps->current.type == TOK_RPAREN) {
        return NULL;
    }
    Ast *head = NULL;
    Ast *tail = NULL;
    for (;;) {
        Ast *arg = parse_expr(ps);
        if (!head) {
            head = tail = arg;
        } else if (tail) {
            tail->next = arg;
            tail = arg;
        }
        if (!ps_match(ps, TOK_COMMA)) {
            break;
        }
    }
    return head;
}

static int bin_prec(TokenType type) {
    switch (type) {
        case TOK_PLUS:
        case TOK_MINUS:
            return 10;
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_PERCENT:
            return 20;
        default:
            return 0;
    }
}

static Ast *parse_bin_rhs(Parser *ps, int prec, Ast *lhs) {
    for (;;) {
        int tok_prec = bin_prec(ps->current.type);
        if (tok_prec < prec) {
            return lhs;
        }
        Token op = ps->current;
        ps_advance(ps);
        Ast *rhs = parse_primary(ps);
        if (!rhs) return lhs;

        int next_prec = bin_prec(ps->current.type);
        if (tok_prec < next_prec) {
            rhs = parse_bin_rhs(ps, tok_prec + 1, rhs);
        }

        Ast *node = ast_new(ps, AST_BIN, op);
        if (!node) return lhs;
        node->as.bin.op = op.type;
        node->as.bin.lhs = lhs;
        node->as.bin.rhs = rhs;
        lhs = node;
    }
}

static Ast *parse_expr(Parser *ps) {
    Ast *lhs = parse_primary(ps);
    if (!lhs) return NULL;
    return parse_bin_rhs(ps, 1, lhs);
}

static Str parse_type(Parser *ps) {
    Token t = ps->current;
    if (!(ps_match(ps, TOK_KW_INT) || ps_match(ps, TOK_KW_VOID) || ps_match(ps, TOK_IDENT))) {
        diag_error(ps->diag, t.pos, "expected type name");
        Str s = {0};
        return s;
    }
    size_t start = t.pos;
    size_t end = t.pos + t.len;
    while (ps_match(ps, TOK_LBRACKET)) {
        Token right = ps_expect(ps, TOK_RBRACKET, "expected ']' after '[' in type");
        end = right.pos + right.len;
    }
    Str s;
    s.data = ps->lx.src + start;
    s.len = end - start;
    return s;
}

static Ast *parse_block(Parser *ps);

static Ast *parse_statement(Parser *ps) {
    if (ps->current.type == TOK_IDENT) {
        Token ident = ps->current;
        Token next = ps_peek(ps);
        if (next.type == TOK_EQ) {
            ps_advance(ps);
            ps_expect(ps, TOK_EQ, "expected '=' in assignment");
            Ast *value = parse_expr(ps);
            ps_expect(ps, TOK_SEMI, "expected ';' after assignment");
            Ast *node = ast_new(ps, AST_ASSIGN, ident);
            if (!node) return NULL;
            node->as.assign.name = token_text(ident);
            node->as.assign.value = value;
            return node;
        }
        if (next.type == TOK_PLUS_PLUS) {
            ps_advance(ps);
            ps_expect(ps, TOK_PLUS_PLUS, "expected '++'");
            ps_expect(ps, TOK_SEMI, "expected ';' after increment");
            Ast *node = ast_new(ps, AST_INC, ident);
            if (!node) return NULL;
            node->as.inc.name = token_text(ident);
            return node;
        }
    }
    if (ps_match(ps, TOK_KW_RETURN)) {
        Token t = ps->current;
        Ast *expr = NULL;
        if (ps->current.type != TOK_SEMI) {
            expr = parse_expr(ps);
        }
        ps_expect(ps, TOK_SEMI, "expected ';' after return statement");
        Ast *node = ast_new(ps, AST_RETURN, t);
        if (!node) return NULL;
        node->as.return_stmt.expr = expr;
        return node;
    }

    if (ps->current.type == TOK_LBRACE) {
        return parse_block(ps);
    }

    if (ps->current.type == TOK_KW_INT || (ps->current.type == TOK_IDENT && ps_peek(ps).type == TOK_IDENT)) {
        Str type = parse_type(ps);
        Token name = ps_expect(ps, TOK_IDENT, "expected identifier in variable declaration");
        Ast *node = ast_new(ps, AST_VAR_DECL, name);
        if (!node) return NULL;
        node->as.var_decl.type = type;
        node->as.var_decl.name = token_text(name);
        node->as.var_decl.init = NULL;
        if (ps_match(ps, TOK_EQ)) {
            node->as.var_decl.init = parse_expr(ps);
        }
        ps_expect(ps, TOK_SEMI, "expected ';' after variable declaration");
        return node;
    }

    Ast *expr = parse_expr(ps);
    ps_expect(ps, TOK_SEMI, "expected ';' after expression");
    Ast *stmt = ast_new(ps, AST_EXPR_STMT, expr ? expr->tok : ps->current);
    if (!stmt) return NULL;
    stmt->as.expr_stmt.expr = expr;
    return stmt;
}

static Ast *parse_block(Parser *ps) {
    Token t = ps_expect(ps, TOK_LBRACE, "expected '{' to start block");
    Ast *node = ast_new(ps, AST_BLOCK, t);
    if (!node) return NULL;
    Ast *head = NULL;
    Ast *tail = NULL;
    while (ps->current.type != TOK_RBRACE && ps->current.type != TOK_EOF) {
        Ast *stmt = parse_statement(ps);
        if (!head) {
            head = tail = stmt;
        } else if (tail) {
            tail->next = stmt;
            tail = stmt;
        }
    }
    ps_expect(ps, TOK_RBRACE, "expected '}' to close block");
    node->as.block.stmts = head;
    return node;
}

static Ast *parse_params(Parser *ps) {
    Ast *head = NULL;
    Ast *tail = NULL;
    if (ps->current.type == TOK_RPAREN) {
        return NULL;
    }
    for (;;) {
        Str type = parse_type(ps);
        Token name = ps_expect(ps, TOK_IDENT, "expected parameter name");
        Ast *param = ast_new(ps, AST_VAR_DECL, name);
        if (!param) return head;
        param->as.var_decl.type = type;
        param->as.var_decl.name = token_text(name);
        param->as.var_decl.init = NULL;
        if (!head) {
            head = tail = param;
        } else {
            tail->next = param;
            tail = param;
        }
        if (!ps_match(ps, TOK_COMMA)) {
            break;
        }
    }
    return head;
}

static Ast *parse_member(Parser *ps) {
    int is_static = 0;
    while (ps->current.type == TOK_KW_PUBLIC || ps->current.type == TOK_KW_STATIC) {
        if (ps_match(ps, TOK_KW_PUBLIC)) {
            continue;
        }
        if (ps_match(ps, TOK_KW_STATIC)) {
            is_static = 1;
        }
    }
    Str type = parse_type(ps);
    Token name = ps_expect(ps, TOK_IDENT, "expected member name");
    if (ps_match(ps, TOK_LPAREN)) {
        Ast *method = ast_new(ps, AST_METHOD, name);
        if (!method) return NULL;
        method->as.method_decl.is_static = is_static;
        method->as.method_decl.ret_type = type;
        method->as.method_decl.name = token_text(name);
        method->as.method_decl.params = parse_params(ps);
        ps_expect(ps, TOK_RPAREN, "expected ')' after parameters");
        method->as.method_decl.body = parse_block(ps);
        return method;
    }
    Ast *field = ast_new(ps, AST_FIELD, name);
    if (!field) return NULL;
    field->as.field_decl.type = type;
    field->as.field_decl.name = token_text(name);
    ps_expect(ps, TOK_SEMI, "expected ';' after field declaration");
    return field;
}

static Ast *parse_class(Parser *ps) {
    ps_match(ps, TOK_KW_PUBLIC);
    Token t = ps_expect(ps, TOK_KW_CLASS, "expected 'class'");
    Token name = ps_expect(ps, TOK_IDENT, "expected class name");
    Ast *node = ast_new(ps, AST_CLASS, t);
    if (!node) return NULL;
    node->as.class_decl.name = token_text(name);

    ps_expect(ps, TOK_LBRACE, "expected '{' after class name");

    Ast *head = NULL;
    Ast *tail = NULL;
    while (ps->current.type != TOK_RBRACE && ps->current.type != TOK_EOF) {
        Ast *member = parse_member(ps);
        if (!head) {
            head = tail = member;
        } else if (tail) {
            tail->next = member;
            tail = member;
        }
    }

    ps_expect(ps, TOK_RBRACE, "expected '}' to close class body");
    node->as.class_decl.members = head;
    return node;
}

static Ast *parse_import(Parser *ps) {
    Token start = ps_expect(ps, TOK_KW_IMPORT, "expected 'import'");
    Token first = ps_expect(ps, TOK_IDENT, "expected import name");
    size_t import_start = first.pos;
    size_t import_end = first.pos + first.len;
    while (ps_match(ps, TOK_DOT)) {
        Token part = ps_expect(ps, TOK_IDENT, "expected identifier after '.'");
        import_end = part.pos + part.len;
    }
    ps_expect(ps, TOK_SEMI, "expected ';' after import");

    Ast *node = ast_new(ps, AST_IMPORT, start);
    if (!node) return NULL;
    Str name;
    name.data = ps->lx.src + import_start;
    name.len = import_end - import_start;
    node->as.import_decl.name = name;
    return node;
}

Ast *parse_compilation_unit(Parser *ps) {
    Token t = ps->current;
    Ast *node = ast_new(ps, AST_COMP_UNIT, t);
    if (!node) return NULL;

    Ast *imports = NULL;
    Ast *tail = NULL;
    while (ps->current.type == TOK_KW_IMPORT) {
        Ast *imp = parse_import(ps);
        if (!imports) {
            imports = tail = imp;
        } else if (tail) {
            tail->next = imp;
            tail = imp;
        }
    }

    Ast *clazz = parse_class(ps);
    node->as.comp_unit.imports = imports;
    node->as.comp_unit.clazz = clazz;
    return node;
}
