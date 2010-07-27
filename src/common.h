#ifndef COMMON_H
#define COMMON_H

#include <node.h>
#include <cstring>

v8::Handle<v8::Value> VException(const char *msg);

struct Point {
    int x, y;
    Point() {}
    Point(int xx, int yy) : x(xx), y(yy) {}
};

struct Rect {
    int x, y, w, h;
    Rect() {}
    Rect(int xx, int yy, int ww, int hh) : x(xx), y(yy), w(ww), h(hh) {}
    bool isNull() { return x == 0 && y == 0 && w == 0 && h == 0; }
};

bool str_eq(const char *s1, const char *s2);
unsigned char *rgba_to_rgb(const unsigned char *rgba, int rgba_size);
unsigned char *bgra_to_rgb(const unsigned char *rgba, int bgra_size);
unsigned char *bgr_to_rgb(const unsigned char *rgb, int rgb_size);

typedef enum { BUF_RGB, BUF_BGR, BUF_RGBA, BUF_BGRA } buffer_type;

#endif

