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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <limits>
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
static double search_radius;
static double shift_penalty_lambda;

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
char_for_subimage(const cv::Mat &image, const std::vector<cv::Mat> &templates,
                  cv::Mat &scratch, const cv::Mat &penalty) {
    int best_match = 0;
    double global_minval = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i < templates.size(); i++) {
        double minval, maxval;

        cv::matchTemplate(image, templates[i], scratch, cv::TM_SQDIFF_NORMED);
        if (!penalty.empty()) {
            scratch += penalty;
        }
        cv::minMaxLoc(scratch, &minval, &maxval);

        /* TM_SQDIFF_NORMED is "lower is better"; the best position over the
         * search window has the smallest response, and the best template is
         * the one with the smallest such minimum. */
        if (minval < global_minval) {
            global_minval = minval;
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

    /* Search window allows +/- (search_radius * cell) pixels of shift, with
     * the template sliding over (2*radius_x+1) x (2*radius_y+1) positions. */
    int radius_x = (int)std::round(search_radius * char_width);
    int radius_y = (int)std::round(search_radius * char_height);
    int win_w = char_width  + 2 * radius_x;
    int win_h = char_height + 2 * radius_y;

    cv::Mat scratch(2 * radius_y + 1, 2 * radius_x + 1, CV_32FC1);
    cv::Rect img_rect(0, 0, edges.cols, edges.rows);

    /* Bias the match-score toward near-center alignments.  Without this an
     * off-center match in one cell can be reached just as cheaply from a
     * neighbor's search window, so a feature near a cell boundary lights up
     * two adjacent cells with the same character. */
    cv::Mat penalty;
    if (shift_penalty_lambda > 0.0 && (radius_x > 0 || radius_y > 0)) {
        penalty.create(2 * radius_y + 1, 2 * radius_x + 1, CV_32FC1);
        for (int dy = 0; dy <= 2 * radius_y; dy++) {
            for (int dx = 0; dx <= 2 * radius_x; dx++) {
                double ndx = (double)(dx - radius_x) / char_width;
                double ndy = (double)(dy - radius_y) / char_height;
                penalty.at<float>(dy, dx) =
                    (float)(shift_penalty_lambda * (ndx * ndx + ndy * ndy));
            }
        }
    }

    for (int j = 0; j < output_rows; j++) {
        for (int i = 0; i < output_cols; i++) {
            /* Window centered on the cell.  Pulling from `edges` directly
             * (rather than zero-padding a cell-sized ROI) lets the local
             * search see real neighbor content; we only fall back to zero
             * padding at the image's true boundary. */
            int x0 = i * char_width  - radius_x;
            int y0 = j * char_height - radius_y;
            cv::Rect win(x0, y0, win_w, win_h);
            cv::Rect clipped = win & img_rect;

            cv::Mat subimage;
            if (clipped.size() == win.size()) {
                subimage = edges(win);
            } else {
                subimage = cv::Mat::zeros(win_h, win_w, edges.type());
                if (clipped.area() > 0) {
                    cv::Rect dst(clipped.x - x0, clipped.y - y0,
                                 clipped.width, clipped.height);
                    edges(clipped).copyTo(subimage(dst));
                }
            }

            fprintf(output_file, "%c",
                    char_for_subimage(subimage, templates, scratch, penalty));
        }
        fprintf(output_file, "\n");
    }
    fflush(output_file);
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
    if (c <= 0 || r <= 0) {
        panic(1, "Grid dimensions must be positive (got %dx%d)", c, r);
    }

    int cell_width = 4, cell_height = 8;
    config_lookup_int(&config, "cell_width", &cell_width);
    config_lookup_int(&config, "cell_height", &cell_height);
    if (cell_width <= 0 || cell_height <= 0) {
        panic(1, "cell_width and cell_height must be positive (got %dx%d)",
              cell_width, cell_height);
    }

    search_radius = 0.5;
    config_lookup_float(&config, "search_radius", &search_radius);
    if (search_radius < 0.0) {
        panic(1, "search_radius must be non-negative (got %f)", search_radius);
    }

    shift_penalty_lambda = 0.0;
    config_lookup_float(&config, "shift_penalty", &shift_penalty_lambda);
    if (shift_penalty_lambda < 0.0) {
        panic(1, "shift_penalty must be non-negative (got %f)", shift_penalty_lambda);
    }

    int target_w = c * cell_width;
    int target_h = r * cell_height;
    int interp = (target_w <= src.cols && target_h <= src.rows)
                   ? cv::INTER_AREA : cv::INTER_CUBIC;
    cv::resize(src, src, cv::Size(target_w, target_h), 0, 0, interp);

    cv::GaussianBlur(src, src, cv::Size(3, 3), 0, 0);
    output_rows = r;
    output_cols = c;
}

extern "C" void
shutdown_asciimatic(void) {
    src.release();
}
