// converts RGB file to GIF in memory
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gif_lib.h>

struct gif {
    int size;
    int mem_size;
    GifByteType *gif;
};

static int
gif_writer(GifFileType *gif_file, const GifByteType *data, int size)
{
    struct gif *gif = gif_file->UserData;
    if (gif->mem_size < gif->size + size) {
        printf("realloc to %d\n", gif->size + size + 10*1024);
        GifByteType *new_ptr = realloc(gif->gif, gif->size + size + 10*1024);
        if (!new_ptr) {
            printf("realloc failed\n");
            free(gif->gif);
            exit(1);
        }
        gif->gif = new_ptr;
        gif->mem_size += size + 10*1024;
    }
    memcpy(gif->gif + gif->size, data, size);
    gif->size += size;
    return size;
}

static void
rgb_to_gif(const char *filename, int width, int height)
{
    FILE *in = fopen(filename, "r");
    if (!in) {
        fprintf(stderr, "Error: could not open %s\n", filename);
        exit(1);
    }

    GifByteType *mem = malloc(sizeof(GifByteType)*width*height*3);
    if (!mem) {
        fprintf(stderr, "malloc failed");
        exit(1);
    }
    GifByteType *red_buf = mem;
    GifByteType *green_buf = mem + width*height;
    GifByteType *blue_buf = mem + width*height*2;

    GifByteType *buf = malloc(sizeof(GifByteType)*width*3);
    if (!buf) {
        fprintf(stderr, "malloc failed");
        exit(1);
    }

    int i, j;
    GifByteType *bufp, *rp=red_buf, *gp=green_buf, *bp=blue_buf;
    for (i = 0; i < height; i++) {
        if (fread(buf, width*3, 1, in) != 1) {
            fprintf(stderr, "fread failed");
            exit(1);
        }
        for (j = 0, bufp = buf; j < width; j++) {
            *rp++ = *bufp++;
            *gp++ = *bufp++;
            *bp++ = *bufp++;
        }
    }
    fclose(in);
    free(buf);

    int color_map_size = 256;
    ColorMapObject *output_color_map = MakeMapObject(256, NULL);
    if (!output_color_map) {
        fprintf(stderr, "MakeMapObject failed");
        exit(1);
    }
    GifByteType *output_buf = malloc(sizeof(GifByteType)*width*height);
    if (!output_buf) {
        fprintf(stderr, "malloc failed");
        exit(1);
    }

    if (QuantizeBuffer(width, height, &color_map_size,
        red_buf, green_buf, blue_buf,
        output_buf, output_color_map->Colors) == GIF_ERROR)
    {
        fprintf(stderr, "QuantizeBuffer failed");
        exit(1);
    }
    free(mem);

    // output gif to memory
    struct gif gif = {0};
    GifFileType *gif_file = EGifOpen(&gif, gif_writer);
    if (!gif_file) {
        fprintf(stderr, "EGifOpenFileHandle failed");
        exit(1);
    }

    EGifSetGifVersion("89a");
    if (EGifPutScreenDesc(gif_file, width, height, color_map_size, 0, output_color_map)
        == GIF_ERROR)
    {
        fprintf(stderr, "EGifPutScreenDesc failed");
        exit(1);
    }

    /*
    char moo[] = {
        1,    // enable transparency
        0, 0, // no time delay,
        0xFF  // transparency color index
    };
    EGifPutExtension(gif_file, GRAPHICS_EXT_FUNC_CODE, 4, moo);
    */

    if (EGifPutImageDesc(gif_file, 0, 0, width, height, FALSE, NULL) == GIF_ERROR)
    {
        fprintf(stderr, "EGifPutImageDesc failed");
        exit(1);
    }

    GifByteType *output_bufp = output_buf;
    for (i = 0; i < height; i++) {
        if (EGifPutLine(gif_file, output_bufp, width) == GIF_ERROR) {
            fprintf(stderr, "EGifPutLine failed");
            exit(1);
        }
        output_bufp += width;
    }

    free(output_buf);
    FreeMapObject(output_color_map);
    EGifCloseFile(gif_file);

    FILE *out = fopen("terminal-mem.gif", "w");
    if (!out) {
        fprintf(stderr, "Failed opening terminal-mem.gif\n");
        exit(1);
    }
    fwrite(gif.gif, gif.size, sizeof(GifByteType), out);
    fclose(out);
    free(gif.gif);
}

int
main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: rgb2gif <rgb file> <width> <height>\n");
        exit(1);
    }
    char *filename = argv[1];
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);

    printf("Converting %s (%dx%d) from RGB to GIF.\n", filename, width, height);
    rgb_to_gif(filename, width, height);
}
