#ifndef _ASCIIMATIC_H_
#define _ASCIIMATIC_H_

#include <opencv/cv.h>

void init_asciimatic(const char *filename);
IplImage *detect_edges(IplImage *src, int first_thresh, int second_thresh);
void shutdown_asciimatic(void);

#endif
