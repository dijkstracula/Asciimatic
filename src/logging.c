/* logging.c
 * Wrapper around syslog or writing to a file/stderr.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libconfig.h>

#include "logging.h"
#include "main.h"
#include "utils.h"

extern const char *__progname;
extern config_t config;
extern bool_t verbose_mode;

bool_t syslog_enabled;
static FILE *logf = NULL;
static bool_t initialised = false;

bool_t init_logging() {
    if (initialised)
        return false;

    config_lookup_bool(&config, "syslog", (int *)&syslog_enabled);

    const char *log_path = NULL;
    config_lookup_string(&config, "logfile", &log_path);

    if (syslog_enabled && log_path != NULL) {
        panic(1, "Error in configuration file: both syslog logging and file logging are specified.");
    }

    if (syslog_enabled) {
        openlog(__progname, LOG_PID, LOG_USER);
    } else if (log_path != NULL) {
        logf = xfopen(log_path, "a"); /* TODO: should come up with a log rotation plan */
    } else {
        logf = stderr;
    }

    initialised = true;
    return true;
}

bool_t shutdown_logging() {
    if (!initialised)
        return false;

    if (logf && logf != stderr) 
        fclose(logf);

    return true;
}

char *getlogdate() {
    char *tp;
    time_t tm = time(NULL);

    tp = ctime(&tm);
    stripnl(tp);
    return(tp);
}

void xlog(int prio, char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vxlog(prio, fmt, ap);
    va_end(ap);
}

void vxlog(int prio, const char *fmt, va_list ap) {
    if(prio == LOG_DEBUG && !verbose_mode)
        return;

    if (!initialised) {
        logf = stderr;
    }

    if (syslog_enabled) {
        vsyslog(prio, fmt, ap);
    } else {
        fprintf(logf, "%s [%d]: %s: ", getlogdate(), getpid(), __progname);
        vfprintf(logf, fmt, ap);
        fprintf(logf, "\n");
        fflush(logf);
    }
}

