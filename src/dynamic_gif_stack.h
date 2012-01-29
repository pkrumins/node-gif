#ifndef DYNAMIC_gif_STACK_H
#define DYNAMIC_gif_STACK_H

#include <node.h>
#include <node_buffer.h>

#include <utility>
#include <vector>

#include <cstdlib>

#include "common.h"

struct GifUpdate {
    int len, x, y, w, h;
    unsigned char *data;

    GifUpdate(unsigned char *ddata, int llen, int xx, int yy, int ww, int hh) :
        len(llen), x(xx), y(yy), w(ww), h(hh)
    {
        data = (unsigned char *)malloc(sizeof(*data)*len);
        if (!data) throw "malloc failed in DynamicGifStack::GifUpdate";
        memcpy(data, ddata, len);
    }

    ~GifUpdate() {
        free(data);
    }
};

class DynamicGifStack : public node::ObjectWrap {
    typedef std::vector<GifUpdate *> GifUpdates;
    GifUpdates gif_stack;

    Point offset;
    int width, height;
    buffer_type buf_type;
    Color transparency_color;

    std::pair<Point, Point> optimal_dimension();

    static void EIO_GifEncode(eio_req *req);
    static int EIO_GifEncodeAfter(eio_req *req);
    void construct_gif_data(unsigned char *data, Point &top);

public:
    static void Initialize(v8::Handle<v8::Object> target);
    DynamicGifStack(buffer_type bbuf_type);
    ~DynamicGifStack();

    v8::Handle<v8::Value> Push(unsigned char *buf_data, size_t buf_len, int x, int y, int w, int h);
    v8::Handle<v8::Value> Dimensions();
    v8::Handle<v8::Value> GifEncodeSync();

    static v8::Handle<v8::Value> New(const v8::Arguments &args);
    static v8::Handle<v8::Value> Push(const v8::Arguments &args);
    static v8::Handle<v8::Value> Dimensions(const v8::Arguments &args);
    static v8::Handle<v8::Value> GifEncodeSync(const v8::Arguments &args);
    static v8::Handle<v8::Value> GifEncodeAsync(const v8::Arguments &args);
};

#endif

