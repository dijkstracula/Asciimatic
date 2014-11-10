/* main.c
 * The entry point.
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>

#include "asciimatic.h"
#include "gui.h"
#include "logging.h"
#include "utils.h"

config_t config;

bool_t verbose_mode; /* Log LOG_DEBUG messages, bounds checking, etc? */

extern const char *__progname;
const char *input_filename;

static void validate_config(int argc, char **argv) {
    char optch;
    extern int optind;
    extern char *optarg;
    bool_t show_usage = false;
    bool_t show_version = false;
    bool_t valid_usage = true;


    extern FILE *output_file;
    output_file = stdout;

    while ((optch = getopt(argc, argv, "ho:v")) != EOF) {
        switch (optch) {
            case 'o':
                output_file = xfopen(optarg, "w");
            case 'v':
                show_version = true;
                break;
            default:
                valid_usage = false; /* FALLTHROUGH */
            case 'h':
                show_usage = true;
                break;
        }
    }

    if (show_version) {
        printf("%s version %s\n", __progname, VERSION_STRING);
        exit(0);
    }

    argc -= optind;
    argv += optind;

    if (argc != 3) {
        show_usage = true;
        goto done;
    }

    extern long output_rows;
    extern long output_cols;
    
    output_cols = strtol(argv[0], NULL, 10);
    if (output_cols== 0 || output_rows > 1024) {
        panic(1, "Bad column count size");
    }
    output_rows = strtol(argv[1], NULL, 10);
    if (output_cols == 0 || output_cols > 4096) {
        panic(1, "Bad row count size");
    }
    input_filename = argv[2];
    
done:
    if (show_usage) {
        fprintf(stderr, "usage: %s [options] <columns> <rows> <input file>\n", __progname);
        fprintf(stderr, "    -h: display this message\n");
        fprintf(stderr, "    -o <file>: output ASCII image to file rather than stdout\n");
        fprintf(stderr, "    -v: show version\n");
        exit(valid_usage ? 0 : 1);
    }
    
}

/* Parses the configuration file.  Returns whether the file is present (e.g. whether we are
 * using default values.
 */
static bool_t read_config() {
    config_init(&config);

    char *cf_path;
    if (asprintf(&cf_path, "config/%s.cfg", __progname) == -1) {
        panic(1, "Can't allocate space for asprintf()");
    }

    FILE *cfile = xfopen(cf_path, "r");
    if (config_read(&config, cfile) == CONFIG_FALSE) {
        panic(1, "Can't parse %s:%d : %s", 
                config_error_file(&config), config_error_line(&config), 
                (config_error_text(&config) ? config_error_text(&config) : "<unknown>"));
    }

    free(cf_path);
    return true;
}

int main(int argc, char **argv) {
    read_config();
    validate_config(argc, argv);
    
    init_logging();
    init_asciimatic(input_filename);
    init_gui();

    gui_loop();

    shutdown_gui();
    shutdown_asciimatic();
    shutdown_logging();

    config_destroy(&config);
}
