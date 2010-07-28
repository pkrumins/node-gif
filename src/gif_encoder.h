#ifndef GIF_ENCODER_H
#define GIF_ENCODER_H

#include <gif_lib.h>

#include "common.h"

struct GifImage {
    int size, mem_size;
    unsigned char *gif;

    GifImage();
    ~GifImage();
};

class GifEncoder {
    unsigned char *data;
    int width, height;
    buffer_type buf_type;
    GifImage gif;
    Color transparent_color;

    static int gif_writer(GifFileType *gif_file, const GifByteType *data, int size);
public:
    GifEncoder(unsigned char *ddata, int wwidth, int hheight, buffer_type bbuf_type);

    void set_transparency_color(unsigned char r, unsigned char g, unsigned char b);
    void set_transparency_color(const Color &c);

    void encode();
    const unsigned char *get_gif() const;
    const int get_gif_len() const;
};

#endif

