#include "type_check.h"

#include <stdio.h>

#include "str.h"

typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_STRING_ARRAY,
    TYPE_UNKNOWN
} TypeKind;

typedef struct {
    Str name;
    TypeKind type;
} Local;

typedef struct {
    Local locals[256];
    int count;
} LocalMap;

static TypeKind type_from_str(Str s) {
    if (str_eq_c(s, "int")) return TYPE_INT;
    if (str_eq_c(s, "String")) return TYPE_STRING;
    if (str_eq_c(s, "void")) return TYPE_VOID;
    if (str_eq_c(s, "String[]")) return TYPE_STRING_ARRAY;
    return TYPE_UNKNOWN;
}

static const char *type_name(TypeKind t) {
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_STRING: return "String";
        case TYPE_VOID: return "void";
        case TYPE_STRING_ARRAY: return "String[]";
        default: return "unknown";
    }
}

static int locals_add(LocalMap *m, Str name, TypeKind type) {
    if (m->count >= 256) return -1;
    m->locals[m->count].name = name;
    m->locals[m->count].type = type;
    return m->count++;
}

static int locals_find(LocalMap *m, Str name) {
    for (int i = 0; i < m->count; i++) {
        if (str_eq(m->locals[i].name, name)) {
            return i;
        }
    }
    return -1;
}

static TypeKind locals_type(LocalMap *m, Str name) {
    int idx = locals_find(m, name);
    if (idx < 0) return TYPE_UNKNOWN;
    return m->locals[idx].type;
}

static int concat_foldable(Ast *expr) {
    if (!expr) return 0;
    switch (expr->kind) {
        case AST_STRING_LIT:
        case AST_INT_LIT:
            return 1;
        case AST_BIN:
            if (expr->as.bin.op != TOK_PLUS) return 0;
            return concat_foldable(expr->as.bin.lhs) && concat_foldable(expr->as.bin.rhs);
        default:
            return 0;
    }
}

static TypeKind check_expr(Ast *expr, Diag *diag, LocalMap *locals) {
    if (!expr) return TYPE_UNKNOWN;
    switch (expr->kind) {
        case AST_INT_LIT:
            return TYPE_INT;
        case AST_STRING_LIT:
            return TYPE_STRING;
        case AST_IDENT: {
            TypeKind t = locals_type(locals, expr->as.ident.name);
            if (t == TYPE_UNKNOWN) {
                diag_error(diag, expr->tok.pos, "unknown local '%.*s'", (int)expr->as.ident.name.len, expr->as.ident.name.data);
            }
            return t;
        }
        case AST_BIN: {
            TypeKind lt = check_expr(expr->as.bin.lhs, diag, locals);
            TypeKind rt = check_expr(expr->as.bin.rhs, diag, locals);
            if (expr->as.bin.op == TOK_PLUS) {
                if (lt == TYPE_INT && rt == TYPE_INT) return TYPE_INT;
                if ((lt == TYPE_STRING || rt == TYPE_STRING) && concat_foldable(expr)) {
                    return TYPE_STRING;
                }
                diag_error(diag, expr->tok.pos, "unsupported '+' operands (%s, %s)", type_name(lt), type_name(rt));
                return TYPE_UNKNOWN;
            }
            if (lt == TYPE_INT && rt == TYPE_INT) return TYPE_INT;
            diag_error(diag, expr->tok.pos, "binary operator only supports int operands");
            return TYPE_UNKNOWN;
        }
        case AST_CALL: {
            Str callee = expr->as.call.callee;
            if (!str_eq_c(callee, "System.out.println")) {
                diag_error(diag, expr->tok.pos, "unsupported call '%.*s'", (int)callee.len, callee.data);
                return TYPE_UNKNOWN;
            }
            Ast *arg = expr->as.call.args;
            if (!arg) return TYPE_VOID;
            if (arg->next) {
                diag_error(diag, expr->tok.pos, "println expects zero or one argument");
                return TYPE_UNKNOWN;
            }
            TypeKind at = check_expr(arg, diag, locals);
            if (at != TYPE_INT && at != TYPE_STRING) {
                diag_error(diag, expr->tok.pos, "println argument must be int or String");
            }
            return TYPE_VOID;
        }
        default:
            diag_error(diag, expr->tok.pos, "unsupported expression");
            return TYPE_UNKNOWN;
    }
}

