#ifndef NODE_GIF_H
#define NODE_GIF_H

#include <node.h>
#include <node_buffer.h>

#include "common.h"

class Gif : public node::ObjectWrap {
    int width, height;
    buffer_type buf_type;
    Color transparency_color;

    static void EIO_GifEncode(uv_work_t *req);
    static void EIO_GifEncodeAfter(uv_work_t *req, int status);

public:
    static void Initialize(v8::Handle<v8::Object> target);
    Gif(int wwidth, int hheight, buffer_type bbuf_type);
    v8::Handle<v8::Value> GifEncodeSync();
    void SetTransparencyColor(unsigned char r, unsigned char g, unsigned char b);

    static v8::Handle<v8::Value> New(const v8::Arguments &args);
    static v8::Handle<v8::Value> GifEncodeSync(const v8::Arguments &args);
    static v8::Handle<v8::Value> GifEncodeAsync(const v8::Arguments &args);
    static v8::Handle<v8::Value> SetTransparencyColor(const v8::Arguments &args);
};

#endif

