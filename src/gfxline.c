/* gfxline.c

Utility functions for linked lists of lines and splines (gfxline_t struct).

Copyright (c) 2012 Matthias Kramm <kramm@quiss.org>
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

#include <stdlib.h>
#include "gfxpoly.h"
#include "gfxline.h"

gfxline_t* gfxline_new()
{
    return NULL; // list is initially empty
}

gfxline_t* gfxline_moveTo(gfxline_t*prev, gfxcoord_t x, gfxcoord_t y)
{
    gfxline_t*line = malloc(sizeof(gfxline_t));
    line->type = gfx_moveTo;
    line->x = x;
    line->y = y;
    line->prev = prev;
    line->next = NULL;
    if (prev) {
        prev->next = line;
    }
    return line;
}

gfxline_t* gfxline_lineTo(gfxline_t*prev, gfxcoord_t x, gfxcoord_t y)
{
    gfxline_t*line = malloc(sizeof(gfxline_t));
    line->type = gfx_lineTo;
    line->x = x;
    line->y = y;
    line->prev = prev;
    line->next = NULL;
    if (prev) {
        prev->next = line;
    }
    return line;
}

gfxline_t* gfxline_splineTo(gfxline_t*prev, gfxcoord_t sx, gfxcoord_t sy, gfxcoord_t x, gfxcoord_t y)
{
    gfxline_t*line = malloc(sizeof(gfxline_t));
    line->type = gfx_splineTo;
    line->x = x;
    line->y = y;
    line->sx = sx;
    line->sy = sy;
    line->prev = prev;
    line->next = NULL;
    if (prev) {
        prev->next = line;
    }
    return line;
}

gfxline_t* gfxline_rewind(gfxline_t*line)
{
    while (line && line->prev)
        line = line->prev;
    return line;
}
