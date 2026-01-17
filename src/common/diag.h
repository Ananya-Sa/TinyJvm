#ifndef TINYJVM_DIAG_H
#define TINYJVM_DIAG_H

#include <stddef.h>

typedef struct {
    const char *path;
    const char *source;
    size_t len;
    int had_error;
} Diag;

void diag_init(Diag *d, const char *path, const char *source, size_t len);
void diag_error(Diag *d, size_t pos, const char *fmt, ...);

#endif
