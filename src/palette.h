#ifndef PALETTE_H
#define PALETTE_H

#include <gif_lib.h>

extern GifColorType ext_web_safe_palette[256];

int find_closest_color(int r, int g, int b);

#endif

