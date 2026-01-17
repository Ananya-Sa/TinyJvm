#include "diag.h"

#include <stdarg.h>
#include <stdio.h>

void diag_init(Diag *d, const char *path, const char *source, size_t len) {
    d->path = path;
    d->source = source;
    d->len = len;
    d->had_error = 0;
}

static void diag_find_line(const char *src, size_t len, size_t pos, size_t *out_line_start, size_t *out_line_no) {
    size_t line = 1;
    size_t line_start = 0;
    for (size_t i = 0; i < len && i < pos; i++) {
        if (src[i] == '\n') {
            line++;
            line_start = i + 1;
        }
    }
    *out_line_start = line_start;
    *out_line_no = line;
}

void diag_error(Diag *d, size_t pos, const char *fmt, ...) {
    size_t line_start = 0;
    size_t line_no = 1;
    diag_find_line(d->source, d->len, pos, &line_start, &line_no);

    size_t col = pos >= line_start ? (pos - line_start + 1) : 1;

    fprintf(stderr, "%s:%zu:%zu: error: ", d->path, line_no, col);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    d->had_error = 1;
}
