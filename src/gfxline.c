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
    if(prev) {
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
    if(prev) {
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
    if(prev) {
        prev->next = line;
    }
    return line;
}

gfxline_t* gfxline_rewind(gfxline_t*line)
{
    while(line && line->prev)
        line = line->prev;
    return line;
}
