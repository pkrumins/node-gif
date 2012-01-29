#include <cerrno>
#include <cstdlib>

#include "common.h"
#include "utils.h"
#include "gif_encoder.h"
#include "async_animated_gif.h"
#include "buffer_compat.h"

#include "loki/ScopeGuard.h"

using namespace v8;
using namespace node;

void
AsyncAnimatedGif::Initialize(Handle<Object> target)
{
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "endPush", EndPush);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", Encode);
    NODE_SET_PROTOTYPE_METHOD(t, "setOutputFile", SetOutputFile);
    NODE_SET_PROTOTYPE_METHOD(t, "setTmpDir", SetTmpDir);
    target->Set(String::NewSymbol("AsyncAnimatedGif"), t->GetFunction());
}

AsyncAnimatedGif::AsyncAnimatedGif(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), buf_type(bbuf_type),
    transparency_color(0xFF, 0xFF, 0xFE),
    push_id(0), fragment_id(0) {}

void
AsyncAnimatedGif::EIO_Push(eio_req *req)
{
    push_request *push_req = (push_request *)req->data;

    if (!is_dir(push_req->tmp_dir)) {
        if (mkdir(push_req->tmp_dir, 0775) == -1) {
            // there is no way to return this error to node as this call was
            // async with no callback
            fprintf(stderr, "Could not mkdir(%s) in AsyncAnimatedGif::EIO_Push.\n",
                push_req->tmp_dir);
            return;
        }
    }

    char fragment_dir[512];
    snprintf(fragment_dir, 512, "%s/%d", push_req->tmp_dir, push_req->push_id);

    if (!is_dir(fragment_dir)) {
        if (mkdir(fragment_dir, 0775) == -1) {
            fprintf(stderr, "Could not mkdir(%s) in AsyncAnimatedGif::EIO_Push.\n",
                fragment_dir);
            return;
        }
    }

    char filename[512];
    snprintf(filename, 512, "%s/%d/rect-%d-%d-%d-%d-%d.dat",
        push_req->tmp_dir, push_req->push_id, push_req->fragment_id,
        push_req->x, push_req->y, push_req->w, push_req->h);
    FILE *out = fopen(filename, "w+");
    LOKI_ON_BLOCK_EXIT(fclose, out);
    if (!out) {
        fprintf(stderr, "Failed to open %s in AsyncAnimatedGif::EIO_Push.\n",
            filename);
        return;
    }
    int written = fwrite(push_req->data, sizeof(unsigned char), push_req->data_size, out);
    if (written != push_req->data_size) {
        fprintf(stderr, "Failed to write all data to %s. Wrote only %d of %d.\n",
            filename, written, push_req->data_size);
    }

    return;
}

int
AsyncAnimatedGif::EIO_PushAfter(eio_req *req)
{
    ev_unref(EV_DEFAULT_UC);

    push_request *push_req = (push_request *)req->data;
    free(push_req->data);
    free(push_req);

    return 0;
}

Handle<Value>
AsyncAnimatedGif::Push(unsigned char *data_buf, int x, int y, int w, int h)
{
    HandleScope scope;

    if (tmp_dir.empty())
        throw "Tmp dir is not set. Use .setTmpDir to set it before pushing.";

    if (output_file.empty())
        throw "Output file is not set. Use .setOutputFile to set it before pushing.";

    push_request *push_req = (push_request *)malloc(sizeof(*push_req));
    if (!push_req)
        throw "malloc in AsyncAnimatedGif::Push failed.";

    push_req->data = (unsigned char *)malloc(sizeof(*push_req->data)*w*h*3);
    if (!push_req->data) {
        free(push_req);
        throw "malloc in AsyncAnimatedGif::Push failed.";
    }

    memcpy(push_req->data, data_buf, w*h*3);
    push_req->push_id = push_id;
    push_req->fragment_id = fragment_id++;
    push_req->tmp_dir = tmp_dir.c_str();
    push_req->data_size = w*h*3;
    push_req->x = x;
    push_req->y = y;
    push_req->w = w;
    push_req->h = h;

    eio_custom(EIO_Push, EIO_PRI_DEFAULT, EIO_PushAfter, push_req);
    ev_ref(EV_DEFAULT_UC);

    return Undefined();
}

