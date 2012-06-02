/* xrow.h

Representation of point positions in scanlines

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
