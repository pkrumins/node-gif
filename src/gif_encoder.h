#ifndef GIF_ENCODER_H
#define GIF_ENCODER_H

#include <string>
#include <gif_lib.h>

#include "common.h"

struct GifImage {
    int size, mem_size;
    unsigned char *gif;

    GifImage();
    ~GifImage();
};

int gif_writer(GifFileType *gif_file, const GifByteType *data, int size);

class GifEncoder {
    unsigned char *data;
    int width, height;
    buffer_type buf_type;
    GifImage gif;
    Color transparency_color;

public:
    GifEncoder(unsigned char *ddata, int wwidth, int hheight, buffer_type bbuf_type);

    void set_transparency_color(unsigned char r, unsigned char g, unsigned char b);
    void set_transparency_color(const Color &c);

    void encode();
    const unsigned char *get_gif() const;
    const int get_gif_len() const;
};

class AnimatedGifEncoder {
    int width, height;
    buffer_type buf_type;
    GifImage gif;

    unsigned char *gif_buf;
    ColorMapObject *output_color_map;
    GifFileType *gif_file;
    int color_map_size;

    bool headers_set;
    Color transparency_color;

    std::string file_name;
    OutputFunc write_func;
    void *write_user_data;

    void end_encoding();
public:
    AnimatedGifEncoder(int wwidth, int hheight, buffer_type bbuf_type);
    ~AnimatedGifEncoder();

    void new_frame(unsigned char *data, int delay=0); // delay in 1/100s of a second
    void finish();

    void set_transparency_color(unsigned char r, unsigned char g, unsigned char b);
    void set_transparency_color(const Color &c);

    void set_output_file(const char *ffile_name);
    void set_output_func(OutputFunc func, void* user_data);

    const unsigned char *get_gif() const;
    const int get_gif_len() const;
};

class RGBator {
    GifByteType *memory;

    void rgb_to_rgb(unsigned char *data, int width, int height);
    void bgr_to_rgb(unsigned char *data, int width, int height);
    void rgba_to_rgb(unsigned char *data, int width, int height);
    void bgra_to_rgb(unsigned char *data, int width, int height);
public:
    GifByteType *red, *green, *blue;
    RGBator(unsigned char *data, int width, int height, buffer_type buf_type);
    ~RGBator();
};

#endif

