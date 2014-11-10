#ifndef _ASCIIMATIC_H_
#define _ASCIIMATIC_H_

#include <opencv/cv.h>

void init_asciimatic(const char *filename);
void asciify(IplImage *edges);
IplImage *detect_edges(IplImage *dst, IplImage *src);
void shutdown_asciimatic(void);

#endif
