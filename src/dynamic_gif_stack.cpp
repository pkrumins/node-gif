#include "common.h"
#include "gif_encoder.h"
#include "dynamic_gif_stack.h"
#include "buffer_compat.h"

using namespace v8;
using namespace node;

std::pair<Point, Point>
DynamicGifStack::optimal_dimension()
{
    Point top(-1, -1), bottom(-1, -1);
    for (GifUpdates::iterator it = gif_stack.begin(); it != gif_stack.end(); ++it) {
        GifUpdate *gif = *it;
        if (top.x == -1 || gif->x < top.x)
            top.x = gif->x;
        if (top.y == -1 || gif->y < top.y)
            top.y = gif->y;
        if (bottom.x == -1 || gif->x + gif->w > bottom.x)
            bottom.x = gif->x + gif->w;
        if (bottom.y == -1 || gif->y + gif->h > bottom.y)
            bottom.y = gif->y + gif->h;
    }

    return std::make_pair(top, bottom);
}

void
DynamicGifStack::construct_gif_data(unsigned char *data, Point &top)
{
    switch (buf_type) {
    case BUF_RGB:
    case BUF_RGBA:
        for (GifUpdates::iterator it = gif_stack.begin(); it != gif_stack.end(); ++it) {
            GifUpdate *gif = *it;
            int start = (gif->y - top.y)*width*3 + (gif->x - top.x)*3;
            unsigned char *gifdatap = gif->data;
            for (int i = 0; i < gif->h; i++) {
                unsigned char *datap = &data[start + i*width*3];
                for (int j = 0; j < gif->w; j++) {
                    *datap++ = *gifdatap++;
                    *datap++ = *gifdatap++;
                    *datap++ = *gifdatap++;
                    if (buf_type == BUF_RGBA) gifdatap++;
                }
            }
        }
        break;

    case BUF_BGR:
    case BUF_BGRA:
        for (GifUpdates::iterator it = gif_stack.begin(); it != gif_stack.end(); ++it) {
            GifUpdate *gif = *it;
            int start = (gif->y - top.y)*width*3 + (gif->x - top.x)*3;
            unsigned char *gifdatap = gif->data;
            for (int i = 0; i < gif->h; i++) {
                unsigned char *datap = &data[start + i*width*3];
                for (int j = 0; j < gif->w; j++) {
                    *datap++ = *(gifdatap + 2);
                    *datap++ = *(gifdatap + 1);
                    *datap++ = *gifdatap;
                    gifdatap += (buf_type == BUF_BGRA) ? 4 : 3;
                }
            }
        }
        break;

    default:
        throw "Unexpected buf_type in DynamicGifStack::GifEncode";
    }
}

void
DynamicGifStack::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", GifEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", GifEncodeSync);
    NODE_SET_PROTOTYPE_METHOD(t, "dimensions", Dimensions);
    target->Set(String::NewSymbol("DynamicGifStack"), t->GetFunction());
}

DynamicGifStack::DynamicGifStack(buffer_type bbuf_type) :
    buf_type(bbuf_type), transparency_color(0xFF, 0xFF, 0xFE) {}

DynamicGifStack::~DynamicGifStack()
{
    for (GifUpdates::iterator it = gif_stack.begin(); it != gif_stack.end(); ++it)
        delete *it;
}

Handle<Value>
DynamicGifStack::Push(unsigned char *buf_data, size_t buf_len, int x, int y, int w, int h)
{
    try {
        GifUpdate *gif_update = new GifUpdate(buf_data, buf_len, x, y, w, h);
        gif_stack.push_back(gif_update);
        return Undefined();
    }
    catch (const char *e) {
        return VException(e);
    }
}

Handle<Value>
DynamicGifStack::GifEncodeSync()
{
    HandleScope scope;

    std::pair<Point, Point> optimal = optimal_dimension();
    Point top = optimal.first, bot = optimal.second;

    offset = top;
    width = bot.x - top.x;
    height = bot.y - top.y;

    unsigned char *data = (unsigned char*)malloc(sizeof(*data)*width*height*3);
    if (!data) return VException("malloc failed in DynamicGifStack::GifEncode");

    unsigned char *datap = data;
    for (int i = 0; i < width*height*3; i+=3) {
        *datap++ = transparency_color.r;
        *datap++ = transparency_color.g;
        *datap++ = transparency_color.b;
    }

    construct_gif_data(data, top);

    try {
        GifEncoder encoder(data, width, height, BUF_RGB);
        encoder.set_transparency_color(transparency_color);
        encoder.encode();
        free(data);
        int gif_len = encoder.get_gif_len();
        Buffer *retbuf = Buffer::New(gif_len);
        memcpy(BufferData(retbuf), encoder.get_gif(), gif_len);
        return scope.Close(retbuf->handle_);
    }
    catch (const char *err) {
        return VException(err);
    }
}

Handle<Value>
DynamicGifStack::Dimensions()
{
    HandleScope scope;

    Local<Object> dim = Object::New();
    dim->Set(String::NewSymbol("x"), Integer::New(offset.x));
    dim->Set(String::NewSymbol("y"), Integer::New(offset.y));
    dim->Set(String::NewSymbol("width"), Integer::New(width));
    dim->Set(String::NewSymbol("height"), Integer::New(height));

    return scope.Close(dim);
}

