#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gif_encoder.h"

GifImage::GifImage() : size(0), mem_size(0), gif(NULL) {}
GifImage::~GifImage() { free(gif); }

GifEncoder::GifEncoder(unsigned char *ddata, int wwidth, int hheight, buffer_type bbuf_type) :
    data(ddata), width(wwidth), height(hheight), buf_type(bbuf_type) {}

int
GifEncoder::gif_writer(GifFileType *gif_file, const GifByteType *data, int size)
{
    GifImage *gif = (GifImage *)gif_file->UserData;
    if (gif->mem_size < gif->size + size) {
        GifByteType *new_ptr = (GifByteType *)realloc(gif->gif, gif->size + size + 10*1024);
        if (!new_ptr) {
            free(gif->gif);
            throw "realloc in GifEncoder::gif_writer failed";
        }
        gif->gif = new_ptr;
        gif->mem_size += size + 10*1024;
    }
    memcpy(gif->gif + gif->size, data, size);
    gif->size += size;
    return size;
}

void
GifEncoder::encode()
{
    GifByteType *rgb_mem = (GifByteType *)malloc(sizeof(GifByteType)*width*height*3);
    if (!rgb_mem)
        throw "malloc in GifEncoder::encode failed";

    GifByteType *red_buf = rgb_mem;
    GifByteType *green_buf = rgb_mem + width*height;
    GifByteType *blue_buf = rgb_mem + width*height*2;

    GifByteType *datap = data, *rp = red_buf, *gp = green_buf, *bp = blue_buf;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            *rp++ = *datap++;
            *gp++ = *datap++;
            *bp++ = *datap++;
        }
    }
    
    int color_map_size = 256;
    ColorMapObject *output_color_map = MakeMapObject(256, NULL);
    if (!output_color_map) {
        free(rgb_mem);
        throw "MakeMapObject in GifEncoder::encode failed";
    }

    GifByteType *gif_buf = (GifByteType *)malloc(sizeof(GifByteType)*width*height);
    if (!gif_buf) {
        free(rgb_mem);
        FreeMapObject(output_color_map);
        throw "malloc in GifEncoder::encode failed";
    }

    if (QuantizeBuffer(width, height, &color_map_size,
        red_buf, green_buf, blue_buf,
        gif_buf, output_color_map->Colors) == GIF_ERROR)
    {
        free(rgb_mem);
        FreeMapObject(output_color_map);
        free(gif_buf);
        throw "QuantizeBuffer in GifEncoder::encode failed";
    }
    free(rgb_mem);

    GifFileType *gif_file = EGifOpen(&gif, gif_writer);
    if (!gif_file) {
        FreeMapObject(output_color_map);
        free(gif_buf);
        throw "EGifOpen in GifEncoder::encode failed";
    }

    EGifSetGifVersion("89a");
    if (EGifPutScreenDesc(gif_file, width, height,
        color_map_size, 0, output_color_map) == GIF_ERROR)
    {
        FreeMapObject(output_color_map);
        free(gif_buf);
        EGifCloseFile(gif_file);
        throw "EGifPutScreenDesc in GifEncoder::encode failed";
    }

    /*
    char moo[] = {
        1,    // enable transparency
        0, 0, // no time delay,
        0xFF  // transparency color index
    };
    EGifPutExtension(gif_file, GRAPHICS_EXT_FUNC_CODE, 4, moo);
    */

    if (EGifPutImageDesc(gif_file, 0, 0, width, height, FALSE, NULL)
        == GIF_ERROR)
    {
        FreeMapObject(output_color_map);
        free(gif_buf);
        EGifCloseFile(gif_file);
        throw "EGifPutImageDesc in GifEncoder::encode failed";
    }

    GifByteType *gif_bufp = gif_buf;
    for (int i = 0; i < height; i++) {
        if (EGifPutLine(gif_file, gif_bufp, width) == GIF_ERROR) {
            FreeMapObject(output_color_map);
            free(gif_buf);
            EGifCloseFile(gif_file);
            throw "EGifPutLine in GifEncoder::encode failed";
        }
        gif_bufp += width;
    }

    FreeMapObject(output_color_map);
    free(gif_buf);
    EGifCloseFile(gif_file);
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

