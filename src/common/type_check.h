#ifndef TINYJVM_TYPE_CHECK_H
#define TINYJVM_TYPE_CHECK_H

#include "ast.h"
#include "diag.h"

int type_check_comp_unit(Ast *comp_unit, Diag *diag);

#endif