static void check_stmt(Ast *stmt, Diag *diag, LocalMap *locals) {
    if (!stmt) return;
    switch (stmt->kind) {
        case AST_VAR_DECL: {
            if (locals_find(locals, stmt->as.var_decl.name) >= 0) {
                diag_error(diag, stmt->tok.pos, "duplicate local '%.*s'", (int)stmt->as.var_decl.name.len, stmt->as.var_decl.name.data);
                return;
            }
            TypeKind var_type = type_from_str(stmt->as.var_decl.type);
            if (var_type == TYPE_UNKNOWN || var_type == TYPE_VOID || var_type == TYPE_STRING_ARRAY) {
                diag_error(diag, stmt->tok.pos, "unsupported local type '%.*s'", (int)stmt->as.var_decl.type.len, stmt->as.var_decl.type.data);
                return;
            }
            if (locals_add(locals, stmt->as.var_decl.name, var_type) < 0) {
                diag_error(diag, stmt->tok.pos, "too many locals");
                return;
            }
            if (stmt->as.var_decl.init) {
                TypeKind init_type = check_expr(stmt->as.var_decl.init, diag, locals);
                if (init_type != TYPE_UNKNOWN && init_type != var_type) {
                    diag_error(diag, stmt->tok.pos, "type mismatch: '%s' cannot be assigned to '%s'", type_name(init_type), type_name(var_type));
                }
            }
        } break;
        case AST_EXPR_STMT:
            check_expr(stmt->as.expr_stmt.expr, diag, locals);
            break;
        case AST_ASSIGN: {
            TypeKind lt = locals_type(locals, stmt->as.assign.name);
            if (lt == TYPE_UNKNOWN) {
                diag_error(diag, stmt->tok.pos, "unknown local '%.*s'", (int)stmt->as.assign.name.len, stmt->as.assign.name.data);
                break;
            }
            TypeKind rt = check_expr(stmt->as.assign.value, diag, locals);
            if (rt != TYPE_UNKNOWN && rt != lt) {
                diag_error(diag, stmt->tok.pos, "type mismatch: '%s' cannot be assigned to '%s'", type_name(rt), type_name(lt));
            }
        } break;
        case AST_INC: {
            TypeKind lt = locals_type(locals, stmt->as.inc.name);
            if (lt != TYPE_INT) {
                diag_error(diag, stmt->tok.pos, "++ only supports int locals");
            }
        } break;
        case AST_RETURN:
            if (stmt->as.return_stmt.expr) {
                diag_error(diag, stmt->tok.pos, "return expression not allowed in void method");
                check_expr(stmt->as.return_stmt.expr, diag, locals);
            }
            break;
        case AST_BLOCK: {
            Ast *cur = stmt->as.block.stmts;
            while (cur) {
                check_stmt(cur, diag, locals);
                cur = cur->next;
            }
        } break;
        default:
            diag_error(diag, stmt->tok.pos, "unsupported statement");
            break;
    }
}

static void check_main_body(Ast *method, Diag *diag) {
    LocalMap locals;
    locals.count = 0;

    Ast *param = method->as.method_decl.params;
    if (param) {
        TypeKind pt = type_from_str(param->as.var_decl.type);
        if (pt != TYPE_STRING_ARRAY) {
            diag_error(diag, param->tok.pos, "main parameter must be String[]");
        } else {
            locals_add(&locals, param->as.var_decl.name, pt);
        }
    }

    if (method->as.method_decl.body) {
        Ast *stmt = method->as.method_decl.body->as.block.stmts;
        while (stmt) {
            check_stmt(stmt, diag, &locals);
            stmt = stmt->next;
        }
    }
}

int type_check_comp_unit(Ast *comp_unit, Diag *diag) {
    if (!comp_unit || comp_unit->kind != AST_COMP_UNIT || !comp_unit->as.comp_unit.clazz) {
        return 0;
    }
    Ast *member = comp_unit->as.comp_unit.clazz->as.class_decl.members;
    while (member) {
        if (member->kind == AST_FIELD) {
            TypeKind ft = type_from_str(member->as.field_decl.type);
            if (ft != TYPE_INT && ft != TYPE_STRING) {
                diag_error(diag, member->tok.pos, "unsupported field type '%.*s'", (int)member->as.field_decl.type.len, member->as.field_decl.type.data);
            }
        }
        if (member->kind == AST_METHOD) {
            Str name = member->as.method_decl.name;
            Str ret = member->as.method_decl.ret_type;
            Ast *params = member->as.method_decl.params;
            int is_static = member->as.method_decl.is_static;
            if (is_static && str_eq_c(name, "main")) {
                if (!str_eq_c(ret, "void")) {
                    diag_error(diag, member->tok.pos, "main must return void");
                }
                if (!params || params->next != NULL) {
                    diag_error(diag, member->tok.pos, "main must have one parameter");
                }
                check_main_body(member, diag);
                return !diag->had_error;
            }
        }
        member = member->next;
    }
    diag_error(diag, comp_unit->tok.pos, "main method not found");
    return 0;
}
