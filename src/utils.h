/* utils.h */

#ifndef __UTILS_H_
#define __UTILS_H_

#include <unistd.h>

#define stripnl(x) x[strlen(x) - 1] = '\0';

void panic(int, const char *, ...);
void panicx(int, const char *, ...);

char *xstrdup(const char *str);
void *xcalloc(size_t count, size_t sz);
void *xrealloc(void *p, size_t sz);
void *xmalloc(size_t sz);

FILE *xfopen(const char *path, const char *flags);

ssize_t xgetline(char **lineptr, size_t *n, FILE *stream);
#endif
