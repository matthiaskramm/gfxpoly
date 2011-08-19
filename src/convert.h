/* convert.h

   Polygon conversion functions

   Copyright (c) 2009 Matthias Kramm <kramm@quiss.org>

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

#ifndef __poly_convert_h__
#define __poly_convert_h__

#include "poly.h"

typedef enum {gfx_moveTo, gfx_lineTo, gfx_splineTo} gfx_linetype;
typedef enum {gfx_joinMiter, gfx_joinRound, gfx_joinBevel} gfx_joinType;
typedef enum {gfx_capButt, gfx_capRound, gfx_capSquare} gfx_capType;

typedef double gfxcoord_t;

typedef struct _gfxline
{
    gfx_linetype type;
    gfxcoord_t x,y;
    gfxcoord_t sx,sy;
    struct _gfxline*next; /*NULL=end*/
} gfxline_t;

typedef struct _gfxdrawer
{
    void*internal;
    gfxcoord_t x,y;
    void (*moveTo)(struct _gfxdrawer*d, gfxcoord_t x, gfxcoord_t y);
    void (*lineTo)(struct _gfxdrawer*d, gfxcoord_t x, gfxcoord_t y);
    void (*splineTo)(struct _gfxdrawer*d, gfxcoord_t sx, gfxcoord_t sy, gfxcoord_t x, gfxcoord_t y);
    void (*close)(struct _gfxdrawer*d);
    void* (*result)(struct _gfxdrawer*d);
} gfxdrawer_t;

typedef struct _polywriter
{
    void(*moveto)(struct _polywriter*, int32_t x, int32_t y);
    void(*lineto)(struct _polywriter*, int32_t x, int32_t y);
    void(*setgridsize)(struct _polywriter*, double g);
    void*(*finish)(struct _polywriter*);
    void*internal;
} polywriter_t;

void gfxdrawer_target_poly(gfxdrawer_t*d, double gridsize);

void gfxpolywriter_init(polywriter_t*w);
gfxpoly_t* gfxpoly_from_fill(gfxline_t*line, double gridsize);
gfxpoly_t* gfxpoly_from_file(const char*filename, double gridsize);
void gfxpoly_destroy(gfxpoly_t*poly);

gfxline_t*gfxline_from_gfxpoly(gfxpoly_t*poly);
gfxline_t*gfxline_from_gfxpoly_with_direction(gfxpoly_t*poly); // preserves up/down

gfxline_t* gfxpoly_circular_to_evenodd(gfxline_t*line, double gridsize);
gfxpoly_t* gfxpoly_createbox(double x1, double y1,double x2, double y2, double gridsize);

#endif //__poly_convert_h__
