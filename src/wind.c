/* wind.c

Fillstyle tyles (wind rules)

Copyright (c) 2009 Matthias Kramm <kramm@quiss.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. */

#include "poly.h"

edgestyle_t edgestyle_default;

windstate_t windstate_nonfilled = {
    is_filled: 0,
    wind_nr: 0,
};

windcontext_t onepolygon = {NULL,1};
windcontext_t twopolygons = {NULL,2};

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
edgestyle_t* evenodd_diff(windcontext_t*context, windstate_t*left, windstate_t*right)
{
    if (left->is_filled==right->is_filled)
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
    if (dir == DIR_DOWN)
        left.wind_nr++;
    else
        left.wind_nr--;

    left.is_filled = left.wind_nr != 0;
    return left;
}

edgestyle_t* circular_diff(windcontext_t*context, windstate_t*left, windstate_t*right)
{
    if (left->is_filled==right->is_filled)
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

edgestyle_t* intersect_diff(windcontext_t*context, windstate_t*left, windstate_t*right)
{
    if (left->is_filled==right->is_filled)
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

edgestyle_t* union_diff(windcontext_t*context, windstate_t*left, windstate_t*right)
{
    if (left->is_filled==right->is_filled)
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
