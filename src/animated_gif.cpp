#include <cstdlib>

#include "common.h"
#include "gif_encoder.h"
#include "animated_gif.h"

using namespace v8;
using namespace node;

void
AnimatedGif::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "endPush", EndPush);
    NODE_SET_PROTOTYPE_METHOD(t, "getGif", GetGif);
    NODE_SET_PROTOTYPE_METHOD(t, "setTransparencyColor", SetTransparencyColor);
    target->Set(String::NewSymbol("AnimatedGif"), t->GetFunction());
}

AnimatedGif::AnimatedGif(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), buf_type(bbuf_type),
    gif_encoder(wwidth, hheight, bbuf_type),
    data(NULL) {}

Handle<Value>
AnimatedGif::Push(unsigned char *data_buf, int x, int y, int w, int h)
{
    if (!data) {
        data = (unsigned char *)malloc(sizeof(*data)*width*height*3);
        if (!data) throw "malloc in AnimatedGif::Push failed";

        if (!transparency_color.color_present) {
            transparency_color = Color(0xD8, 0xA8, 0x10);
            gif_encoder.set_transparency_color(transparency_color);
        }
        unsigned char *datap = data;
        for (int i = 0; i < width*height*3; i+=3) {
            *datap++ = transparency_color.r;
            *datap++ = transparency_color.g;
            *datap++ = transparency_color.b;
        }
    }

    int start = y*width*3 + x*3;

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < 3*w; j+=3) {
            data[start + i*width*3 + j] = data_buf[i*w*3 + j];
            data[start + i*width*3 + j + 1] = data_buf[i*w*3 + j + 1];
            data[start + i*width*3 + j + 2] = data_buf[i*w*3 + j + 2];
        }
    }
}

void
AnimatedGif::EndPush()
{
    gif_encoder.new_frame(data);
    free(data);
    data = NULL;
}

Handle<Value>
AnimatedGif::New(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() < 2)
        return VException("At least two arguments required - width, height, [and input buffer type]");
    if (!args[0]->IsInt32())
        return VException("First argument must be integer width.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer height.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 3) {
        if (!args[2]->IsString())
            return VException("Third argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[3]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
            str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return VException("Third argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
        }
        
        if (str_eq(*bts, "rgb"))
            buf_type = BUF_RGB;
        else if (str_eq(*bts, "bgr"))
            buf_type = BUF_BGR;
        else if (str_eq(*bts, "rgba"))
            buf_type = BUF_RGBA;
        else if (str_eq(*bts, "bgra"))
            buf_type = BUF_BGRA;
        else
            return VException("Third argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    int w = args[0]->Int32Value();
    int h = args[1]->Int32Value();

    if (w < 0)
        return VException("Width smaller than 0.");
    if (h < 0)
        return VException("Height smaller than 0.");

    AnimatedGif *gif = new AnimatedGif(w, h, buf_type);
    gif->Wrap(args.This());
    return args.This();
}

Handle<Value>
AnimatedGif::Push(const Arguments &args)
{
    HandleScope scope;

    if (!Buffer::HasInstance(args[0]))
        return VException("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer x.");
    if (!args[2]->IsInt32())
        return VException("Third argument must be integer y.");
    if (!args[3]->IsInt32())
        return VException("Fourth argument must be integer w.");
    if (!args[4]->IsInt32())
        return VException("Fifth argument must be integer h.");

    AnimatedGif *gif = ObjectWrap::Unwrap<AnimatedGif>(args.This());
    Buffer *data_buf = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
    int x = args[1]->Int32Value();
    int y = args[2]->Int32Value();
    int w = args[3]->Int32Value();
    int h = args[4]->Int32Value();

    if (x < 0)
        return VException("Coordinate x smaller than 0.");
    if (y < 0)
        return VException("Coordinate y smaller than 0.");
    if (w < 0)
        return VException("Width smaller than 0.");
    if (h < 0)
        return VException("Height smaller than 0.");
    if (x >= gif->width) 
        return VException("Coordinate x exceeds AnimatedGif's dimensions.");
    if (y >= gif->height) 
        return VException("Coordinate y exceeds AnimatedGif's dimensions.");
    if (x+w > gif->width) 
        return VException("Pushed fragment exceeds AnimatedGif's width.");
    if (y+h > gif->height) 
        return VException("Pushed fragment exceeds AnimatedGif's height.");

    try {
        gif->Push((unsigned char *)data_buf->data(), x, y, w, h);
    }
    catch (const char *err) {
        return VException(err);
    }

    return Undefined();
}

Handle<Value>
AnimatedGif::EndPush(const Arguments &args)
{
    HandleScope scope;

    try {
        AnimatedGif *gif = ObjectWrap::Unwrap<AnimatedGif>(args.This());
        gif->EndPush();
    }
    catch (const char *err) {
        return VException(err);
    }

    return Undefined();
}

Handle<Value>
AnimatedGif::GetGif(const Arguments &args)
{
    HandleScope scope;

    AnimatedGif *gif = ObjectWrap::Unwrap<AnimatedGif>(args.This());
    gif->gif_encoder.finish();
    return scope.Close(
        Encode((char *)gif->gif_encoder.get_gif(), gif->gif_encoder.get_gif_len(), BINARY)
    );
}

Handle<Value>
AnimatedGif::SetTransparencyColor(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 3)
        return VException("Three arguments required - r, g, b");

    if (!args[0]->IsInt32())
        return VException("First argument must be integer red.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer green.");
    if (!args[2]->IsInt32())
        return VException("Third argument must be integer blue.");
    
    unsigned char r = args[0]->Int32Value();
    unsigned char g = args[1]->Int32Value();
    unsigned char b = args[2]->Int32Value();

    AnimatedGif *gif = ObjectWrap::Unwrap<AnimatedGif>(args.This());
    gif->gif_encoder.set_transparency_color(r, g, b);

    return Undefined();
}

