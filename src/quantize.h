#ifndef QUANTIZE_H
#define QUANTIZE_H

#include <gif_lib.h>

int web_safe_quantize(int width, int height,
    GifByteType *r, GifByteType *g, GifByteType *b,
    GifByteType *out);

#endif

