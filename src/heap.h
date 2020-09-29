/* heap.h

An inline heap implementation

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

#define HEAP_DEFINE(name,t,lt)                                         \
typedef struct {                                                       \
    t**elements;                                                       \
    int size;                                                          \
    int max_size;                                                      \
} name##_t;                                                            \
static void name##_put(name##_t*h, t*e)                                \
{                                                                      \
    int parent = h->size++;                                            \
    if (h->size>=h->max_size) {                                         \
        h->max_size = h->max_size<15?15:(h->max_size+1)*2-1;           \
        h->elements = (t**)realloc(h->elements,                        \
                                      h->max_size*sizeof(t*));         \
    }                                                                  \
    int node;                                                          \
    do {                                                               \
        node = parent;                                                 \
        if (!node) break;                                               \
        parent = (node-1)/2;                                           \
        h->elements[node] = h->elements[parent];                       \
    } while (lt(e, h->elements[parent]));                               \
    h->elements[node] = e;                                             \
}                                                                      \
static t* name##_get(name##_t*h)                                       \
{                                                                      \
    if (!h->size) return 0;                                             \
    t*r = h->elements[0];                                              \
    int node,child = 0;                                                \
    t*node_p = h->elements[--h->size];                                 \
    h->elements[0] = node_p; /* for the size==1 case */                \
    do {                                                               \
        node = child;                                                  \
        child = node<<1|1;                                             \
        if (child >= h->size) {                                         \
            break;                                                     \
        }                                                              \
        if (child+1 < h->size && lt(h->elements[child+1],               \
                                   h->elements[child]))                \
            child++;                                                   \
        h->elements[node] = h->elements[child];                        \
    } while (lt(h->elements[child],node_p));                            \
    h->elements[node] = node_p;                                        \
    return r;                                                          \
}                                                                      \
static void name##_init(name##_t*h)                                    \
{                                                                      \
    memset(h, 0, sizeof(*h));                                          \
    h->max_size = 15;                                                  \
    h->elements = malloc(h->max_size*sizeof(t*));                      \
}                                                                      \
static void name##_destroy(name##_t*h)                                 \
{                                                                      \
    free((h)->elements);                                               \
}
