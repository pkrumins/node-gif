#include <cstdio>
#include <cassert>
#include <map>

#include "common.h"
#include "quantize.h"
#include "palette.h"

int
web_safe_quantize(int width, int height,
    GifByteType *r, GifByteType *g, GifByteType *b,
    GifByteType *out)
{
    assert(width);
    assert(height);
    assert(r);
    assert(g);
    assert(b);
    assert(out);

    // naive quantization
    
    typedef std::map<int, char> IdxCache;
    IdxCache cache;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            IdxCache::iterator idx = cache.find(*r<<16 | *g<<8 | *b);
            if (idx != cache.end()) {
                *out++ = idx->second;
                r++; g++; b++;
                continue;
            }
            *out = find_closest_color(*r, *g, *b); // hidden for 255..0 loop!
            cache[*r<<16 | *g<<8 | *b] = *out;
            out++; r++; g++; b++;
        }
    }

    return GIF_OK;
}

