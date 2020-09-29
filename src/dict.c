/* dict.c

Hashtable implementation

Copyright (c) 2010-2011 Matthias Kramm <kramm@quiss.org>
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "dict.h"

// ------------------------------- crc32 -------------------------------
static unsigned int crc32[256];
static char crc32_initialized=0;
static void crc32_init(void)
{
    int t;
    if (crc32_initialized)
        return;
    crc32_initialized = 1;
    for(t=0; t<256; t++) {
        unsigned int c = t;
        int s;
        for (s = 0; s < 8; s++) {
          c = (0xedb88320L*(c&1)) ^ (c >> 1);
        }
        crc32[t] = c;
    }
}
// ------------------------------- hash function -----------------------
unsigned int crc32_add_byte(unsigned int checksum, unsigned char b)
{
    crc32_init();
    return checksum>>8 ^ crc32[(b^checksum)&0xff];
}
unsigned int crc32_add_string(unsigned int checksum, const char*s)
{
    crc32_init();
    if (!s)
        return checksum;
    while (*s) {
        checksum = checksum>>8 ^ crc32[(*s^checksum)&0xff];
        s++;
    }
    return checksum;
}
unsigned int crc32_add_bytes(unsigned int checksum, const void*_s, int len)
{
    unsigned char*s = (unsigned char*)_s;
    crc32_init();
    if (!s || !len)
        return checksum;
    do {
        checksum = checksum>>8 ^ crc32[(*s^checksum)&0xff];
        s++;
    } while (--len);
    return checksum;
}
unsigned int hash_block(const unsigned char*data, int len)
{
    int t;
    unsigned int checksum = 0;
    crc32_init();
    for(t=0;t<len;t++) {
        checksum = checksum>>8 ^ crc32[(data[t]^checksum)&0xff];
    }
    return checksum;
}

// ------------------------------- type_t -------------------------------

bool ptr_equals(const void*o1, const void*o2)
{
    return o1==o2;
}
unsigned int ptr_hash(const void*o)
{
    return hash_block((unsigned char*)&o, sizeof(o));
}
void* ptr_dup(const void*o)
{
    return (void*)o;
}
void ptr_free(void*o)
{
    return;
}

bool int_equals(const void*o1, const void*o2)
{
    return o1==o2;
}
unsigned int int_hash(const void*o)
{
    return hash_block((const unsigned char*)&o, sizeof(o));
}
void* int_dup(const void*o)
{
    return (void*)o;
}
void int_free(void*o)
{
    return;
}

bool charptr_equals(const void*o1, const void*o2)
{
    if (!o1 || !o2)
        return o1==o2;
    return !strcmp(o1,o2);
}
unsigned int charptr_hash(const void*o)
{
    if (!o)
        return 0;
    int l = strlen(o);
    return hash_block((unsigned char*)o, l);
}
void* charptr_dup(const void*o)
{
    if (!o)
        return 0;
    return strdup(o);
}
void charptr_free(void*o)
{
    if (o) {
        free(o);
    }
}

type_t int_type = {
    equals: int_equals,
    hash: int_hash,
    dup: int_dup,
    free: int_free,
};

type_t ptr_type = {
    equals: ptr_equals,
    hash: ptr_hash,
    dup: ptr_dup,
    free: ptr_free,
};

type_t charptr_type = {
    equals: charptr_equals,
    hash: charptr_hash,
    dup: charptr_dup,
    free: charptr_free,
};

// ------------------------------- dictionary_t -------------------------------

#define INITIAL_SIZE 1

dict_t*dict_new(type_t*t)
{
    dict_t*d = malloc(sizeof(dict_t));
    dict_init(d, INITIAL_SIZE);
    d->key_type = t;
    return d;
}
void dict_init(dict_t*h, int size)
{
    memset(h, 0, sizeof(dict_t));
    h->hashsize = size;
    h->slots = h->hashsize?(dictentry_t**)calloc(1,sizeof(dictentry_t*)*h->hashsize):0;
    h->num = 0;
    h->key_type = &charptr_type;
}
void dict_init2(dict_t*h, type_t*t, int size)
{
    memset(h, 0, sizeof(dict_t));
    h->hashsize = size;
    h->slots = h->hashsize?(dictentry_t**)calloc(1,sizeof(dictentry_t*)*h->hashsize):0;
    h->num = 0;
    h->key_type = t;
}

dict_t*dict_clone(dict_t*o)
{
    dict_t*h = malloc(sizeof(dict_t));
    memcpy(h, o, sizeof(dict_t));
    h->slots = h->hashsize?(dictentry_t**)calloc(1,sizeof(dictentry_t*)*h->hashsize):0;
    int t;
    for(t=0;t<o->hashsize;t++) {
        dictentry_t*e = o->slots[t];
        while (e) {
            dictentry_t*n = (dictentry_t*)malloc(sizeof(dictentry_t));
            memcpy(n, e, sizeof(dictentry_t));
            n->key = h->key_type->dup(e->key);
            n->data = e->data;
            n->next = h->slots[t];
            h->slots[t] = n;
            e = e->next;
        }
    }
    return h;
}

static void dict_expand(dict_t*h, int newlen)
{
    assert(h->hashsize < newlen);
    dictentry_t**newslots = (dictentry_t**)calloc(1,sizeof(dictentry_t*)*newlen);
    int t;
    for(t=0;t<h->hashsize;t++) {
        dictentry_t*e = h->slots[t];
        while (e) {
            dictentry_t*next = e->next;
            unsigned int newhash = e->hash%newlen;
            e->next = newslots[newhash];
            newslots[newhash] = e;
            e = next;
        }
    }
    if (h->slots)
        free(h->slots);
    h->slots = newslots;
    h->hashsize = newlen;
}

dictentry_t* dict_put(dict_t*h, const void*key, void* data)
{
    unsigned int hash = h->key_type->hash(key);
    dictentry_t*e = (dictentry_t*)malloc(sizeof(dictentry_t));

    if (!h->hashsize)
        dict_expand(h, 1);

    unsigned int hash2 = hash % h->hashsize;

    e->key = h->key_type->dup(key);
    e->hash = hash; //for resizing
    e->next = h->slots[hash2];
    e->data = data;
    h->slots[hash2] = e;
    h->num++;
    return e;
}
void dict_put2(dict_t*h, const char*s, void*data)
{
    assert(h->key_type == &charptr_type);
    dict_put(h, s, data);
}
void dict_dump(dict_t*h, FILE*fi, const char*prefix)
{
    int t;
    for(t=0;t<h->hashsize;t++) {
        dictentry_t*e = h->slots[t];
        while (e) {
            if (h->key_type!=&charptr_type) {
                fprintf(fi, "%s%p=%p\n", prefix, e->key, e->data);
            } else {
                fprintf(fi, "%s%s=%p\n", prefix, (char*)e->key, e->data);
            }
            e = e->next;
        }
    }
}

int dict_count(dict_t*h)
{
    return h->num;
}

static inline dictentry_t* dict_do_lookup(dict_t*h, const void*key)
{
    if (!h->num) {
        return 0;
    }

    unsigned int ohash = h->key_type->hash(key);
    unsigned int hash = ohash % h->hashsize;

    /* check first entry for match */
    dictentry_t*e = h->slots[hash];
    if (e && h->key_type->equals(e->key, key)) {
        return e;
    } else if (e) {
        e = e->next;
    }

    /* if dict is 2/3 filled, double the size. Do
       this the first time we have to actually iterate
       through a slot to find our data */
    if (e && h->num*3 >= h->hashsize*2) {
        int newsize = h->hashsize;
        while (h->num*3 >= newsize*2) {
            newsize = newsize<15?15:(newsize+1)*2-1;
        }
        dict_expand(h, newsize);
        hash = ohash % h->hashsize;
        e = h->slots[hash];
        if (e && h->key_type->equals(e->key, key)) {
            // omit move to front
            return e;
        } else if (e) {
            e = e->next;
        }
    }

    /* check subsequent entries for a match */
    dictentry_t*last = h->slots[hash];
    while (e) {
        if (h->key_type->equals(e->key, key)) {
            /* move to front- makes a difference of about 10% in most applications */
            last->next = e->next;
            e->next = h->slots[hash];
            h->slots[hash] = e;
            return e;
        }
        last=e;
        e = e->next;
    }
    return 0;
}
void* dict_lookup(dict_t*h, const void*key)
{
    dictentry_t*e = dict_do_lookup(h, key);
    if (e)
        return e->data;
    return 0;
}
char dict_contains(dict_t*h, const void*key)
{
    dictentry_t*e = dict_do_lookup(h, key);
    return !!e;
}

