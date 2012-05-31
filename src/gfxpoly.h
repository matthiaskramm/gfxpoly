/* gfxpoly.h

   Boolean polygon operations library

   Copyright (c) 2009-2012 Matthias Kramm <kramm@quiss.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef __gfxpoly_h__
#define __gfxpoly_h__

#include <stdint.h>

/* A "grid" value is the granularity at which polygon intersection operates.
   It usually makes sense to set this to the smallest value that can actually be represented
   in the output device (like 0.05 = 1 twip for SWF).  */
#define DEFAULT_GRID (0.05)

/* A coordinate on the grid. This is represented as integers, and the real coordinate
   can be derived by multiplying each component with the grid size */
typedef struct _point {
    int32_t x;
    int32_t y;
} point_t;

/* Every segment has an original direction, which is the direction
   the segment had in the input data.
   as our scanline moves from minimum y to maximum y, "DOWN" means
   the the (original) segment's y2 is larger than its y1 */
typedef enum {DIR_UP, DIR_DOWN, DIR_UNKNOWN} segment_dir_t;

typedef struct _edgestyle {
    void*internal;
} edgestyle_t;

typedef struct _gfxpolystroke {
    segment_dir_t dir;
    edgestyle_t*fs;
    int points_size;
    int num_points;
    point_t*points;
    struct _gfxpolystroke*next;
} gfxpolystroke_t;

typedef struct _gfxpoly {
    double gridsize;
    gfxpolystroke_t*strokes;
} gfxpoly_t;

void gfxpoly_destroy(gfxpoly_t*poly);

/* operators */
gfxpoly_t* gfxpoly_intersect(gfxpoly_t*p1, gfxpoly_t*p2);
gfxpoly_t* gfxpoly_union(gfxpoly_t*p1, gfxpoly_t*p2);

/* area and moments functions */
typedef struct _moments {
    double area;
    double m[3][3];
} moments_t;

double gfxpoly_area(gfxpoly_t*p);
double gfxpoly_intersection_area(gfxpoly_t*p1, gfxpoly_t*p2);

/* low-level scanline processing interface */
typedef struct _windstate {
    char is_filled;
    int wind_nr;
} windstate_t;

typedef struct _windcontext {
    int num_polygons;
} windcontext_t;

typedef struct _windrule
{
    windstate_t (*start)(windcontext_t* context);
    windstate_t (*add)(windcontext_t*context, windstate_t left, edgestyle_t*edge, segment_dir_t dir, int polygon_nr);
    edgestyle_t* (*diff)(windstate_t*left, windstate_t*right);
} windrule_t;

gfxpoly_t* gfxpoly_process(gfxpoly_t*poly1, gfxpoly_t*poly2, windrule_t*windrule, windcontext_t*context, moments_t*moments);

/* conversion from shapes with curves and floating point coordinates */
typedef double gfxcoord_t;
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

gfxpoly_t* gfxpoly_from_fill(gfxline_t*line, double gridsize);
gfxpoly_t* gfxpoly_from_stroke(gfxline_t*line, gfxcoord_t width, gfx_capType cap_style, gfx_joinType joint_style, gfxcoord_t miterLimit, double gridsize);

/* creation of gfxpoly objects by drawing on a "gfxcanvas" */

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

/* conversion from gfxpoly to gfxline lists */
gfxline_t* gfxline_from_gfxpoly(gfxpoly_t*poly);
gfxline_t* gfxline_from_gfxpoly_with_direction(gfxpoly_t*poly);

/* convenience functions */
gfxline_t* gfxpoly_circular_to_evenodd(gfxline_t*line, double gridsize);
gfxpoly_t* gfxpoly_createbox(double x1, double y1,double x2, double y2, double gridsize);

#endif
