#ifndef _ASCIIMATIC_H_
#define _ASCIIMATIC_H_

#ifdef __cplusplus
extern "C" {
#endif

void init_asciimatic(const char *filename, int r, int c);
void shutdown_asciimatic(void);

#ifdef __cplusplus
}  /* extern "C" */

#include <opencv2/opencv.hpp>

void asciify(cv::Mat &edges);
cv::Mat detect_edges(const cv::Mat &src);

extern cv::Mat src;
extern int first_thresh;
extern int second_thresh;

#endif  /* __cplusplus */

#endif
