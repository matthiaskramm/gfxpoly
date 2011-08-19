/* render.h

   Polygon rendering (for debugging)

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
