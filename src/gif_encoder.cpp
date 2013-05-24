#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "loki/ScopeGuard.h"

#include "gif_encoder.h"
#include "palette.h"
#include "quantize.h"

static int
find_color_index(ColorMapObject *color_map, int color_map_size, Color &color)
{
    for (int i = 255; i >= 0; i--) { // cause our transparent color for now is at 255!
        /*
        printf("%d: %02x %02x %02x\n", i, color_map->Colors[i].Red,
            color_map->Colors[i].Green, color_map->Colors[i].Blue);
        */
        if (color_map->Colors[i].Red == color.r &&
            color_map->Colors[i].Green == color.g &&
            color_map->Colors[i].Blue == color.b)
        {
            return i;
        }
    }
    return -1;
}

static int
nearest_pow2(int n)
{
    return (int)pow(2, ceil(log2(n)));
}

GifImage::GifImage() : size(0), mem_size(0), gif(NULL) {}
GifImage::~GifImage() { free(gif); }

GifEncoder::GifEncoder(unsigned char *ddata, int wwidth, int hheight, buffer_type bbuf_type) :
    data(ddata), width(wwidth), height(hheight), buf_type(bbuf_type) {}

RGBator::RGBator(unsigned char *data, int width, int height, buffer_type buf_type) {
    memory = (GifByteType *)malloc(sizeof(GifFileType)*width*height*3);
    if (!memory) throw "malloc in RGBator::RGBator failed";
    red = memory;
    green = memory + width*height;
    blue = memory + width*height*2;

    switch (buf_type) {
    case BUF_RGB:
        rgb_to_rgb(data, width, height);
        break;
    case BUF_BGR:
        bgr_to_rgb(data, width, height);
        break;
    case BUF_RGBA:
        rgba_to_rgb(data, width, height);
        break;
    case BUF_BGRA:
        bgra_to_rgb(data, width, height);
        break;
    default:
        throw "Unexpected buf_type in RGBator::RGBator";
    }
};

RGBator::~RGBator() { free(memory); }

void
RGBator::rgb_to_rgb(unsigned char *data, int width, int height)
{
    GifByteType *datap = data;
    GifByteType *rp = red, *gp = green, *bp = blue;
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            *rp++ = *datap++;
            *gp++ = *datap++;
            *bp++ = *datap++;
        }
    }
}

void
RGBator::bgr_to_rgb(unsigned char *data, int width, int height)
{
    GifByteType *datap = data;
    GifByteType *rp = red, *gp = green, *bp = blue;
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            *rp++ = *(datap + 2);
            *gp++ = *(datap + 1);
            *bp++ = *datap;
            datap += 3;
        }
    }
}

void
RGBator::rgba_to_rgb(unsigned char *data, int width, int height)
{
    GifByteType *datap = data;
    GifByteType *rp = red, *gp = green, *bp = blue;
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            *rp++ = *datap++;
            *gp++ = *datap++;
            *bp++ = *datap++;
            datap++;
        }
    }
}

void
RGBator::bgra_to_rgb(unsigned char *data, int width, int height)
{
    GifByteType *datap = data;
    GifByteType *rp = red, *gp = green, *bp = blue;
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            *rp++ = *(datap + 2);
            *gp++ = *(datap + 1);
            *bp++ = *datap;
            datap += 4;
        }
    }
}

int
gif_writer(GifFileType *gif_file, const GifByteType *data, int size)
{
    GifImage *gif = (GifImage *)gif_file->UserData;
    if (gif->size + size > gif->mem_size) {
        GifByteType *new_ptr = (GifByteType *)realloc(gif->gif, gif->size + size + 10*1024);
        if (!new_ptr)
            throw "realloc in gif_writer failed";
        gif->gif = new_ptr;
        gif->mem_size = gif->size + size + 10*1024;
    }
    memcpy(gif->gif + gif->size, data, size);
    gif->size += size;
    return size;
}