char dict_del(dict_t*h, const void*key)
{
    if (!h->num)
        return 0;
    unsigned int hash = h->key_type->hash(key) % h->hashsize;
    dictentry_t*head = h->slots[hash];
    dictentry_t*e = head, *prev=0;
    while (e) {
        if (h->key_type->equals(e->key, key)) {
            dictentry_t*next = e->next;
            h->key_type->free(e->key);
            memset(e, 0, sizeof(dictentry_t));
            free(e);
            if (e == head) {
                h->slots[hash] = next;
            } else {
                assert(prev);
                prev->next = next;
            }
            h->num--;
            return 1;
        }
        prev = e;
        e = e->next;
    }
    return 0;
}

char dict_del2(dict_t*h, const void*key, void*data)
{
    if (!h->num)
        return 0;
    unsigned int hash = h->key_type->hash(key) % h->hashsize;
    dictentry_t*head = h->slots[hash];
    dictentry_t*e = head, *prev=0;
    while (e) {
        if (h->key_type->equals(e->key, key) && e->data == data) {
            dictentry_t*next = e->next;
            h->key_type->free(e->key);
            memset(e, 0, sizeof(dictentry_t));
            free(e);
            if (e == head) {
                h->slots[hash] = next;
            } else {
                assert(prev);
                prev->next = next;
            }
            h->num--;
            return 1;
        }
        prev = e;
        e = e->next;
    }
    return 0;
}

