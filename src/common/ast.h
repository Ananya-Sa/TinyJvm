#ifndef TINYJVM_AST_H
#define TINYJVM_AST_H

#include <stddef.h>
#include "token.h"
#include "str.h"

typedef enum {
    AST_COMP_UNIT,
    AST_IMPORT,
    AST_CLASS,
    AST_FIELD,
    AST_METHOD,
    AST_BLOCK,
    AST_RETURN,
    AST_VAR_DECL,
    AST_EXPR_STMT,
    AST_ASSIGN,
    AST_INC,
    AST_BIN,
    AST_INT_LIT,
    AST_STRING_LIT,
    AST_IDENT,
    AST_CALL,
    AST_NEW
} AstKind;

typedef struct Ast Ast;

struct Ast {
    AstKind kind;
    Token tok;
    Ast *next;
    union {
        struct {
            Ast *imports;
            Ast *clazz;
        } comp_unit;
        struct {
            Str name;
        } import_decl;
        struct {
            Str name;
            Ast *members;
        } class_decl;
        struct {
            Str name;
            Str type;
        } field_decl;
        struct {
            Str name;
            Str ret_type;
            int is_static;
            Ast *params;
            Ast *body;
        } method_decl;
        struct {
            Ast *stmts;
        } block;
        struct {
            Ast *expr;
        } return_stmt;
        struct {
            Str name;
            Str type;
            Ast *init;
        } var_decl;
        struct {
            Ast *expr;
        } expr_stmt;
        struct {
            Str name;
            Ast *value;
        } assign;
        struct {
            Str name;
        } inc;
        struct {
            TokenType op;
            Ast *lhs;
            Ast *rhs;
        } bin;
        struct {
            int value;
        } int_lit;
        struct {
            Str value;
        } string_lit;
        struct {
            Str name;
        } ident;
        struct {
            Str callee;
            Ast *args;
        } call;
        struct {
            Str class_name;
        } new_expr;
    } as;
};

#endif
