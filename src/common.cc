#include <cstdlib>
#include <cassert>
#include "common.h"

using namespace v8;

Handle<Value>
VException(const char *msg) {
    HandleScope scope;
    return ThrowException(Exception::Error(String::New(msg)));
}

bool str_eq(const char *s1, const char *s2)
{
        return strcmp(s1, s2) == 0;
}

unsigned char *
rgba_to_rgb(const unsigned char *rgba, int rgba_size)
{
    assert(rgba_size%4==0);

    int rgb_size = rgba_size*3/4;
    unsigned char *rgb = (unsigned char *)malloc(sizeof(*rgb)*rgb_size);
    if (!rgb) return NULL;

    for (int i=0,j=0; i<rgba_size; i+=4,j+=3) {
        rgb[j] = rgba[i];
        rgb[j+1] = rgba[i+1];
        rgb[j+2] = rgba[i+2];
    }
    return rgb;
}

unsigned char *
bgra_to_rgb(const unsigned char *bgra, int bgra_size)
{
    assert(bgra_size%4==0);

    int rgb_size = bgra_size*3/4;
    unsigned char *rgb = (unsigned char *)malloc(sizeof(*rgb)*rgb_size);
    if (!rgb) return NULL;

    for (int i=0,j=0; i<bgra_size; i+=4,j+=3) {
        rgb[j] = bgra[i+2];
        rgb[j+1] = bgra[i+1];
        rgb[j+2] = bgra[i];
    }
    return rgb;
}

unsigned char *
bgr_to_rgb(const unsigned char *bgr, int bgr_size)
{
    assert(bgr_size%3==0);

    unsigned char *rgb = (unsigned char *)malloc(sizeof(*rgb)*bgr_size);
    if (!rgb) return NULL;

    for (int i=0; i<bgr_size; i+=3) {
        rgb[i] = bgr[i+2];
        rgb[i+1] = bgr[i+1];
        rgb[i+2] = bgr[i];
    }
    return rgb;
}

