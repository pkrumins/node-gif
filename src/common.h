#ifndef COMMON_H
#define COMMON_H

#include <node.h>
#include <cstring>

v8::Handle<v8::Value> ErrorException(const char *msg);
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

struct Color {
    bool color_present; // true if this is really a color
    unsigned char r, g, b;
    Color(unsigned char rr, unsigned char gg, unsigned char bb, bool ccolor_present=true) :
        r(rr), g(gg), b(bb), color_present(ccolor_present) {}
    Color() : color_present(false) {}
};

bool str_eq(const char *s1, const char *s2);

typedef enum { BUF_RGB, BUF_BGR, BUF_RGBA, BUF_BGRA } buffer_type;

struct encode_request {
    v8::Persistent<v8::Function> callback;
    void *gif_obj;
    char *gif;
    int gif_len;
    char *error;
    char *buf_data;
};

#endif

