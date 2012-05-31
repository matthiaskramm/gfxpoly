/* wind.h

   Fillstyle tyles (wind rules)

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

#ifndef __wind_h__
#define __wind_h__

#include "gfxpoly.h" 

#define DIR_INVERT(d) ((d)^(DIR_UP^DIR_DOWN))

extern edgestyle_t edgestyle_default;

extern windcontext_t onepolygon;
extern windcontext_t twopolygons;

extern windrule_t windrule_evenodd;
extern windrule_t windrule_circular;
extern windrule_t windrule_intersect;
extern windrule_t windrule_union;

#endif
