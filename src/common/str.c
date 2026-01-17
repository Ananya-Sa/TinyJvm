#include "str.h"

#include <string.h>

int str_eq_c(Str s, const char *cstr) {
    size_t n = strlen(cstr);
    if (s.len != n) {
        return 0;
    }
    return memcmp(s.data, cstr, n) == 0;
}

int str_eq(Str a, Str b) {
    if (a.len != b.len) {
        return 0;
    }
    return memcmp(a.data, b.data, a.len) == 0;
}