void
GifEncoder::encode()
{
    RGBator rgb(data, width, height, buf_type);

    int color_map_size = 256;
    ColorMapObject *output_color_map = GifMakeMapObject(256, ext_web_safe_palette);
    LOKI_ON_BLOCK_EXIT(GifFreeMapObject, output_color_map);
    if (!output_color_map)
        throw "MakeMapObject in GifEncoder::encode failed";

    GifByteType *gif_buf = (GifByteType *)malloc(sizeof(GifByteType)*width*height);
    LOKI_ON_BLOCK_EXIT(free, gif_buf);
    if (!gif_buf)
        throw "malloc in GifEncoder::encode failed";

    if (web_safe_quantize(width, height, rgb.red, rgb.green, rgb.blue, gif_buf) == GIF_ERROR)
        throw "web_safe_quantize in GifEncoder::encode failed";

    /*
    if (QuantizeBuffer(width, height, &color_map_size,
        rgb.red, rgb.green, rgb.blue,
        gif_buf, output_color_map->Colors) == GIF_ERROR)
    {
        throw "QuantizeBuffer in GifEncoder::encode failed";
    }
    */

    int nError;
    GifFileType *gif_file = EGifOpen(&gif, gif_writer, &nError);
    LOKI_ON_BLOCK_EXIT(EGifCloseFile, gif_file);
    if (!gif_file)
        throw "EGifOpen in GifEncoder::encode failed";

    if (EGifPutScreenDesc(gif_file, width, height,
        color_map_size, 0, output_color_map) == GIF_ERROR)
    {
        throw "EGifPutScreenDesc in GifEncoder::encode failed";
    }

    if (transparency_color.color_present) {
        int i = find_color_index(output_color_map, color_map_size, transparency_color);
        if (i) {
            char extension[] = {
                1, // enable transparency
                0, 0, // no time delay
                i // transparency color index
            };
            EGifPutExtension(gif_file, GRAPHICS_EXT_FUNC_CODE, 4, extension);
        }
    }

    if (EGifPutImageDesc(gif_file, 0, 0, width, height, FALSE, NULL) == GIF_ERROR) {
        throw "EGifPutImageDesc in GifEncoder::encode failed";
    }

    GifByteType *gif_bufp = gif_buf;
    for (int i = 0; i < height; i++) {
        if (EGifPutLine(gif_file, gif_bufp, width) == GIF_ERROR)
            throw "EGifPutLine in GifEncoder::encode failed";
        gif_bufp += width;
    }
}

void
GifEncoder::set_transparency_color(unsigned char r, unsigned char g, unsigned char b)
{
    transparency_color.r = r;
    transparency_color.r = g;
    transparency_color.r = b;
    transparency_color.color_present = true;
}

void
GifEncoder::set_transparency_color(const Color &c)
{
    transparency_color = c;
}

const unsigned char *
GifEncoder::get_gif() const
{
    return gif.gif;
}

const int
GifEncoder::get_gif_len() const
{
    return gif.size;
}

// Animated Gif Encoder
AnimatedGifEncoder::AnimatedGifEncoder(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), buf_type(bbuf_type),
    gif_buf(NULL), output_color_map(NULL), gif_file(NULL), color_map_size(256), write_func(0), write_user_data(0),
    headers_set(false) {}

AnimatedGifEncoder::~AnimatedGifEncoder() { end_encoding(); }

void
AnimatedGifEncoder::end_encoding() {
    free(gif_buf);
    gif_buf = NULL;
    if (output_color_map) {
        GifFreeMapObject(output_color_map);
        output_color_map = NULL;
    }
    if (gif_file) {
        EGifCloseFile(gif_file);
        gif_file = NULL;
    }
}