void
AsyncAnimatedGif::EndPush()
{
    push_id++;
    fragment_id = 0;
}

Handle<Value>
AsyncAnimatedGif::New(const Arguments &args)
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

    AsyncAnimatedGif *gif = new AsyncAnimatedGif(w, h, buf_type);
    gif->Wrap(args.This());
    return args.This();
}

Handle<Value>
AsyncAnimatedGif::Push(const Arguments &args)
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

    AsyncAnimatedGif *gif = ObjectWrap::Unwrap<AsyncAnimatedGif>(args.This());
    //    Buffer *data_buf = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
    //    unsigned char *buf = (unsigned char *)data_buf->data();
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
        return VException("Coordinate x exceeds AsyncAnimatedGif's dimensions.");
    if (y >= gif->height)
        return VException("Coordinate y exceeds AsyncAnimatedGif's dimensions.");
    if (x+w > gif->width)
        return VException("Pushed fragment exceeds AsyncAnimatedGif's width.");
    if (y+h > gif->height)
        return VException("Pushed fragment exceeds AsyncAnimatedGif's height.");

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
AsyncAnimatedGif::EndPush(const Arguments &args)
{
    HandleScope scope;

    AsyncAnimatedGif *gif = ObjectWrap::Unwrap<AsyncAnimatedGif>(args.This());
    gif->EndPush();

    return Undefined();
}

int
fragment_sort(const void *aa, const void *bb)
{
    const char *a = *(const char **)aa;
    const char *b = *(const char **)bb;
    int na, nb;
    sscanf(a, "rect-%d", &na);
    sscanf(b, "rect-%d", &nb);
    return na > nb;
}

unsigned char *
AsyncAnimatedGif::init_frame(int width, int height, Color &transparency_color)
{
    unsigned char *frame = (unsigned char *)malloc(sizeof(*frame)*width*height*3);
    if (!frame) return NULL;

    unsigned char *framgep = frame;
    for (int i = 0; i < width*height; i++) {
        *framgep++ = transparency_color.r;
        *framgep++ = transparency_color.g;
        *framgep++ = transparency_color.b;
    }

    return frame;
}

void
AsyncAnimatedGif::push_fragment(unsigned char *frame, int width, int height,
    buffer_type buf_type, unsigned char *fragment, int x, int y, int w, int h)
{
    int start = y*width*3 + x*3;
    unsigned char *fragmentp = fragment;

    switch (buf_type) {
    case BUF_RGB:
        for (int i = 0; i < h; i++) {
            unsigned char *framep = &frame[start + i*width*3];
            for (int j = 0; j < w; j++) {
                *framep++ = *fragment++;
                *framep++ = *fragment++;
                *framep++ = *fragment++;
            }
        }
        break;
    case BUF_BGR:
        for (int i = 0; i < h; i++) {
            unsigned char *framep = &frame[start + i*width*3];
            for (int j = 0; j < w; j++) {
                *framep++ = *(fragment + 2);
                *framep++ = *(fragment + 1);
                *framep++ = *fragment;
                framep += 3;
            }
        }
        break;
    }
}

Rect
AsyncAnimatedGif::rect_dims(const char *fragment_name)
{
    int moo, x, y, w, h;
    sscanf(fragment_name, "rect-%d-%d-%d-%d-%d", &moo, &x, &y, &w, &h);
    return Rect(x, y, w, h);
}

