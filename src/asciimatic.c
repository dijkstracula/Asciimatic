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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>

#include <cairo.h>
#include <libconfig.h>
#include <opencv2/opencv.hpp>

#include "asciimatic.h"
#include "logging.h"
#include "main.h"
#include "utils.h"

extern config_t config;

static int output_rows;
static int output_cols;

cv::Mat src;

int first_thresh;
int second_thresh;
const char *valid_characters;

FILE *input_file;
FILE *output_file;

/* Build template images for each valid character by rendering with cairo,
 * then converting to grayscale.  Cairo's ARGB32 is BGRA in memory on
 * little-endian, so BGRA2GRAY matches the actual byte layout. */
static std::vector<cv::Mat>
init_templates(int char_width, int char_height) {
    int num_chars = strlen(valid_characters);
    std::vector<cv::Mat> templates;
    templates.reserve(num_chars);

    for (int i = 0; i < num_chars; i++) {
        cairo_text_extents_t te;
        char c = valid_characters[i];
        char letter[2] = {c, '\0'};

        cairo_surface_t *surface =
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, char_width, char_height);
        cairo_t *cr = cairo_create(surface);

        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_paint(cr);

        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, (int)(char_height * (4.0 / 3))); /* x px <-> (4/3)x pt */
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_text_extents(cr, letter, &te);
        cairo_move_to(cr, (char_width / 2) - te.x_bearing - te.width / 2,
                          (char_height / 2) - te.y_bearing - te.height / 2);
        cairo_show_text(cr, letter);
        cairo_surface_flush(surface);

        cv::Mat rgba(char_height, char_width, CV_8UC4,
                     cairo_image_surface_get_data(surface),
                     cairo_image_surface_get_stride(surface));
        cv::Mat grayscale;
        cv::cvtColor(rgba, grayscale, cv::COLOR_BGRA2GRAY);
        templates.push_back(grayscale);

        cairo_surface_destroy(surface);
        cairo_destroy(cr);
    }

    return templates;
}

static char
char_for_subimage(const cv::Mat &image, const std::vector<cv::Mat> &templates, cv::Mat &scratch) {
    int best_match = 0;
    double global_maxval = -1.0;

    for (size_t i = 0; i < templates.size(); i++) {
        double minval, maxval;

        cv::matchTemplate(image, templates[i], scratch, cv::TM_SQDIFF_NORMED);
        cv::pow(scratch, 3, scratch);
        cv::normalize(scratch, scratch, 1, 0, cv::NORM_MINMAX);

        cv::minMaxLoc(scratch, &minval, &maxval);

        if (maxval > global_maxval) {
            global_maxval = maxval;
            best_match = (int)i;
        }
    }

    return valid_characters[best_match];
}

void
asciify(cv::Mat &edges) {
    int char_height = edges.rows / output_rows;
    int char_width  = edges.cols / output_cols;

    xlog(LOG_INFO, (char *)"Characters correspond to %dx%d pixel blocks\n", char_width, char_height);

    std::vector<cv::Mat> templates = init_templates(char_width, char_height);

    /* scratch sized per matchTemplate output formula. */
    cv::Mat scratch(char_height + 1, char_width + 1, CV_32FC1);

    for (int j = 0; j < output_rows; j++) {
        for (int i = 0; i < output_cols; i++) {
            cv::Rect char_area(i * char_width, j * char_height, char_width, char_height);
            cv::Mat roi = edges(char_area);

            cv::Mat subimage;
            cv::copyMakeBorder(roi, subimage,
                               char_height / 2, char_height / 2,
                               char_width  / 2, char_width  / 2,
                               cv::BORDER_CONSTANT, cv::Scalar::all(0));

            fprintf(stderr, "%c", char_for_subimage(subimage, templates, scratch));
        }
        fprintf(stderr, "\n");
    }
}

/* Performs Canny edge detection on an input image. */
cv::Mat
detect_edges(const cv::Mat &src) {
    cv::Mat dst;
    cv::Canny(src, dst, first_thresh, second_thresh, 3);
    return dst;
}

extern "C" void
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

    src = cv::imread(filename, cv::IMREAD_GRAYSCALE);
    if (src.empty()) {
        panic(1, "Can't load source image \"%s\"", filename);
    }
    cv::GaussianBlur(src, src, cv::Size(3, 3), 0, 0);
    output_rows = r;
    output_cols = c;
}

extern "C" void
shutdown_asciimatic(void) {
    src.release();
}
