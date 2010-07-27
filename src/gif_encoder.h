#ifndef GIF_ENCODER_H
#define GIF_ENCODER_H

#include <gif_lib.h>

#include "common.h"

struct Gif {
    int size;
    int mem_size;
    unsigned char *gif;

    Gif();
    ~Gif();
};

class GifEncoder {
    unsigned char *data;
    int width, height;
    buffer_type buf_type;
    Gif gif;

    static int gif_writer(GifFileType *gif_file, const GifByteType *data, int size);
public:
    GifEncoder(unsigned char *ddata, int wwidth, int hheight, buffer_type bbuf_type);

    void encode();
    const unsigned char *get_gif() const;
    const int get_gif_len() const;
};

#endif

