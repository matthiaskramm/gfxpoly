/* poly.h

Polygon intersection

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

#ifndef __poly_h__
#define __poly_h__

#include <stdint.h>
#include "gfxpoly.h"
#include "wind.h"
#include "dict.h"

/* features */
#define SPLAY
#define DONT_REMEMBER_CROSSINGS

typedef enum {EVENT_CROSS, EVENT_END, EVENT_START, EVENT_HORIZONTAL} eventtype_t;
typedef enum {SLOPE_POSITIVE, SLOPE_NEGATIVE} slope_t;

#define point_t gridpoint_t

#define INVALID_COORD (0x7fffffff)
#define SEGNR(s) ((int)((s)?(s)->nr:-1))
type_t point_type;

typedef struct _segment {
    point_t a;
    point_t b;
    point_t delta;
    double k; //k = a.x*b.y-a.y*b.x = delta.y*a.x - delta.x*a.y (=0 for points on the segment)
    int32_t minx, maxx;

    segment_dir_t dir;
    edgestyle_t*fs;
    edgestyle_t*fs_out;
#ifdef CHECKS
    char fs_out_ok;
#endif

    int polygon_nr;
    windstate_t wind;
    uintptr_t nr;

#ifdef SPLAY
    struct _segment*parent;
    struct _segment*leftchild;
    struct _segment*rightchild;
#endif
    struct _segment*left;
    struct _segment*right;
    char changed;

    point_t pos;

    gfxsegmentlist_t*stroke;
    int stroke_pos;

#ifndef DONT_REMEMBER_CROSSINGS
    dict_t scheduled_crossings;
#endif
} segment_t;

#define LINE_EQ(p,s) ((double)(s)->delta.y*(p).x - (double)(s)->delta.x*(p).y - (s)->k)

/* x1 + ((x2-x1)*(y-y1)) / dy =
   (x1*(y2-y1) + (x2-x1)*(y-y1)) / dy =
   (x1*(y2-y)  +  x2    *(y-y1)) / dy =
   (x1*y2 - x2*y1 + x2*y - y*x1) / dy =
   (k + x2*y - x1*y) / dy
   (k + dx*y) / dy
*/
//#define XPOS(s,ypos) ((s)->a.x + ((s)->delta.x * (double)((ypos) - (s)->a.y)) / (s)->delta.y)
#define XPOS(s,ypos) (((s)->k + (double)(s)->delta.x*ypos) / (s)->delta.y)

#define XPOS_INT(s,ypos) ((int)ceil(XPOS((s),ypos)))
#define XDIFF(s1,s2,ypos) (((s1)->k + (double)(s1)->delta.x*ypos)*(s2)->delta.y - \
                           ((s2)->k + (double)(s2)->delta.x*ypos)*(s1)->delta.y)

void gfxpoly_fail(char*expr, char*file, int line, const char*function);

char gfxpoly_check(gfxpoly_t*poly, char updown);
int gfxpoly_num_segments(gfxpoly_t*poly);
int gfxpoly_size(gfxpoly_t*poly);
void gfxpoly_dump(gfxpoly_t*poly);
void gfxpoly_save(gfxpoly_t*poly, const char*filename);
void gfxpoly_save_arrows(gfxpoly_t*poly, const char*filename);
gfxpoly_t* gfxpoly_process(gfxpoly_t*poly1, gfxpoly_t*poly2, windrule_t*windrule, windcontext_t*context, moments_t*moments);

gfxpoly_t* gfxpoly_intersect(gfxpoly_t*p1, gfxpoly_t*p2);
gfxpoly_t* gfxpoly_union(gfxpoly_t*p1, gfxpoly_t*p2);
double gfxpoly_area(gfxpoly_t*p);
double gfxpoly_intersection_area(gfxpoly_t*p1, gfxpoly_t*p2);

#ifndef CHECKS
#ifdef assert
#undef assert
#endif
#define assert(x)
#else
#define assert(x) ((x)?0:gfxpoly_fail(__STRING(x), __FILE__, __LINE__, __PRETTY_FUNCTION__))
#endif


#endif
