#include <cstdlib>

#include "common.h"
#include "gif_encoder.h"
#include "animated_gif.h"
#include "buffer_compat.h"

#include <iostream>

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
    NODE_SET_PROTOTYPE_METHOD(t, "end", End);
    NODE_SET_PROTOTYPE_METHOD(t, "setOutputFile", SetOutputFile);
    NODE_SET_PROTOTYPE_METHOD(t, "setOutputCallback", SetOutputCallback);
    target->Set(String::NewSymbol("AnimatedGif"), t->GetFunction());
}

AnimatedGif::AnimatedGif(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), buf_type(bbuf_type),
    gif_encoder(wwidth, hheight, BUF_RGB), transparency_color(0xFF, 0xFF, 0xFE),
    data(NULL)
{
    gif_encoder.set_transparency_color(transparency_color);
}

Handle<Value>
AnimatedGif::Push(unsigned char *data_buf, int x, int y, int w, int h)
{
    if (!data) {
        data = (unsigned char *)malloc(sizeof(*data)*width*height*3);
        if (!data) throw "malloc in AnimatedGif::Push failed";

        unsigned char *datap = data;
        for (int i = 0; i < width*height; i++) {
            *datap++ = transparency_color.r;
            *datap++ = transparency_color.g;
            *datap++ = transparency_color.b;
        }
    }

    int start = y*width*3 + x*3;

    unsigned char *data_bufp = data_buf;

    switch (buf_type) {
    case BUF_RGB:
        for (int i = 0; i < h; i++) {
            unsigned char *datap = &data[start + i*width*3];
            for (int j = 0; j < w; j++) {
                *datap++ = *data_bufp++;
                *datap++ = *data_bufp++;
                *datap++ = *data_bufp++;
            }
        }
        break;
    case BUF_BGR:
        for (int i = 0; i < h; i++) {
            unsigned char *datap = &data[start + i*width*3];
            for (int j = 0; j < w; j++) {
                *datap++ = *(data_bufp + 2);
                *datap++ = *(data_bufp + 1);
                *datap++ = *data_bufp;
                data_bufp += 3;
            }
        }
        break;
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

        String::AsciiValue bts(args[2]->ToString());
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
        char *buf_data = BufferData(args[0]->ToObject());

        gif->Push((unsigned char*)buf_data, x, y, w, h);
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
    int gif_len = gif->gif_encoder.get_gif_len();
    Buffer *retbuf = Buffer::New(gif_len);
    memcpy(BufferData(retbuf), gif->gif_encoder.get_gif(), gif_len);
    return scope.Close(retbuf->handle_);
}

Handle<Value>
AnimatedGif::End(const Arguments &args)
{
    HandleScope scope;

    AnimatedGif *gif = ObjectWrap::Unwrap<AnimatedGif>(args.This());
    gif->gif_encoder.finish();

    return Undefined();
}

Handle<Value>
AnimatedGif::SetOutputFile(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 1)
        return VException("One argument required - path to output file.");

    if (!args[0]->IsString())
        return VException("First argument must be string.");

    String::AsciiValue file_name(args[0]->ToString());

    AnimatedGif *gif = ObjectWrap::Unwrap<AnimatedGif>(args.This());
    gif->gif_encoder.set_output_file(*file_name);

    return Undefined();
}

int
stream_writer(GifFileType *gif_file, const GifByteType *data, int size)
{
    AnimatedGif *gif = (AnimatedGif *)gif_file->UserData;
    Buffer *retbuf = Buffer::New(size);
    memcpy(BufferData(retbuf), data, size);
    Handle<Value> argv[1] = {
      retbuf->handle_
    };
    //Local<Value> callback_v = gif->Wrap()->Get(String::New("ondata"));
    //assert(callback_v->IsFunction());
    //Local<Function> callback = Local<Function>::Cast(callback_v);
    gif->ondata->Call(Context::GetCurrent()->Global(), 1, argv);
    return size;
}

Handle<Value>
AnimatedGif::SetOutputCallback(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 1)
        return VException("One argument required - path to output file.");

    if (!args[0]->IsFunction())
        return VException("First argument must be string.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    AnimatedGif *gif = ObjectWrap::Unwrap<AnimatedGif>(args.This());
    gif->ondata = Persistent<Function>::New(callback);
    gif->gif_encoder.set_output_func(stream_writer, (void*)gif);
    return Undefined();
}
