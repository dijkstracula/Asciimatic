#ifndef __LOGGING_H_
#define __LOGGING_H_

#include <stdarg.h>
#include <syslog.h>

#include "main.h"

bool_t init_logging();
bool_t shutdown_logging();

void xlog(int prio, char *fmt, ...);
void vxlog(int prio, const char *fmt, va_list ap);

#endif
