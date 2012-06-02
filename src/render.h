/* render.h

Polygon rendering (for debugging)

Copyright (c) 2009 Matthias Kramm <kramm@quiss.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. */

#ifndef __render_h__
#define __render_h__

#include "poly.h"
#include "wind.h"

typedef struct {
    int xmin;
    int ymin;
    int xmax;
    int ymax;
    int width;
    int height;
} intbbox_t;

unsigned char* render_polygon(gfxpoly_t*polygon, intbbox_t*bbox, double zoom, windrule_t*rule, windcontext_t*context);

intbbox_t intbbox_new(int x1, int y1, int x2, int y2);
intbbox_t intbbox_from_polygon(gfxpoly_t*polygon, double zoom);

int bitmap_ok(intbbox_t*bbox, unsigned char*data);
int compare_bitmaps(intbbox_t*bbox, unsigned char*data1, unsigned char*data2);

#endif
