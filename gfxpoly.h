/* gfxpoly.h

Boolean polygon operations library

Copyright (c) 2009-2012 Matthias Kramm <kramm@quiss.org>
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

#ifndef __gfxpoly_h__
#define __gfxpoly_h__

#include <stdint.h>

/* +----------------------------------------------------------------+ */
/* |                           Definitions                          | */
/* +----------------------------------------------------------------+ */

/* A "grid" value is the granularity at which polygon intersection operates.
   It usually makes sense to set this to the smallest value that can actually be represented
   in the output device (e.g. 0.05 for Flash animations).  */
#define DEFAULT_GRID (0.05)

/* A coordinate on the grid. This is represented as integers, and the real coordinate
   can be derived by multiplying each component with the grid size */
typedef struct _gridpoint {
    int32_t x;
    int32_t y;
} gridpoint_t;

/* A coordinate in the original space (i.e., not on the grid, and hence represented
   as floating point number */
typedef double gfxcoord_t;

/* a bounding box (in real coordinate space) */
typedef struct _gfxbbox {
    gfxcoord_t x1,y1,x2,y2;
} gfxbbox_t;

/* Every segment has an original direction, which is the direction
   the segment had in the input data.
   as our scanline moves from minimum y to maximum y, "DOWN" means
   the the (original) segment's y2 is larger than its y1 */
typedef enum {DIR_UP, DIR_DOWN, DIR_UNKNOWN} segment_dir_t;

typedef struct _edgestyle {
    void*internal;
} edgestyle_t;

/* +----------------------------------------------------------------+ */
/* |                          gfxpoly_t objects                     | */
/* +----------------------------------------------------------------+ */

typedef struct _gfxsegmentlist {
    segment_dir_t dir;
    edgestyle_t*fs;
    int points_size;
    int num_points;
    gridpoint_t*points;
    struct _gfxsegmentlist*next;
} gfxsegmentlist_t;

typedef struct _gfxpoly {
    double gridsize;
    gfxsegmentlist_t*strokes;
} gfxpoly_t;

gfxbbox_t gfxpoly_calculate_bbox(gfxpoly_t*poly);
void gfxpoly_destroy(gfxpoly_t*poly);

/* +----------------------------------------------------------------+ */
/* |                            Operators                           | */
/* +----------------------------------------------------------------+ */

gfxpoly_t* gfxpoly_intersect(gfxpoly_t*p1, gfxpoly_t*p2);
gfxpoly_t* gfxpoly_union(gfxpoly_t*p1, gfxpoly_t*p2);

gfxpoly_t* gfxpoly_selfintersect_evenodd(gfxpoly_t*p);
gfxpoly_t* gfxpoly_selfintersect_circular(gfxpoly_t*p);

/* +----------------------------------------------------------------+ */
/* |                         Area and Moments                       | */
/* +----------------------------------------------------------------+ */

typedef struct _moments {
    double area;
    double m[3][3];
} moments_t;

double gfxpoly_area(gfxpoly_t*p);
double gfxpoly_intersection_area(gfxpoly_t*p1, gfxpoly_t*p2);
moments_t gfxpoly_moments(gfxpoly_t*p);

/* +----------------------------------------------------------------+ */
/* |     Conversion from curves and floating point coordinates      | */
/* +----------------------------------------------------------------+ */

typedef enum {gfx_moveTo, gfx_lineTo, gfx_splineTo} gfx_linetype;
typedef enum {gfx_joinMiter, gfx_joinRound, gfx_joinBevel} gfx_joinType;
typedef enum {gfx_capButt, gfx_capRound, gfx_capSquare} gfx_capType;

typedef struct _gfxline {
    gfx_linetype type;
    gfxcoord_t x,y;
    gfxcoord_t sx,sy;
    struct _gfxline*prev;
    struct _gfxline*next;
} gfxline_t;

gfxline_t* gfxline_new();
gfxline_t* gfxline_moveTo(gfxline_t*line, gfxcoord_t x, gfxcoord_t y);
gfxline_t* gfxline_lineTo(gfxline_t*line, gfxcoord_t x, gfxcoord_t y);
gfxline_t* gfxline_splineTo(gfxline_t*line, gfxcoord_t sx, gfxcoord_t sy, gfxcoord_t x, gfxcoord_t y);

gfxpoly_t* gfxpoly_from_fill(gfxline_t*line, double gridsize);
gfxpoly_t* gfxpoly_from_stroke(gfxline_t*line, gfxcoord_t width, gfx_capType cap_style, gfx_joinType joint_style, gfxcoord_t miterLimit, double gridsize);
void gfxline_destroy(gfxline_t*l);

/* +----------------------------------------------------------------+ */
/* |     creation of gfxpoly objects by drawing on a "gfxcanvas"    | */
/* +----------------------------------------------------------------+ */

typedef struct _gfxcanvas
{
    void*internal;
    gfxcoord_t x,y;
    void (*moveTo)(struct _gfxcanvas*d, gfxcoord_t x, gfxcoord_t y);
    void (*lineTo)(struct _gfxcanvas*d, gfxcoord_t x, gfxcoord_t y);
    void (*splineTo)(struct _gfxcanvas*d, gfxcoord_t sx, gfxcoord_t sy, gfxcoord_t x, gfxcoord_t y);
    void (*close)(struct _gfxcanvas*d);
    void* (*result)(struct _gfxcanvas*d);
} gfxcanvas_t;

gfxcanvas_t* gfxcanvas_new(double gridsize);

/* +----------------------------------------------------------------+ */
/* |           conversion from gfxpoly to gfxline lists             | */
/* +----------------------------------------------------------------+ */

gfxline_t* gfxline_from_gfxpoly(gfxpoly_t*poly);
gfxline_t* gfxline_from_gfxpoly_with_direction(gfxpoly_t*poly);

/* +----------------------------------------------------------------+ */
/* |                   convenience functions                        | */
/* +----------------------------------------------------------------+ */

gfxline_t* gfxpoly_circular_to_evenodd(gfxline_t*line, double gridsize);
gfxpoly_t* gfxpoly_createbox(double x1, double y1,double x2, double y2, double gridsize);

/* +----------------------------------------------------------------+ */
/* |           Low level scanline processing interface              | */
/* +----------------------------------------------------------------+ */

typedef struct _windstate {
    char is_filled;
    int wind_nr;
} windstate_t;

typedef struct _windcontext {
    int num_polygons;
} windcontext_t;

extern windcontext_t onepolygon;
extern windcontext_t twopolygons;

typedef struct _windrule
{
    windstate_t (*start)(windcontext_t* context);
    windstate_t (*add)(windcontext_t*context, windstate_t left, edgestyle_t*edge, segment_dir_t dir, int polygon_nr);
    edgestyle_t* (*diff)(windstate_t*left, windstate_t*right);
} windrule_t;

extern windrule_t windrule_evenodd;
extern windrule_t windrule_circular;
extern windrule_t windrule_intersect;
extern windrule_t windrule_union;

gfxpoly_t* gfxpoly_process(gfxpoly_t*poly1, gfxpoly_t*poly2, windrule_t*windrule, windcontext_t*context, moments_t*moments);

#endif