Handle<Value>
DynamicGifStack::New(const Arguments &args)
{
    HandleScope scope;

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 1) {
        if (!args[0]->IsString())
            return VException("First argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[0]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
            str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return VException("First argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
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
            return VException("First argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    DynamicGifStack *gif_stack = new DynamicGifStack(buf_type);
    gif_stack->Wrap(args.This());
    return args.This();
}

Handle<Value>
DynamicGifStack::Push(const Arguments &args)
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

    DynamicGifStack *gif_stack = ObjectWrap::Unwrap<DynamicGifStack>(args.This());

    Local<Object> buf_obj = args[0]->ToObject();
    char *buf_data = BufferData(buf_obj);
    size_t buf_len = BufferLength(buf_obj);

    return scope.Close(gif_stack->Push((unsigned char*)buf_data, buf_len, x, y, w, h));
}

Handle<Value>
DynamicGifStack::Dimensions(const Arguments &args)
{
    HandleScope scope;

    DynamicGifStack *gif_stack = ObjectWrap::Unwrap<DynamicGifStack>(args.This());
    return scope.Close(gif_stack->Dimensions());
}

Handle<Value>
DynamicGifStack::GifEncodeSync(const Arguments &args)
{
    HandleScope scope;

    DynamicGifStack *gif_stack = ObjectWrap::Unwrap<DynamicGifStack>(args.This());
    return scope.Close(gif_stack->GifEncodeSync());
}

void
DynamicGifStack::EIO_GifEncode(eio_req *req)
{
    encode_request *enc_req = (encode_request *)req->data;
    DynamicGifStack *gif = (DynamicGifStack *)enc_req->gif_obj;

    std::pair<Point, Point> optimal = gif->optimal_dimension();
    Point top = optimal.first, bot = optimal.second;

    gif->offset = top;
    gif->width = bot.x - top.x;
    gif->height = bot.y - top.y;

    unsigned char *data = (unsigned char*)malloc(sizeof(*data)*gif->width*gif->height*3);
    if (!data) {
        enc_req->error = strdup("malloc failed in DynamicGifStack::EIO_GifEncode.");
        return;
    }

    unsigned char *datap = data;
    for (int i = 0; i < gif->width*gif->height; i++) {
        *datap++ = gif->transparency_color.r;
        *datap++ = gif->transparency_color.g;
        *datap++ = gif->transparency_color.b;
    }

    gif->construct_gif_data(data, top);

    buffer_type pbt = (gif->buf_type == BUF_BGR || gif->buf_type == BUF_BGRA) ?
        BUF_BGRA : BUF_RGBA;

    try {
        GifEncoder encoder(data, gif->width, gif->height, BUF_RGB);
        encoder.set_transparency_color(gif->transparency_color);
        encoder.encode();
        free(data);
        enc_req->gif_len = encoder.get_gif_len();
        enc_req->gif = (char *)malloc(sizeof(*enc_req->gif)*enc_req->gif_len);
        if (!enc_req->gif) {
            enc_req->error = strdup("malloc in DynamicGifStack::EIO_GifEncode failed.");
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
DynamicGifStack::EIO_GifEncodeAfter(eio_req *req)
{
    HandleScope scope;

    ev_unref(EV_DEFAULT_UC);
    encode_request *enc_req = (encode_request *)req->data;
    DynamicGifStack *gif = (DynamicGifStack *)enc_req->gif_obj;

    Handle<Value> argv[3];

    if (enc_req->error) {
        argv[0] = Undefined();
        argv[1] = Undefined();
        argv[2] = ErrorException(enc_req->error);
    }
    else {
        Buffer *buf = Buffer::New(enc_req->gif_len);
        memcpy(BufferData(buf), enc_req->gif, enc_req->gif_len);
        argv[0] = buf->handle_;
        argv[1] = gif->Dimensions();
        argv[2] = Undefined();
    }

    TryCatch try_catch; // don't quite see the necessity of this

    enc_req->callback->Call(Context::GetCurrent()->Global(), 3, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    enc_req->callback.Dispose();
    free(enc_req->gif);
    free(enc_req->error);

    gif->Unref();
    free(enc_req);

    return 0;
}

Handle<Value>
DynamicGifStack::GifEncodeAsync(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 1)
        return VException("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return VException("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    DynamicGifStack *gif = ObjectWrap::Unwrap<DynamicGifStack>(args.This());

    encode_request *enc_req = (encode_request *)malloc(sizeof(*enc_req));
    if (!enc_req)
        return VException("malloc in DynamicGifStack::GifEncodeAsync failed.");

    enc_req->callback = Persistent<Function>::New(callback);
    enc_req->gif_obj = gif;
    enc_req->gif = NULL;
    enc_req->gif_len = 0;
    enc_req->error = NULL;

    eio_custom(EIO_GifEncode, EIO_PRI_DEFAULT, EIO_GifEncodeAfter, enc_req);

    ev_ref(EV_DEFAULT_UC);
    gif->Ref();

    return Undefined();
}

