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

static int output_rows;
static int output_cols;

static int num_threads;

/* Edge detection stuff */
IplImage *src;

int first_thresh;
int second_thresh;
const char *valid_characters;

FILE *input_file;
FILE *output_file;

/* Given the supplied set of valid characters, generate the templates we'll be
 * matching against.
 */
static IplImage**
init_templates(int char_width, int char_height) {
    IplImage **templates;
    int num_chars = strlen(valid_characters);

    templates = xmalloc(sizeof(IplImage*) * (1 + num_chars));
    templates[num_chars] = NULL;

    int square_side = MAX(char_width, char_height);

    /* For each character, render each as a square image and then scale it to the
     * (char_width x char_height) dimension.
     */
    for (int i = 0; i < num_chars; i++) {
        cairo_text_extents_t te;
        char c = valid_characters[i];
        char letter[2] = {c, '\0'};

        cairo_surface_t *surface =
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, char_width, char_height);
        cairo_t *cr = cairo_create(surface);

        /* Render the text onto the image. */
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_paint(cr);

        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, (int)(char_height * (4.0/3))); // x px <-> (4/3)x pt
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_text_extents (cr, letter, &te);
        cairo_move_to(cr, (char_width / 2) - te.x_bearing - te.width / 2,
                          (char_height / 2) - te.y_bearing - te.height / 2);
        cairo_show_text(cr, letter);
        cairo_surface_flush(surface);

        /* Copy the Cairo surface into an IplImage... */
        IplImage *image = cvCreateImageHeader(cvSize(char_width, char_height), IPL_DEPTH_8U, 4);
        char *data = xmalloc(char_width * char_height * sizeof(uint32_t));
        memcpy(data, cairo_image_surface_get_data(surface), char_width * char_height * sizeof(uint32_t));
        cvSetData(image, data, char_width * sizeof(uint32_t));

        /* ...next, convert to grayscale. */
        IplImage *grayscale_image = cvCreateImage(cvSize(char_width, char_height), IPL_DEPTH_8U, 1);
        cvCvtColor(image, grayscale_image, CV_RGB2GRAY);

        templates[i] = grayscale_image;

        free(image->imageData);
        cvReleaseImageHeader(&image);
        cairo_surface_destroy(surface);
        cairo_destroy(cr);
    }

    return templates;
}

void
free_templates(IplImage **templates) {
    int i, num_chars;

    num_chars = strlen(valid_characters);
    for (i = 0; i < num_chars; i++) {
        cvReleaseImage(&templates[i]);
    }
    free(templates);

    cvReleaseImage(&src);
}

static char
char_for_subimage(IplImage *image, IplImage **templates, IplImage *scratch) {
    IplImage **t;
    int i = 0, best_match;
    double global_maxval = -1.0;

    for (t = templates; *t != NULL; t++) {
        double minval, maxval;
        
        cvMatchTemplate(image, *t, scratch, CV_TM_SQDIFF_NORMED);
        cvPow(scratch, scratch, 3);
        cvNormalize(scratch, scratch, 1, 0, CV_MINMAX, 0);

        cvMinMaxLoc(scratch, &minval, &maxval, NULL, NULL, 0);

        if (maxval > global_maxval) {
            global_maxval = maxval;
            best_match = i;
        }
        i++;
    }

    return valid_characters[best_match];
}

void
asciify(IplImage *edges) {
    int i, j;
    int char_height = edges->height / output_rows;
    int char_width = edges->width / output_cols;
    IplImage *scratch;

    xlog(LOG_INFO, "Characters correspond to %dx%d pixel blocks\n", char_width, char_height);

    IplImage **templates = init_templates(char_width, char_height);

    /* subimage will be size [w*2,h*2] 
     * http://docs.opencv.org/modules/imgproc/doc/object_detection.html#matchtemplate */
    scratch = cvCreateImage(cvSize(char_width + 1, char_height + 1), IPL_DEPTH_32F, 1);

    for (j = 0; j < output_rows; j++) {
        for (i = 0; i < output_cols; i++) {
            IplImage *subimage;

            CvRect char_area = cvRect(i * char_width, j * char_height, char_width, char_height);
            cvSetImageROI(edges, char_area);

            subimage = cvCreateImage(cvSize(char_width * 2, char_height * 2), IPL_DEPTH_8U, 1);
            cvCopyMakeBorder(edges, subimage, cvPoint(char_width / 2, char_height / 2), IPL_BORDER_CONSTANT, cvScalarAll(0));

            fprintf(stderr, "%c", char_for_subimage(subimage, templates, scratch));

            cvReleaseImage(&subimage);
        }
        fprintf(stderr, "\n");
    }

    cvReleaseImage(&scratch);
    free_templates(templates);
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
init_asciimatic(const char *filename, int r, int c) {
    if (!config_lookup_int(&config, "threshold1", &first_thresh)) {
        panic(1, "Missing default threshold1 parameter in config file");
    }
    if (!config_lookup_int(&config, "threshold2", &second_thresh)) {
        panic(1, "Missing default threshold2 parameter in config file");
    }
    if (!config_lookup_string(&config, "valid_characters", &valid_characters)) {
        panic(1, "Missing valid character set in config file");
    }

    src = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
    if (src == NULL) {
        panic(1, "Can't load source image \"%s\"", filename);
    }
    cvSmooth(src, src, CV_GAUSSIAN, 3, 3, 0, 0);
    output_rows = r;
    output_cols = c;
}

void
shutdown_asciimatic(void) {
    cvReleaseImage(&src);
}
