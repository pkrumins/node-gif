#ifndef ANIMATED_GIF_H
#define ANIMATED_GIF_H

#include <node.h>
#include <node_buffer.h>

#include "gif_encoder.h"
#include "common.h"

class AnimatedGif : public node::ObjectWrap {
    int width, height;
    buffer_type buf_type;

    AnimatedGifEncoder gif_encoder;
    unsigned char *data;
    Color transparency_color;

public:

    v8::Persistent<v8::Function> ondata;

    static void Initialize(v8::Handle<v8::Object> target);

    AnimatedGif(int wwidth, int hheight, buffer_type bbuf_type);
    v8::Handle<v8::Value> Push(unsigned char *data_buf, int x, int y, int w, int h);
    void EndPush();

    static v8::Handle<v8::Value> New(const v8::Arguments &args);
    static v8::Handle<v8::Value> Push(const v8::Arguments &args);
    static v8::Handle<v8::Value> EndPush(const v8::Arguments &args);
    static v8::Handle<v8::Value> End(const v8::Arguments &args);
    static v8::Handle<v8::Value> GetGif(const v8::Arguments &args);
    static v8::Handle<v8::Value> SetOutputFile(const v8::Arguments &args);
    static v8::Handle<v8::Value> SetOutputCallback(const v8::Arguments &args);
};

#endif

