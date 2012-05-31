/* stroke.h

   Stroke to polygon conversion

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

#ifndef __stroke_h__
#define __stroke_h__
#include "poly.h"
gfxpoly_t* gfxpoly_from_stroke(gfxline_t*line, gfxcoord_t width, gfx_capType cap_style, gfx_joinType joint_style, gfxcoord_t miterLimit, double gridsize);
#endif
