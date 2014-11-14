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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libconfig.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "asciimatic.h"
#include "logging.h"
#include "main.h"
#include "utils.h"

/* Clobal config stuff */
extern config_t config;

extern IplImage *src;
extern int first_thresh;
extern int second_thresh;

/* GUI painting state */
static int lbutton_down = 0; //default to up
static int dirty = 1; /* Only repaint if there's something to repaint */

static const char *window_name = "Asciimatic";

IplImage *edges;



static void
on_trackbar(int val) {
    (void)val;
    dirty = 1;
}

static void
on_mouse(int event, int x, int y, int flags, void *p) {
    (void)flags;
    (void)p;

    if (event == CV_EVENT_LBUTTONDOWN) {
        lbutton_down = 1;
    }
    else if (event == CV_EVENT_LBUTTONUP) {
        lbutton_down = 0;
    }

    if (!lbutton_down) {
        return;
    }

    dirty = 0;

    CvRect roi = cvRect(
            MAX(x-11, 0), 
            MAX(y-11, 0), 
            11,11);
    cvSetImageROI(edges, roi);
    cvSetZero(edges);
    cvResetImageROI(edges);
}

void
init_gui() {
    int max_thresh1, max_thresh2;

    if (!config_lookup_int(&config, "max_threshold1", &max_thresh1)) {
        panic(1, "Missing default maximum threshold1 parameter in config file");
    }
    if (!config_lookup_int(&config, "max_threshold2", &max_thresh2)) {
        panic(1, "Missing default maximum threshold2 parameter in config file");
    }

    cvNamedWindow(window_name, CV_WINDOW_NORMAL);
    cvCreateTrackbar("low_th", window_name, &first_thresh, max_thresh1, on_trackbar);
    cvCreateTrackbar("high_th", window_name, &second_thresh, max_thresh2, on_trackbar);
    cvSetMouseCallback(window_name, on_mouse, NULL);
}

void
gui_loop() {
    char c;

    while ((c = cvWaitKey(50)) != 27) {
        if (dirty) {
            edges = detect_edges(edges, src);
        }
        cvShowImage(window_name, edges);
    }
    //TODO: make this nicer than just as we exit the loop
    asciify(edges);
    cvReleaseImage(&edges);
}

void
shutdown_gui() {
    cvDestroyAllWindows();
}
