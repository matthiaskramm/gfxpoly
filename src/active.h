/* active.h

   Active list

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

#ifndef __active_h__
#define __active_h__

#include "poly.h"

typedef struct _actlist
{
    segment_t*list;
    int size;
#ifdef SPLAY
    segment_t*root;
#endif
} actlist_t;

#define actlist_left(a,s) ((s)->left)
#define actlist_right(a,s) ((s)?(s)->right:(a)->list)

actlist_t* actlist_new();
void actlist_destroy(actlist_t*a);
int actlist_size(actlist_t*a);
void actlist_verify(actlist_t*a, int32_t y);
void actlist_dump(actlist_t*a, int32_t y, double gridsize);
segment_t* actlist_find(actlist_t*a, point_t p1, point_t p2);  // finds segment immediately to the left of p1 (breaking ties w/ p2)
void actlist_insert(actlist_t*a, point_t p1, point_t p2, segment_t*s);
void actlist_delete(actlist_t*a, segment_t*s);
void actlist_swap(actlist_t*a, segment_t*s1, segment_t*s2);
segment_t* actlist_leftmost(actlist_t*a);
segment_t* actlist_rightmost(actlist_t*a);

#endif
