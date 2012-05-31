/* wind.c

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

#include "poly.h"

edgestyle_t edgestyle_default;

windstate_t windstate_nonfilled = {
    is_filled: 0,
    wind_nr: 0,
};

windcontext_t onepolygon = {1};
windcontext_t twopolygons = {2};

// -------------------- even/odd ----------------------

windstate_t evenodd_start(windcontext_t*context)
{
    return windstate_nonfilled;
}
windstate_t evenodd_add(windcontext_t*context, windstate_t left, edgestyle_t*edge, segment_dir_t dir, int master)
{
    assert(edge);
    left.is_filled ^= 1;
    return left;
}
edgestyle_t* evenodd_diff(windstate_t*left, windstate_t*right)
{
    if(left->is_filled==right->is_filled)
        return 0;
    else
        return &edgestyle_default;
}

windrule_t windrule_evenodd = {
    start: evenodd_start,
    add: evenodd_add,
    diff: evenodd_diff,
};

// -------------------- circular ----------------------

windstate_t circular_start(windcontext_t*context)
{
    return windstate_nonfilled;
}

windstate_t circular_add(windcontext_t*context, windstate_t left, edgestyle_t*edge, segment_dir_t dir, int master)
{
    assert(edge);
    /* which one is + and which one - doesn't actually make any difference */
    if(dir == DIR_DOWN)
        left.wind_nr++;
    else
        left.wind_nr--;

    left.is_filled = left.wind_nr != 0;
    return left;
}

edgestyle_t* circular_diff(windstate_t*left, windstate_t*right)
{
    if(left->is_filled==right->is_filled)
        return 0;
    else
        return &edgestyle_default;
}

windrule_t windrule_circular = {
    start: circular_start,
    add: circular_add,
    diff: circular_diff,
};

// -------------------- intersect ----------------------

windstate_t intersect_start(windcontext_t*context)
{
    return windstate_nonfilled;
}

windstate_t intersect_add(windcontext_t*context, windstate_t left, edgestyle_t*edge, segment_dir_t dir, int master)
{
    assert(master < context->num_polygons);

    left.wind_nr ^= 1<<master;
    left.is_filled = (left.wind_nr == (1<<context->num_polygons)-1);
    return left;
}

edgestyle_t* intersect_diff(windstate_t*left, windstate_t*right)
{
    if(left->is_filled==right->is_filled)
        return 0;
    else
        return &edgestyle_default;
}

windrule_t windrule_intersect = {
    start: intersect_start,
    add: intersect_add,
    diff: intersect_diff,
};

// -------------------- union ----------------------

windstate_t union_start(windcontext_t*context)
{
    return windstate_nonfilled;
}

windstate_t union_add(windcontext_t*context, windstate_t left, edgestyle_t*edge, segment_dir_t dir, int master)
{
    assert(master<sizeof(left.wind_nr)*8); //up to 32/64 polygons max
    left.wind_nr ^= 1<<master;
    left.is_filled = (left.wind_nr!=0);
    return left;
}

edgestyle_t* union_diff(windstate_t*left, windstate_t*right)
{
    if(left->is_filled==right->is_filled)
        return 0;
    else
        return &edgestyle_default;
}

windrule_t windrule_union = {
    start: union_start,
    add: union_add,
    diff: union_diff,
};


/* 
 } else if (rule == WIND_NONZERO) {
     fill = wind!=0;
 } else if (rule == WIND_ODDEVEN) {
     fill = wind&1;
 } else { // rule == WIND_POSITIVE
     fill = wind>=1;
 }
 */