dictentry_t* dict_get_slot(dict_t*h, const void*key)
{
    if (!h->num)
        return 0;
    unsigned int ohash = h->key_type->hash(key);
    unsigned int hash = ohash % h->hashsize;
    return h->slots[hash];
}

void dict_foreach_keyvalue(dict_t*h, void (*runFunction)(void*data, const void*key, void*val), void*data)
{
    int t;
    for(t=0;t<h->hashsize;t++) {
        dictentry_t*e = h->slots[t];
        while (e) {
            dictentry_t*next = e->next;
            if (runFunction) {
                runFunction(data, e->key, e->data);
            }
            e = next;
        }
    }
}
void dict_foreach_value(dict_t*h, void (*runFunction)(void*))
{
    int t;
    for(t=0;t<h->hashsize;t++) {
        dictentry_t*e = h->slots[t];
        while (e) {
            dictentry_t*next = e->next;
            if (runFunction) {
                runFunction(e->data);
            }
            e = next;
        }
    }
}

void dict_free_all(dict_t*h, char free_keys, void (*free_data_function)(void*))
{
    int t;
    for(t=0;t<h->hashsize;t++) {
        dictentry_t*e = h->slots[t];
        while (e) {
            dictentry_t*next = e->next;
            if (free_keys) {
                h->key_type->free(e->key);
            }
            if (free_data_function) {
                free_data_function(e->data);
            }
            memset(e, 0, sizeof(dictentry_t));
            free(e);
            e = next;
        }
        h->slots[t]=0;
    }
    free(h->slots);
    memset(h, 0, sizeof(dict_t));
}

void dict_clear_shallow(dict_t*h)
{
    dict_free_all(h, 0, 0);
}

void dict_clear(dict_t*h)
{
    dict_free_all(h, 1, 0);
}

void dict_destroy_shallow(dict_t*dict)
{
    dict_clear_shallow(dict);
    free(dict);
}

void dict_destroy(dict_t*dict)
{
    if (!dict)
        return;
    dict_clear(dict);
    free(dict);
}

void dict_destroy_with_data(dict_t*dict)
{
    if (!dict)
        return;
    dict_free_all(dict, 1, free);
    free(dict);
}

