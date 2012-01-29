#include <cstdlib>
#include <cstring>
#include "common.h"
#include "gif_encoder.h"
#include "gif.h"
#include "buffer_compat.h"

using namespace v8;
using namespace node;

void
Gif::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", GifEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", GifEncodeSync);
    NODE_SET_PROTOTYPE_METHOD(t, "setTransparencyColor", SetTransparencyColor);
    target->Set(String::NewSymbol("Gif"), t->GetFunction());
}

Gif::Gif(int wwidth, int hheight, buffer_type bbuf_type) :
  width(wwidth), height(hheight), buf_type(bbuf_type) {}

Handle<Value>
Gif::GifEncodeSync()
{
    HandleScope scope;

    Local<Value> buf_val = handle_->GetHiddenValue(String::New("buffer"));

    char *buf_data = BufferData(buf_val->ToObject());

    try {
        GifEncoder encoder((unsigned char*)buf_data, width, height, buf_type);
        if (transparency_color.color_present) {
            encoder.set_transparency_color(transparency_color);
        }
        encoder.encode();
        int gif_len = encoder.get_gif_len();
        Buffer *retbuf = Buffer::New(gif_len);
        memcpy(BufferData(retbuf), encoder.get_gif(), gif_len);
        return scope.Close(retbuf->handle_);
    }
    catch (const char *err) {
        return VException(err);
    }
}

void
Gif::SetTransparencyColor(unsigned char r, unsigned char g, unsigned char b)
{
    transparency_color = Color(r, g, b, true);
}

Handle<Value>
Gif::New(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() < 3)
        return VException("At least three arguments required - data buffer, width, height, [and input buffer type]");
    if (!Buffer::HasInstance(args[0]))
        return VException("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return VException("Second argument must be integer width.");
    if (!args[2]->IsInt32())
        return VException("Third argument must be integer height.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 4) {
        if (!args[3]->IsString())
            return VException("Fourth argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[3]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
            str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return VException("Fourth argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
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
            return VException("Fourth argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }


    int w = args[1]->Int32Value();
    int h = args[2]->Int32Value();

    if (w < 0)
        return VException("Width smaller than 0.");
    if (h < 0)
        return VException("Height smaller than 0.");

    Gif *gif = new Gif(w, h, buf_type);
    gif->Wrap(args.This());

    // Save buffer.
    gif->handle_->SetHiddenValue(String::New("buffer"), args[0]);

    return args.This();
}

Handle<Value>
Gif::GifEncodeSync(const Arguments &args)
{
    HandleScope scope;

    Gif *gif = ObjectWrap::Unwrap<Gif>(args.This());
    return scope.Close(gif->GifEncodeSync());
}

Handle<Value>
Gif::SetTransparencyColor(const Arguments &args)
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

    Gif *gif = ObjectWrap::Unwrap<Gif>(args.This());
    gif->SetTransparencyColor(r, g, b);

    return Undefined();
}

void
Gif::EIO_GifEncode(eio_req *req)
{
    encode_request *enc_req = (encode_request *)req->data;
    Gif *gif = (Gif *)enc_req->gif_obj;

    try {
        GifEncoder encoder((unsigned char *)enc_req->buf_data, gif->width, gif->height, gif->buf_type);
        if (gif->transparency_color.color_present) {
            encoder.set_transparency_color(gif->transparency_color);
        }
        encoder.encode();
        enc_req->gif_len = encoder.get_gif_len();
        enc_req->gif = (char *)malloc(sizeof(*enc_req->gif)*enc_req->gif_len);
        if (!enc_req->gif) {
            enc_req->error = strdup("malloc in Gif::EIO_GifEncode failed.");
            return;
        }
        else {
            memcpy(enc_req->gif, encoder.get_gif(), enc_req->gif_len);
        }
    }
    catch (const char *err) {
        enc_req->error = strdup(err);
    }

    return;
}

int
Gif::EIO_GifEncodeAfter(eio_req *req)
{
    HandleScope scope;

    ev_unref(EV_DEFAULT_UC);
    encode_request *enc_req = (encode_request *)req->data;

    Handle<Value> argv[2];

    if (enc_req->error) {
        argv[0] = Undefined();
        argv[1] = ErrorException(enc_req->error);
    }
    else {
        Buffer *buf = Buffer::New(enc_req->gif_len);
        memcpy(BufferData(buf), enc_req->gif, enc_req->gif_len);
        argv[0] = buf->handle_;
        argv[1] = Undefined();
    }

    TryCatch try_catch; // don't quite see the necessity of this

    enc_req->callback->Call(Context::GetCurrent()->Global(), 2, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    enc_req->callback.Dispose();
    free(enc_req->gif);
    free(enc_req->error);

    ((Gif *)enc_req->gif_obj)->Unref();
    free(enc_req);

    return 0;
}

Handle<Value>
Gif::GifEncodeAsync(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 1)
        return VException("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return VException("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    Gif *gif = ObjectWrap::Unwrap<Gif>(args.This());

    encode_request *enc_req = (encode_request *)malloc(sizeof(*enc_req));
    if (!enc_req)
        return VException("malloc in Gif::GifEncodeAsync failed.");

    enc_req->callback = Persistent<Function>::New(callback);
    enc_req->gif_obj = gif;
    enc_req->gif = NULL;
    enc_req->gif_len = 0;
    enc_req->error = NULL;

    // We need to pull out the buffer data before
    // we go to the thread pool.
    Local<Value> buf_val = gif->handle_->GetHiddenValue(String::New("buffer"));

    enc_req->buf_data = BufferData(buf_val->ToObject());

    eio_custom(EIO_GifEncode, EIO_PRI_DEFAULT, EIO_GifEncodeAfter, enc_req);

    ev_ref(EV_DEFAULT_UC);
    gif->Ref();

    return Undefined();
}

