/* xrow.h

   Representation of point positions in scanlines

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

#ifndef __xrow_h__
#define __xrow_h__

#include <stdint.h>

#include "poly.h"

typedef struct _xrow {
    int32_t*x;
    int num;
    int size;
    int32_t lastx;
} xrow_t;

xrow_t* xrow_new();

void xrow_add(xrow_t*xrow, int32_t x);
void xrow_sort(xrow_t*xrow);
int xrow_find(xrow_t*r, int32_t x);
char xrow_contains(xrow_t*xrow, int32_t x);
void xrow_dump(xrow_t*xrow, double gridsize);
void xrow_reset(xrow_t*xrow);
void xrow_destroy(xrow_t*xrow);

#endif
