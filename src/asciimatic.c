/* asciimatic.c
 * OpenCV driver.
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
#include <stdlib.h>
#include <string.h>

#include <cairo/cairo.h>
#include <libconfig.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "asciimatic.h"
#include "logging.h"
#include "main.h"
#include "utils.h"

/* Clobal config stuff */
extern config_t config;
extern bool_t verbose_mode;

/* Output params */
int output_rows;
int output_cols;

/* Edge detection stuff */
IplImage *src;

int first_thresh;
int second_thresh;
const char *valid_characters;

FILE *input_file;
FILE *output_file;

/* Font stuff */

cairo_surface_t *characters[256];

/* Given the supplied set of valid characters, generate the templates we'll be
 * matching against.
 */
static void 
init_templates(int char_height) {
    cairo_t *cr;

    memset(characters, 0, sizeof(characters));
    int chars = strlen(valid_characters);

    cairo_text_extents_t te;

    for (int i = 0; i < chars; i++) {
        char c = valid_characters[i];
        char letter[2] = {c, '\0'};
        
        characters[(int)c] = 
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, char_height, char_height);
        cr = cairo_create(characters[(int)c]);

        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_paint (cr);

        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, (int)(char_height * (4.0/3)));
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        
        cairo_text_extents (cr, letter, &te);
        cairo_move_to(cr, (char_height / 2) - te.x_bearing - te.width / 2,
                          (char_height / 2) - te.y_bearing - te.height / 2);
        cairo_show_text(cr, letter);

        cairo_destroy(cr);
    }
}

/* Performs Canny edge detection on an input image.  Caller is responsible for freeing the
 * allocated memory in the return value.
 */
IplImage *
detect_edges(IplImage *dst, IplImage *src) {
    if (dst != NULL) {
        cvReleaseImage(&dst);
    }
    dst = cvCreateImage(cvGetSize(src), 8, 1 );

    cvCanny(src, dst, first_thresh, second_thresh, 3);
    return dst;
}

void 
asciify(IplImage *edges) {
    int char_height = edges->height / output_rows;
    int char_width = edges->width / output_cols;

    xlog(LOG_INFO, "Characters correspond to %dx%d pixel blocks\n", char_width, char_height);
}

void
init_asciimatic(const char *filename) {
    if (!config_lookup_int(&config, "threshold1", &first_thresh)) {
        panic(1, "Missing default threshold1 parameter in config file");
    }
    if (!config_lookup_int(&config, "threshold2", &second_thresh)) {
        panic(1, "Missing default threshold2 parameter in config file");
    }
    if (!config_lookup_string(&config, "valid_characters", &valid_characters)) {
        panic(1, "Missing valid character set in config file");
    }

    src = cvLoadImage(filename, 0);
    if (src == NULL) {
        panic(1, "Can't load source image \"%s\"", filename);
    }

    init_templates(50);
}

void
shutdown_asciimatic(void) {
    int i;

    for (i = 0; i < 256; i++) {
        if (characters[i] != NULL) {
            cairo_surface_destroy(characters[i]);
        }
    }
    cvReleaseImage(&src);
}
