/* gui.c
 * GUI rendering and interface management.
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

#include <libconfig.h>
#include <opencv2/opencv.hpp>

#include "asciimatic.h"
#include "gui.h"
#include "logging.h"
#include "main.h"
#include "utils.h"

extern config_t config;

static int lbutton_down = 0;
static int dirty = 1;

static const char *window_name = "Asciimatic";

static cv::Mat edges;

static void
on_trackbar(int val, void *userdata) {
    (void)val;
    (void)userdata;
    dirty = 1;
}

static void
on_mouse(int event, int x, int y, int flags, void *p) {
    (void)flags;
    (void)p;

    if (event == cv::EVENT_LBUTTONDOWN) {
        lbutton_down = 1;
    } else if (event == cv::EVENT_LBUTTONUP) {
        lbutton_down = 0;
    }

    if (!lbutton_down) {
        return;
    }

    dirty = 0;

    int rx = std::max(x - 11, 0);
    int ry = std::max(y - 11, 0);
    cv::Rect roi(rx, ry,
                 std::min(11, edges.cols - rx),
                 std::min(11, edges.rows - ry));
    if (roi.width > 0 && roi.height > 0) {
        edges(roi).setTo(cv::Scalar::all(0));
    }
}

extern "C" void
init_gui(void) {
    int max_thresh1, max_thresh2;

    if (!config_lookup_int(&config, "max_threshold1", &max_thresh1)) {
        panic(1, "Missing default maximum threshold1 parameter in config file");
    }
    if (!config_lookup_int(&config, "max_threshold2", &max_thresh2)) {
        panic(1, "Missing default maximum threshold2 parameter in config file");
    }

    cv::namedWindow(window_name, cv::WINDOW_NORMAL);
    cv::createTrackbar("low_th",  window_name, &first_thresh,  max_thresh1, on_trackbar);
    cv::createTrackbar("high_th", window_name, &second_thresh, max_thresh2, on_trackbar);
    cv::setMouseCallback(window_name, on_mouse, nullptr);
}

extern "C" void
gui_loop(void) {
    char c;

    while ((c = (char)cv::waitKey(50)) != 27) {
        if (dirty) {
            edges = detect_edges(src);
        }
        cv::imshow(window_name, edges);
    }
    /* TODO: make this nicer than just as we exit the loop */
    asciify(edges);
    edges.release();
}

extern "C" void
shutdown_gui(void) {
    cv::destroyAllWindows();
}
