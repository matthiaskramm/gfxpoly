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

#ifndef __poly_h__
#define __poly_h__

typedef enum {gfx_moveTo, gfx_lineTo, gfx_splineTo} gfx_linetype;
typedef enum {gfx_joinMiter, gfx_joinRound, gfx_joinBevel} gfx_joinType;
typedef enum {gfx_capButt, gfx_capRound, gfx_capSquare} gfx_capType;

typedef struct _gfxline
{
    gfx_linetype type;
    double x,y;
    double sx,sy;
    struct _gfxline*prev;
    struct _gfxline*next;
} gfxline_t;

gfxpoly_t* gfxpoly_from_fill(gfxline_t*line, double gridsize);
gfxpoly_t* gfxpoly_from_stroke(gfxline_t*line, double  width, gfx_capType cap_style, gfx_joinType joint_style, double miterLimit, double gridsize);

gfxpoly_t* gfxpoly_intersect(gfxpoly_t*p1, gfxpoly_t*p2);
gfxpoly_t* gfxpoly_union(gfxpoly_t*p1, gfxpoly_t*p2);
double gfxpoly_area(gfxpoly_t*p);
double gfxpoly_intersection_area(gfxpoly_t*p1, gfxpoly_t*p2);

gfxpoly_t* gfxpoly_process(gfxpoly_t*poly1, gfxpoly_t*poly2, windrule_t*windrule, windcontext_t*context, moments_t*moments);

#endif
