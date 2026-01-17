#ifndef TINYJVM_STR_H
#define TINYJVM_STR_H

#include <stddef.h>

typedef struct {
    const char *data;
    size_t len;
} Str;

int str_eq_c(Str s, const char *cstr);
int str_eq(Str a, Str b);

#endif
