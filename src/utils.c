/* utils.c
 * Various utility functions.
 *
 * Copyright (c) 2014 Nathan Taylor <nbtaylor@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "utils.h"

void panic(int exitcode, const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vxlog(LOG_ERR, fmt, ap);
  va_end(ap);

  shutdown_logging();
  exit(exitcode);
}

char *xstrdup(const char *str) {
    char *p = strdup(str);

    if (!p) {
        panic(1, "strdup failed: %s", strerror(errno));
    }

    return p;
}

void *xcalloc(size_t count, size_t sz) {
    void *p = calloc(count, sz);

    if (!p) {
        panic(1, "calloc(%lu, %lu) failed: %s", count, sz, strerror(errno));
    }

    return p;
}

void *xrealloc(void *p, size_t sz) {
    void *r = realloc(p, sz);

    if (!r) {
        panic(1, "realloc failed: %s", strerror(errno));
    }

    return r;
}

void *xmalloc(size_t sz) {
    void *p = malloc(sz);

    if (!p) {
        panic(1, "malloc(%lu) failed: %s", sz, strerror(errno));
    }

    return p;
}

int xopen(const char *file, int oflag) {
    int fd;

    while ((fd = open(file, oflag)) == -1) {
        if (errno != EINTR) {
            panic(1, "Can't open file %s: %s", file, strerror(errno));
        }
    }

    return fd;
}

FILE *xfopen(const char *file, const char *mode) {
    FILE *f = fopen(file, mode);

    if (!f) {
        panic(1, "Can't open file %s: %s", file, strerror(errno));
    }

    return f;
}

ssize_t xgetline(char **lineptr, size_t *n, FILE *stream) {
    ssize_t chars = getline(lineptr, n, stream);

    if (chars == -1 && errno == EINVAL) {
        panic(1, "Bad arguments to getline()");
    }

    return chars;
}

