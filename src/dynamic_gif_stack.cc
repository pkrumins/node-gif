#include "gif_encoder.h"
#include "dynamic_gif_stack.h"

using namespace v8;
using namespace node;

std::pair<Point, Point>
DynamicGifStack::OptimalDimension()
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
DynamicGifStack::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", GifEncode);
    NODE_SET_PROTOTYPE_METHOD(t, "dimensions", Dimensions);
    target->Set(String::NewSymbol("DynamicGifStack"), t->GetFunction());
}

DynamicGifStack::DynamicGifStack(buffer_type bbuf_type) :
    buf_type(bbuf_type) {}

DynamicGifStack::~DynamicGifStack()
{
    for (GifUpdates::iterator it = gif_stack.begin(); it != gif_stack.end(); ++it)
        delete *it;
}

Handle<Value>
DynamicGifStack::Push(Buffer *buf, int x, int y, int w, int h)
{
    try {
        GifUpdate *gif_update =
            new GifUpdate((unsigned char *)buf->data(), buf->length(), x, y, w, h);
        gif_stack.push_back(gif_update);
        return Undefined();
    }
    catch (const char *e) {
        return VException(e);
    }
}

Handle<Value>
DynamicGifStack::GifEncode()
{
    HandleScope scope;

    std::pair<Point, Point> optimal = OptimalDimension();
    Point top = optimal.first, bot = optimal.second;

    offset = top;
    width = bot.x - top.x;
    height = bot.y - top.y;

    unsigned char *data = (unsigned char*)malloc(sizeof(*data)*width*height*3);
    if (!data) return VException("malloc failed in DynamicGifStack::GifEncode");
    memset(data, 0xFF, width*height*3);

    if (buf_type == BUF_RGB) {
        for (GifUpdates::iterator it = gif_stack.begin(); it != gif_stack.end(); ++it) {
            GifUpdate *gif = *it;
            int start = (gif->y - top.y)*width*3 + (gif->x - top.x)*3;
            for (int i = 0; i < gif->h; i++) {
                for (int j = 0; j < 3*gif->w; j+=3) {
                    data[start + i*width*3 + j] = gif->data[i*gif->w*3 + j];
                    data[start + i*width*3 + j + 1] = gif->data[i*gif->w*3 + j + 1];
                    data[start + i*width*3 + j + 2] = gif->data[i*gif->w*3 + j + 2];
                }
            }
        }
    }
    else if (buf_type == BUF_BGR) {
        for (GifUpdates::iterator it = gif_stack.begin(); it != gif_stack.end(); ++it) {
            GifUpdate *gif = *it;
            int start = (gif->y - top.y)*width*3 + (gif->x - top.x)*3;
            for (int i = 0; i < gif->h; i++) {
                for (int j = 0; j < 3*gif->w; j+=3) {
                    data[start + i*width*3 + j] = gif->data[i*gif->w*3 + j + 2];
                    data[start + i*width*3 + j + 1] = gif->data[i*gif->w*3 + j + 1];
                    data[start + i*width*3 + j + 2] = gif->data[i*gif->w*3 + j];
                }
            }
        }
    }
    else if (buf_type == BUF_RGBA) {
        for (GifUpdates::iterator it = gif_stack.begin(); it != gif_stack.end(); ++it) {
            GifUpdate *gif = *it;
            int start = (gif->y - top.y)*width*3 + (gif->x - top.x)*3;
            for (int i = 0; i < gif->h; i++) {
                for (int j = 0, k = 0; j < 3*gif->w; j+=3, k+=4) {
                    data[start + i*width*3 + j] = gif->data[i*gif->w*4 + k];
                    data[start + i*width*3 + j + 1] = gif->data[i*gif->w*4 + k + 1];
                    data[start + i*width*3 + j + 2] = gif->data[i*gif->w*4 + k + 2];
                }
            }
        }
    }
    else if (buf_type == BUF_BGRA) {
        for (GifUpdates::iterator it = gif_stack.begin(); it != gif_stack.end(); ++it) {
            GifUpdate *gif = *it;
            int start = (gif->y - top.y)*width*3 + (gif->x - top.x)*3;
            for (int i = 0; i < gif->h; i++) {
                for (int j = 0, k = 0; j < 3*gif->w; j+=3, k+=4) {
                    data[start + i*width*3 + j] = gif->data[i*gif->w*4 + k + 2];
                    data[start + i*width*3 + j + 1] = gif->data[i*gif->w*4 + k + 1];
                    data[start + i*width*3 + j + 2] = gif->data[i*gif->w*4 + k];
                }
            }
        }
    }

    try {
        GifEncoder gif_encoder(data, width, height, BUF_RGB);
        gif_encoder.encode();
        free(data);
        return scope.Close(
            Encode((char *)gif_encoder.get_gif(), gif_encoder.get_gif_len(), BINARY)
        );
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

    Buffer *data = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
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
    return scope.Close(gif_stack->Push(data, x, y, w, h));
}

Handle<Value>
DynamicGifStack::Dimensions(const Arguments &args)
{
    HandleScope scope;

    DynamicGifStack *gif_stack = ObjectWrap::Unwrap<DynamicGifStack>(args.This());
    return scope.Close(gif_stack->Dimensions());
}

Handle<Value>
DynamicGifStack::GifEncode(const Arguments &args)
{
    HandleScope scope;

    DynamicGifStack *gif_stack = ObjectWrap::Unwrap<DynamicGifStack>(args.This());
    return scope.Close(gif_stack->GifEncode());
}