void
AnimatedGifEncoder::new_frame(unsigned char *data, int delay)
{
    if (!gif_file) {
       if (write_func != NULL) {
            int nError;
            gif_file = EGifOpen(write_user_data, write_func, &nError);
            if (!gif_file) throw "EGifOpen in AnimatedGifEncoder::new_frame failed";
        } else if (file_name.empty()) { // memory writer
            int nError;
            gif_file = EGifOpen(&gif, gif_writer, &nError);
            if (!gif_file) throw "EGifOpen in AnimatedGifEncoder::new_frame failed";
        } else {
            int nError;
            gif_file = EGifOpenFileName(file_name.c_str(), FALSE, &nError);
            if (!gif_file) throw "EGifOpenFileName in AnimatedGifEncoder::new_frame failed";
        }
        output_color_map = GifMakeMapObject(color_map_size, ext_web_safe_palette);
        if (!output_color_map) throw "MakeMapObject in AnimatedGifEncoder::new_frame failed";

        gif_buf = (GifByteType *)malloc(sizeof(GifByteType)*width*height);
        if (!gif_buf) throw "malloc in AnimatedGifEncoder::new_frame failed";
    }

    RGBator rgb(data, width, height, buf_type);

    if (web_safe_quantize(width, height, rgb.red, rgb.green, rgb.blue, gif_buf) == GIF_ERROR)
        throw "web_safe_quantize in AnimatedGifEncoder::new_frame failed";

    /*
    if (QuantizeBuffer(width, height, &color_map_size,
        rgb.red, rgb.green, rgb.blue,
        gif_buf, output_color_map->Colors) == GIF_ERROR)
    {
        throw "QuantizeBuffer in AnimatedGifEncoder::new_frame failed";
    }
    */

    if (!headers_set) {
        if (EGifPutScreenDesc(gif_file, width, height,
            color_map_size, 0, output_color_map) == GIF_ERROR)
        {
            throw "EGifPutScreenDesc in AnimatedGifEncoder::new_frame failed";
        }
        char netscape_extension[] = "NETSCAPE2.0";
        EGifPutExtension(gif_file, APPLICATION_EXT_FUNC_CODE, 11, netscape_extension);
        char animation_extension[] = { 1, 1, 0 }; // repeat one time
        EGifPutExtension(gif_file, APPLICATION_EXT_FUNC_CODE, 3, animation_extension);
        headers_set = true;
    }

    char frame_flags = 1 << 2;
    char transp_color_idx = 0;
    if (transparency_color.color_present) {
        int i = find_color_index(output_color_map, color_map_size, transparency_color);
        if (i>=0) {
            frame_flags |= 1;
            transp_color_idx = i;
        }
    }

    char extension[] = {
        frame_flags,
        delay%256, delay/256,
        transp_color_idx
    };
    EGifPutExtension(gif_file, GRAPHICS_EXT_FUNC_CODE, 4, extension);

    if (EGifPutImageDesc(gif_file, 0, 0, width, height, FALSE, NULL) == GIF_ERROR) {
        throw "EGifPutImageDesc in AnimatedGifEncoder::new_frame failed";
    }

    GifByteType *gif_bufp = gif_buf;
    for (int i = 0; i < height; i++) {
        if (EGifPutLine(gif_file, gif_bufp, width) == GIF_ERROR) {
            throw "EGifPutLine in AnimatedGifEncoder::new_frame failed";
        }
        gif_bufp += width;
    }
}

void
AnimatedGifEncoder::finish()
{
    end_encoding();
}

void
AnimatedGifEncoder::set_transparency_color(unsigned char r, unsigned char g, unsigned char b)
{
    transparency_color.r = r;
    transparency_color.r = g;
    transparency_color.r = b;
    transparency_color.color_present = true;
}

void
AnimatedGifEncoder::set_transparency_color(const Color &c)
{
    transparency_color = c;
}

const unsigned char *
AnimatedGifEncoder::get_gif() const
{
    return gif.gif;
}

const int
AnimatedGifEncoder::get_gif_len() const
{
    return gif.size;
}

void
AnimatedGifEncoder::set_output_file(const char *ffile_name)
{
    file_name = ffile_name;
}

void
AnimatedGifEncoder::set_output_func(OutputFunc func, void *user_data)
{
   write_func = func;
   write_user_data = user_data;
}
