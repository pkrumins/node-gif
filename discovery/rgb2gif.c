// converts RGB file to GIF (prints it to stdout)
//
#include <stdio.h>
#include <stdlib.h>
#include <gif_lib.h>

static
void rgb_to_gif(const char *filename, int width, int height)
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

    // output gif to stdout
    GifFileType *gif_file = EGifOpenFileHandle(1);
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