void
AsyncAnimatedGif::EIO_Encode(eio_req *req)
{
    async_encode_request *enc_req = (async_encode_request *)req->data;
    AsyncAnimatedGif *gif = (AsyncAnimatedGif *)enc_req->gif_obj;

    AnimatedGifEncoder encoder(gif->width, gif->height, BUF_RGB);
    encoder.set_output_file(gif->output_file.c_str());
    encoder.set_transparency_color(gif->transparency_color);

    for (size_t push_id = 0; push_id < gif->push_id; push_id++) {
        char fragment_path[512];
        snprintf(fragment_path, 512, "%s/%d", gif->tmp_dir.c_str(), push_id);
        if (!is_dir(fragment_path)) {
            char error[600];
            snprintf(error, 600, "Error in AsyncAnimatedGif::EIO_Encode %s is not a dir.",
                fragment_path);
            enc_req->error = strdup(error);
            return;
        }

        char **fragments = find_files(fragment_path);
        LOKI_ON_BLOCK_EXIT(free_file_list, fragments);
        int nfragments = file_list_length(fragments);

        qsort(fragments, nfragments, sizeof(char *), fragment_sort);

        unsigned char *frame = init_frame(gif->width, gif->height, gif->transparency_color);
        LOKI_ON_BLOCK_EXIT(free, frame);
        if (!frame) {
            enc_req->error = strdup("malloc failed in AsyncAnimatedGif::EIO_Encode.");
            return;
        }

        for (int i = 0; i < nfragments; i++) {
            snprintf(fragment_path, 512, "%s/%d/%s",
                gif->tmp_dir.c_str(), push_id, fragments[i]);
            FILE *in = fopen(fragment_path, "r");
            if (!in) {
                char error[600];
                snprintf(error, 600, "Failed opening %s in AsyncAnimatedGif::EIO_Encode.",
                    fragment_path);
                enc_req->error = strdup(error);
                return;
            }
            LOKI_ON_BLOCK_EXIT(fclose, in);
            int size = file_size(fragment_path);
            unsigned char *data = (unsigned char *)malloc(sizeof(*data)*size);
            LOKI_ON_BLOCK_EXIT(free, data);
            int read = fread(data, sizeof *data, size, in);
            if (read != size) {
                char error[600];
                snprintf(error, 600, "Error - should have read %d but read only %d from %s in AsyncAnimatedGif::EIO_Encode", size, read, fragment_path);
                enc_req->error = strdup(error);
                return;
            }
            Rect dims = rect_dims(fragments[i]);
            push_fragment(frame, gif->width, gif->height, gif->buf_type,
                data, dims.x, dims.y, dims.w, dims.h);
        }
        encoder.new_frame(frame);
    }
    encoder.finish();

    return;
}

int
AsyncAnimatedGif::EIO_EncodeAfter(eio_req *req)
{
    HandleScope scope;

    ev_unref(EV_DEFAULT_UC);
    async_encode_request *enc_req = (async_encode_request *)req->data;

    Handle<Value> argv[2];

    if (enc_req->error) {
        argv[0] = False();
        argv[1] = ErrorException(enc_req->error);
    }
    else {
        argv[0] = True();
        argv[1] = Undefined();
    }

    TryCatch try_catch; // don't quite see the necessity of this

    enc_req->callback->Call(Context::GetCurrent()->Global(), 2, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    enc_req->callback.Dispose();

    enc_req->gif_obj->Unref();
    free(enc_req);

    return 0;
}

Handle<Value>
AsyncAnimatedGif::Encode(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 1)
        return VException("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return VException("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    AsyncAnimatedGif *gif = ObjectWrap::Unwrap<AsyncAnimatedGif>(args.This());

    async_encode_request *enc_req = (async_encode_request *)malloc(sizeof(*enc_req));
    if (!enc_req)
        return VException("malloc in AsyncAnimatedGif::Encode failed.");

    enc_req->callback = Persistent<Function>::New(callback);
    enc_req->gif_obj = gif;
    enc_req->error = NULL;

    eio_custom(EIO_Encode, EIO_PRI_DEFAULT, EIO_EncodeAfter, enc_req);

    ev_ref(EV_DEFAULT_UC);
    gif->Ref();

    return Undefined();
}

Handle<Value>
AsyncAnimatedGif::SetOutputFile(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 1)
        return VException("One argument required - path to output file.");

    if (!args[0]->IsString())
        return VException("First argument must be string.");

    String::AsciiValue file_name(args[0]->ToString());

    AsyncAnimatedGif *gif = ObjectWrap::Unwrap<AsyncAnimatedGif>(args.This());
    gif->output_file = *file_name;

    return Undefined();
}

Handle<Value>
AsyncAnimatedGif::SetTmpDir(const Arguments &args)
{
    HandleScope scope;

    if (args.Length() != 1)
        return VException("One argument required - path to tmp dir.");

    if (!args[0]->IsString())
        return VException("First argument must be string.");

    String::AsciiValue tmp_dir(args[0]->ToString());

    AsyncAnimatedGif *gif = ObjectWrap::Unwrap<AsyncAnimatedGif>(args.This());
    gif->tmp_dir = *tmp_dir;

    return Undefined();
}

