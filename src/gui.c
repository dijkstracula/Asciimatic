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

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "asciimatic.h"
#include "logging.h"
#include "main.h"
#include "utils.h"

extern IplImage *src;
extern int first_thresh;
extern int second_thresh;

IplImage *dst;

static const char *window_name = "Asciimatic";
static const char *asciify_btn_name = "Asciify!";

static void
onAsciifyButtonPress(int state, void *p) {
    /* TODO */
}

void 
init_gui() {
    cvNamedWindow(window_name, CV_WINDOW_NORMAL);
    cvCreateTrackbar("thres1", window_name, &first_thresh, 1000, NULL);
    cvCreateTrackbar("thres2", window_name, &second_thresh, 1000, NULL);
    cvCreateButton(asciify_btn_name, onAsciifyButtonPress, (void *)asciify_btn_name, CV_PUSH_BUTTON, 0);
}

void 
gui_loop() {
    char c;
    while ((c = cvWaitKey(100)) != 27) {
        dst = detect_edges(dst, src);
        cvShowImage(window_name, src);
        cvShowImage(window_name, dst);
    }
    //TODO: make this nicer than just as we exit the loop
    asciify(dst);
    cvReleaseImage(&dst);
}

void
shutdown_gui() {
    cvDestroyAllWindows();
}
