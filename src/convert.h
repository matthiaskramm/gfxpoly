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

typedef struct _polywriter
{
    void(*moveto)(struct _polywriter*, int32_t x, int32_t y);
    void(*lineto)(struct _polywriter*, int32_t x, int32_t y);
    void(*setgridsize)(struct _polywriter*, double g);
    void*(*finish)(struct _polywriter*);
    void*internal;
} polywriter_t;

void gfxcanvas_target_poly(gfxcanvas_t*d, double gridsize);

void gfxpolywriter_init(polywriter_t*w);
gfxpoly_t* gfxpoly_from_fill(gfxline_t*line, double gridsize);
gfxpoly_t* gfxpoly_from_file(const char*filename, double gridsize);

#endif //__poly_convert_h__
