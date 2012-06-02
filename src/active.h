/* active.h

Active list

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
