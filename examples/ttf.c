/* ttf.c

Parser and writer for truetype font files.

Copyright (c) 2010,2011,2012 Matthias Kramm <kramm@quiss.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <assert.h>
#include "ttf.h"

#define TTCFTAG 0x74746366

#define OPENTYPE 0x4f54544f
#define TRUETYPE_MACOS 0x74727565
#define VERSION_1_0 0x00010000

#define TAG_OS2 0x4f532f32
#define TAG_CMAP 0x636d6170
#define TAG_GLYF 0x676c7966 //required for non opentype
#define TAG_HEAD 0x68656164 //required
#define TAG_HHEA 0x68686561 //required
#define TAG_HMTX 0x686d7478 //required
#define TAG_VHEA 0x86686561
#define TAG_VMTX 0x866d7478
#define TAG_KERN 0x6b65726e
#define TAG_LOCA 0x6c6f6361 //required for non opentype
#define TAG_MAXP 0x6d617870 //required
#define TAG_NAME 0x6e616d65
#define TAG_POST 0x706f7374
#define TAG_CFF  0x43464620 //required for opentype
#define TAG_CVT  0x63767420
#define TAG_FPGM 0x6670676d
#define TAG_GASP 0x67617370
#define TAG_PREP 0x70726570

#define SWAP16(s) ((((s)>>8)&0x00ff)|(((s)<<8)&0xff00))
#define SWAP32(s) (SWAP16(((s)>>16)&0x0000ffff)|((SWAP16(s)<<16)&0xffff0000))

static void msg(char*format, ...)
{
    char buf[1024];
    int l;
    va_list arglist;
    va_start(arglist, format);
    vsnprintf(buf, sizeof(buf)-1, format, arglist);
    va_end(arglist);
    l = strlen(buf);
    while (l && buf[l-1]=='\n') {
        buf[l-1] = 0;
        l--;
    }
    fprintf(stderr, "%s\n", buf);
    fflush(stdout);
}

static uint32_t checksum_block(uint8_t*_data, int len)
{
    uint32_t sum = 0;
    uint8_t*data = (uint8_t*)_data;

    int pos;
    int len_minus_4 = len-4;
    for(pos=0;pos<=len_minus_4;pos+=4) {
        sum += data[pos]<<24|data[pos+1]<<16|data[pos+2]<<8|data[pos+3];
    }
    int left = len-pos;
    if (left == 1) sum+= data[pos+0]<<24;
    if (left == 2) sum+= data[pos+0]<<24|data[pos+1]<<16;
    if (left == 3) sum+= data[pos+0]<<24|data[pos+1]<<16|data[pos+2]<<8;
    return sum;
}

typedef struct _memreader {
    uint8_t*mem;
    int pos;
    int size;
} memreader_t;

static uint8_t readuint8_t(memreader_t*r)
{
    return r->mem[r->pos++];
}
static inline uint16_t readuint16_t(memreader_t*r)
{
    if (r->pos+2>r->size) return 0;
    uint16_t val = r->mem[r->pos]<<8|
              r->mem[r->pos+1];
    r->pos += 2;
    return val;
}
static int16_t readint16_t(memreader_t*r)
{
    return (int16_t)readuint16_t(r);
}
static uint32_t readuint32_t(memreader_t*r)
{
    if (r->pos+4>r->size) return 0;
    uint32_t val = r->mem[r->pos]<<24|
              r->mem[r->pos+1]<<16|
              r->mem[r->pos+2]<<8|
              r->mem[r->pos+3];
    r->pos += 4;
    return val;
}
static void readBlock(memreader_t*r, void*dest, int len)
{
    int remaining = r->size-r->pos;
    if (len > remaining) {
        memcpy(dest, r->mem+r->pos, remaining);
        memset(dest+remaining, 0, len - remaining);
        r->pos += remaining;
    } else {
        memcpy(dest, r->mem+r->pos, len);
        r->pos += len;
    }
}
static void reader_reset(memreader_t*r)
{
    r->pos;
}
#define INIT_READ(r,data,length,pos) memreader_t r = {(data),(pos),(length)};

static void expand(ttf_table_t*w, int newsize)
{
    int v1 = (newsize+63)&~63;
    int v2 = w->len + w->len / 2;
    w->memsize = v1>v2?v1:v2;
    w->data = realloc(w->data, w->memsize);
}
static inline void writeuint8_t(ttf_table_t*w, unsigned char b)
{
    if (w->memsize<w->len+1)
        expand(w, w->len+1);
    w->data[w->len++] = b;
}
static inline void writeuint16_t(ttf_table_t*w, unsigned short v)
{
    if (w->memsize<w->len+2)
        expand(w, w->len+2);
    w->data[w->len++] = v>>8;
    w->data[w->len++] = v;
}
static inline void writeuint16_t_LE(ttf_table_t*w, unsigned short v)
{
    if (w->memsize<w->len+2)
        expand(w, w->len+2);
    w->data[w->len++] = v;
    w->data[w->len++] = v>>8;
}
#define writeint16_t writeuint16_t
static inline void writeuint32_t(ttf_table_t*w, unsigned long v)
{
    if (w->memsize<w->len+4)
        expand(w, w->len+4);
    w->data[w->len++] = v>>24;
    w->data[w->len++] = v>>16;
    w->data[w->len++] = v>>8;
    w->data[w->len++] = v;
}
static inline void writeuint32_t_LE(ttf_table_t*w, unsigned long v)
{
    if (w->memsize<w->len+4)
        expand(w, w->len+4);
    w->data[w->len++] = v;
    w->data[w->len++] = v>>8;
    w->data[w->len++] = v>>16;
    w->data[w->len++] = v>>24;
}
static inline void writeBlock(ttf_table_t*w, void*data, int len)
{
    if (w->memsize<w->len+len)
        expand(w, w->len+len);
    memcpy(w->data+w->len, data, len);
    w->len += len;
}

ttf_table_t*ttf_table_new(uint32_t id)
{
    ttf_table_t*t = calloc(1, sizeof(ttf_table_t));
    t->id = id;
    return t;
}

ttf_table_t*ttf_addtable(ttf_t*ttf, uint32_t id)
{
    ttf_table_t*t = ttf_table_new(id);

    ttf_table_t*before,*after=0;
    for(before=ttf->tables; before&&before->id<id; before=before->next) {
        after=before;
    }
    if (before && before->id == id) {
        msg("<error> duplicate table %08x", id);
        free(before->data);
        before->data = 0;
        before->len = 0;
        return before;
    }

    if (!after) {
        t->next = ttf->tables;
        ttf->tables = t;
    } else {
        t->prev = after;
        t->next = after->next;
        after->next = t;
    }
    if (t->next)
        t->next->prev = t;
    return t;
}
ttf_table_t*ttf_find_table(ttf_t*ttf, uint32_t id)
{
    ttf_table_t*table = ttf->tables;
    while (table) {
        if (table->id == id)
            return table;
        table = table->next;
    }
    return 0;
}
void ttf_table_delete(ttf_t*ttf, ttf_table_t*table)
{
    if (ttf && ttf->tables == table) {
        ttf->tables = table->next;
    }
    if (table->prev)
        table->prev->next = table->next;
    if (table->next)
        table->next->prev = table->prev;
    free(table->data);
    free(table);
}
uint32_t ttf_table_checksum(ttf_table_t*t)
{
    uint32_t checksum = checksum_block(t->data, t->len);
    if (t->id==TAG_HEAD && t->len>=12) {
        /* the checksum for the HEAD table is calculated by masking out
           the checksumadjust field */
        uint32_t adjust = t->data[8]<<24|t->data[9]<<16|t->data[10]<<8|t->data[11];
        checksum -= adjust;
    }
    return checksum;
}
static uint8_t printable(uint8_t a)
{
    if (a<32 || a==127) return '.';
    else return a;
}
static void hexdump(uint8_t*data, int len, const char*prefix)
{
    int t;
    char ascii[32];
    printf("%s    -=> ",prefix);
    for(t=0;t<len;t++) {
        printf("%02x ", data[t]);
        ascii[t&15] = printable(data[t]);
        if ((t && ((t&15)==15)) || (t==len-1))
        {
            int s,p=((t)&15)+1;
            ascii[p] = 0;
            for(s=p-1;s<16;s++) {
                printf("   ");
            }
            if (t==len-1)
                printf(" %s\n", ascii);
            else
                printf(" %s\n%s    -=> ",ascii,prefix);
        }
    }
}
static void ttf_table_dump(ttf_table_t*t, const char*prefix)
{
    if (!t) return;
    hexdump(t->data, t->len, prefix);
}

static table_head_t*head_new(ttf_t*ttf)
{
    table_head_t*head = calloc(1, sizeof(table_head_t));
    head->units_per_em = 1024;
    int t;
    if (ttf->num_glyphs) {
        head->xmin = ttf->glyphs[0].xmin;
        head->ymin = ttf->glyphs[0].ymin;
        head->xmax = ttf->glyphs[0].xmax;
        head->ymax = ttf->glyphs[0].ymax;
        /* notice: the Microsoft font renderer expects these values to be adjusted (shifted)
           by the left side bearing if bit 1 in ttf->flags is not set */
        for(t=1;t<ttf->num_glyphs;t++) {
            if (ttf->glyphs[t].xmin < head->xmin) head->xmin = ttf->glyphs[t].xmin;
            if (ttf->glyphs[t].ymin < head->ymin) head->ymin = ttf->glyphs[t].ymin;
            if (ttf->glyphs[t].xmax > head->xmax) head->xmax = ttf->glyphs[t].xmax;
            if (ttf->glyphs[t].ymax > head->ymax) head->ymax = ttf->glyphs[t].ymax;
        }
    }
    head->macStyle = 0;
    head->lowest_readable_size = 8; // not sure what font renderers actually do with this
    head->dir_hint = 0;
    return head;
}
static int head_parse(ttf_t*ttf, memreader_t*r)
{
    ttf->head = calloc(1, sizeof(table_head_t));
    uint32_t version = readuint32_t(r);
    if (version!=VERSION_1_0)
        msg("<warning> Font HEAD has unknown version %08x", version);
    uint32_t revision = readuint32_t(r);
    uint32_t checksum2 = readuint32_t(r);
    uint32_t magic = readuint32_t(r);
    if (magic!=0x5f0f3cf5)
        msg("<warning> Font HEAD has unknown magic number %08x", magic);
    ttf->head->flags = readuint16_t(r);
    ttf->head->units_per_em = readuint16_t(r);
    readuint32_t(r);readuint32_t(r); //created
    readuint32_t(r);readuint32_t(r); //modified
    ttf->head->xmin = readuint16_t(r);
    ttf->head->ymin = readuint16_t(r);
    ttf->head->xmax = readuint16_t(r);
    ttf->head->ymax = readuint16_t(r);
    ttf->head->macStyle = readuint16_t(r);
    ttf->head->lowest_readable_size = readuint16_t(r); //in pixels
    ttf->head->dir_hint = readint16_t(r);
    int loc_index = readint16_t(r); //used in 'loca' table
    if (loc_index>1)
        msg("<warning> loca index format %d unknown", loc_index);
    uint16_t glyph_data_format = readint16_t(r);
    if (glyph_data_format!=0)
        msg("<warning> Font glyph data format unknown: %04x", glyph_data_format);
    if (r->pos < r->size) {
        msg("<warning> Leftover bytes (%d) in HEAD tag", r->size - r->pos);
    }
    return loc_index;
}
static void head_write(ttf_t*ttf, ttf_table_t*w, int loca_size)
{
    writeuint32_t(w, 0x10000);
    writeuint32_t(w, 0x10000);
    writeuint32_t(w, 0); //checksum
    writeuint32_t(w, 0x5f0f3cf5); //magic
    writeuint16_t(w, ttf->head->flags);
    writeuint16_t(w, ttf->head->units_per_em);
    writeuint32_t(w, 0);writeuint32_t(w, 0); //created
    writeuint32_t(w, 0);writeuint32_t(w, 0); //modified
    writeuint16_t(w, ttf->head->xmin);
    writeuint16_t(w, ttf->head->ymin);
    writeuint16_t(w, ttf->head->xmax);
    writeuint16_t(w, ttf->head->ymax);
    writeuint16_t(w, ttf->head->macStyle);
    writeuint16_t(w, ttf->head->lowest_readable_size);
    writeint16_t(w, ttf->head->dir_hint);
    writeint16_t(w, loca_size); //loca index size (32 bit)
    writeint16_t(w, 0); //glyph data format
}
static void head_dump(ttf_t*ttf)
{
    printf("head->flags: %d\n", ttf->head->flags);
    printf("head->units_per_em: %d\n", ttf->head->units_per_em);
    printf("head->xmin: %d\n", ttf->head->xmin);
    printf("head->ymin: %d\n", ttf->head->ymin);
    printf("head->xmax: %d\n", ttf->head->xmax);
    printf("head->ymax: %d\n", ttf->head->ymax);
    printf("head->macStyle: %d\n", ttf->head->macStyle);
    printf("head->lowest_readable_size: %d\n", ttf->head->lowest_readable_size);
    printf("head->dir_hint: %d\n", ttf->head->dir_hint);
}
static void head_delete(ttf_t*ttf)
{
    if (ttf->head) {
        free(ttf->head);
        ttf->head=0;
    }
}

static table_os2_t*os2_new(ttf_t*ttf)
{
    table_os2_t*os2 = calloc(1, sizeof(table_os2_t));
    if (ttf->num_glyphs) {
        int average_width=0;
        int t;
        for(t=0;t<ttf->num_glyphs;t++) {
            average_width += (ttf->glyphs[t].advance + ttf->glyphs[t].bearing);
        }
        os2->xAvgCharWidth = average_width / ttf->num_glyphs;
    }

    /* that's what everybody seems to fill in */
    os2->usWeightClass = 400;
    os2->usWidthClass = 5;

    if (ttf->head) {
        int advance = (ttf->head->xmax - ttf->head->xmin)/2;
        int height = (ttf->head->xmax - ttf->head->xmin);
        int ymid = height/2;
        /* I do believe a sane font rendering engine will actually use
           the font advance here- the subscript/superscript position will
           not be the same for each glyph */
        os2->ySuperscriptXSize = os2->ySubscriptXSize = (ttf->head->xmax - ttf->head->xmin)/2;
        os2->ySuperscriptYSize = os2->ySubscriptYSize = (ttf->head->ymax - ttf->head->ymin)/2;
        os2->ySubscriptXOffset = advance;
        os2->ySubscriptYOffset = 0;
        os2->ySuperscriptXOffset = advance;
        os2->ySuperscriptYOffset = (ttf->head->ymax - ttf->head->ymin)/2;
        os2->yStrikeoutSize = ttf->head->units_per_em / 10;
        os2->yStrikeoutPosition = ymid;
        os2->usWinAscent = ttf->ascent;
        os2->usWinDescent = ttf->descent>0?0:-ttf->descent;
        if (os2->usWinAscent+os2->usWinDescent < 32) {
            /* Windows requires fonts to have a height of at least 32, otherwise it'll
               refuse to display it. */
            os2->usWinDescent = 32;
        }

        os2->sxHeight = ymid;
        os2->sCapHeight = height*2/3;
    }
    os2->panose_Weight = 4;

    /* strictly speaking we'd have to set 92/64 bits in these tables, depending on
       what parts of the unicode table is filled. (e.g. bit 90 = tibetan). */
    os2->ulCharRange[0] = 1;
    os2->ulCharRange[1] = 0;
    os2->ulCharRange[2] = 0;
    os2->ulCharRange[3] = 0;
    os2->ulCodePageRange1 = 1;
    os2->ulCodePageRange2 = 0;

    if (ttf->unicode_size) {
        int min,max;
        for(min=0;min<ttf->unicode_size;min++)
            if (ttf->unicode[min]) break;
        for(max=ttf->unicode_size-1;max>=0;max--)
            if (ttf->unicode[max]) break;
        if (min<=max) {
            os2->fsFirstCharIndex = min;
            os2->fsLastCharIndex = max;
        }
    }
    os2->sTypoAscender = ttf->ascent;
    os2->sTypoDescender = ttf->descent;
    os2->sTypoLineGap = ttf->lineGap;

    os2->usDefaultChar = 0;
    os2->usBreakChar = (ttf->unicode_size>0x20 && ttf->unicode[0x20])?0x20:0;
    os2->usMaxContext = 0; // we don't use ligatures yet
    return os2;
}
static table_os2_t*os2_parse(memreader_t*r)
{
    table_os2_t*os2 = calloc(1, sizeof(table_os2_t));
    uint16_t version = readuint16_t(r);
    /* 0 = TrueType 1.5
       1 = TrueType 1.66
       2 = OpenType 1.2
       3 = OpenType 1.4 */
    if (version!=0 && version!=1 && version!=2 && version!=3)
        msg("<warning> Unknown OS2 version: %04x", version);
    os2->xAvgCharWidth = readint16_t(r);
    os2->usWeightClass = readuint16_t(r);
    os2->usWidthClass = readuint16_t(r);
    readuint16_t(r); //fstype
    os2->ySubscriptXSize = readuint16_t(r);
    os2->ySubscriptYSize = readuint16_t(r);
    os2->ySubscriptXOffset = readuint16_t(r);
    os2->ySubscriptYOffset = readuint16_t(r);
    os2->ySuperscriptXSize = readuint16_t(r);
    os2->ySuperscriptYSize = readuint16_t(r);
    os2->ySuperscriptXOffset = readuint16_t(r);
    os2->ySuperscriptYOffset = readuint16_t(r);
    os2->yStrikeoutSize = readuint16_t(r);
    os2->yStrikeoutPosition = readuint16_t(r);
    os2->sFamilyClass = readuint16_t(r);
    os2->panose_FamilyType = readuint8_t(r);
    os2->panose_SerifStyle = readuint8_t(r);
    os2->panose_Weight = readuint8_t(r);
    os2->panose_Proportion = readuint8_t(r);
    os2->panose_Contrast = readuint8_t(r);
    os2->panose_StrokeVariation = readuint8_t(r);
    os2->panose_ArmStyle = readuint8_t(r);
    os2->panose_Letterform = readuint8_t(r);
    os2->panose_Midline = readuint8_t(r);
    os2->panose_XHeight = readuint8_t(r);
    os2->ulCharRange[0] = readuint32_t(r);
    os2->ulCharRange[1] = readuint32_t(r);
    os2->ulCharRange[2] = readuint32_t(r);
    os2->ulCharRange[3] = readuint32_t(r);
    readuint32_t(r); //vendor
    os2->fsSelection = readuint16_t(r);
    os2->fsFirstCharIndex = readuint16_t(r);
    os2->fsLastCharIndex = readuint16_t(r);
    os2->sTypoAscender = readint16_t(r);
    os2->sTypoDescender = readint16_t(r);
    os2->sTypoLineGap = readint16_t(r);
    os2->usWinAscent = readuint16_t(r);
    os2->usWinDescent = readuint16_t(r);
    if (version<1) return os2;
    os2->ulCodePageRange1 = readuint32_t(r);
    os2->ulCodePageRange2 = readuint32_t(r);
    if (version<2) return os2;
    os2->sxHeight = readint16_t(r);
    os2->sCapHeight = readint16_t(r);
    os2->usDefaultChar = readuint16_t(r);
    os2->usBreakChar = readuint16_t(r);
    os2->usMaxContext = readuint16_t(r);

    if (r->pos < r->size) {
        msg("<warning> Leftover bytes (%d) in OS2 tag", r->size - r->pos);
    }
    return os2;
}
static void os2_write(ttf_t*ttf, ttf_table_t*w)
{
    table_os2_t*os2 = ttf->os2;
    uint16_t version=1;
    if (os2->sxHeight|os2->sCapHeight|os2->usDefaultChar|os2->usBreakChar|os2->usMaxContext) {
        version=2;
    }
    writeuint16_t(w, version);
    writeint16_t(w, os2->xAvgCharWidth);
    writeuint16_t(w, os2->usWeightClass);
    writeuint16_t(w, os2->usWidthClass);
    writeuint16_t(w, 0); //fstype
    writeuint16_t(w, os2->ySubscriptXSize);
    writeuint16_t(w, os2->ySubscriptYSize);
    writeuint16_t(w, os2->ySubscriptXOffset);
    writeuint16_t(w, os2->ySubscriptYOffset);
    writeuint16_t(w, os2->ySuperscriptXSize);
    writeuint16_t(w, os2->ySuperscriptYSize);
    writeuint16_t(w, os2->ySuperscriptXOffset);
    writeuint16_t(w, os2->ySuperscriptYOffset);
    writeuint16_t(w, os2->yStrikeoutSize);
    writeuint16_t(w, os2->yStrikeoutPosition);
    writeuint16_t(w, os2->sFamilyClass);
    writeuint8_t(w, os2->panose_FamilyType);
    writeuint8_t(w, os2->panose_SerifStyle);
    writeuint8_t(w, os2->panose_Weight);
    writeuint8_t(w, os2->panose_Proportion);
    writeuint8_t(w, os2->panose_Contrast);
    writeuint8_t(w, os2->panose_StrokeVariation);
    writeuint8_t(w, os2->panose_ArmStyle);
    writeuint8_t(w, os2->panose_Letterform);
    writeuint8_t(w, os2->panose_Midline);
    writeuint8_t(w, os2->panose_XHeight);
    writeuint32_t(w, os2->ulCharRange[0]);
    writeuint32_t(w, os2->ulCharRange[1]);
    writeuint32_t(w, os2->ulCharRange[2]);
    writeuint32_t(w, os2->ulCharRange[3]);
    writeuint32_t(w, 0x53434244); //vendor
    writeuint16_t(w, os2->fsSelection);
    writeuint16_t(w, os2->fsFirstCharIndex);
    writeuint16_t(w, os2->fsLastCharIndex);
    writeint16_t(w, os2->sTypoAscender);
    writeint16_t(w, os2->sTypoDescender);
    writeint16_t(w, os2->sTypoLineGap);
    writeuint16_t(w, os2->usWinAscent);
    writeuint16_t(w, os2->usWinDescent);
    if (version<1) return;
    writeuint32_t(w, os2->ulCodePageRange1);
    writeuint32_t(w, os2->ulCodePageRange2);
    if (version<2) return;
    writeint16_t(w, os2->sxHeight);
    writeint16_t(w, os2->sCapHeight);
    writeuint16_t(w, os2->usDefaultChar);
    writeuint16_t(w, os2->usBreakChar);
    writeuint16_t(w, os2->usMaxContext);
}
static void os2_dump(ttf_t*ttf)
{
    table_os2_t*os2 = ttf->os2;
    if (!os2) return;
    printf("os2->xAvgCharWidth: %d\n", os2->xAvgCharWidth);
    printf("os2->usWeightClass: %d\n", os2->usWeightClass);
    printf("os2->usWidthClass: %d\n", os2->usWidthClass);
    printf("os2->ySubscriptXSize: %d\n", os2->ySubscriptXSize);
    printf("os2->ySubscriptYSize: %d\n", os2->ySubscriptYSize);
    printf("os2->ySubscriptXOffset: %d\n", os2->ySubscriptXOffset);
    printf("os2->ySubscriptYOffset: %d\n", os2->ySubscriptYOffset);
    printf("os2->ySuperscriptXSize: %d\n", os2->ySuperscriptXSize);
    printf("os2->ySuperscriptYSize: %d\n", os2->ySuperscriptYSize);
    printf("os2->ySuperscriptXOffset: %d\n", os2->ySuperscriptXOffset);
    printf("os2->ySuperscriptYOffset: %d\n", os2->ySuperscriptYOffset);
    printf("os2->yStrikeoutSize: %d\n", os2->yStrikeoutSize);
    printf("os2->yStrikeoutPosition: %d\n", os2->yStrikeoutPosition);
    printf("os2->sFamilyClass: %d\n", os2->sFamilyClass);
    printf("os2->panose_FamilyType: %d\n", os2->panose_FamilyType);
    printf("os2->panose_SerifStyle: %d\n", os2->panose_SerifStyle);
    printf("os2->panose_Weight: %d\n", os2->panose_Weight);
    printf("os2->panose_Proportion: %d\n", os2->panose_Proportion);
    printf("os2->panose_Contrast: %d\n", os2->panose_Contrast);
    printf("os2->panose_StrokeVariation: %d\n", os2->panose_StrokeVariation);
    printf("os2->panose_ArmStyle: %d\n", os2->panose_ArmStyle);
    printf("os2->panose_Letterform: %d\n", os2->panose_Letterform);
    printf("os2->panose_Midline: %d\n", os2->panose_Midline);
    printf("os2->panose_XHeight: %d\n", os2->panose_XHeight);
    printf("os2->ulCharRange[0]: %d\n", os2->ulCharRange[0]);
    printf("os2->ulCharRange[1]: %d\n", os2->ulCharRange[1]);
    printf("os2->ulCharRange[2]: %d\n", os2->ulCharRange[2]);
    printf("os2->ulCharRange[3]: %d\n", os2->ulCharRange[3]);
    printf("os2->fsSelection: %d\n", os2->fsSelection);
    printf("os2->fsFirstCharIndex: %d\n", os2->fsFirstCharIndex);
    printf("os2->fsLastCharIndex: %d\n", os2->fsLastCharIndex);
    printf("os2->sTypoAscender: %d\n", os2->sTypoAscender);
    printf("os2->sTypoDescender: %d\n", os2->sTypoDescender);
    printf("os2->sTypoLineGap: %d\n", os2->sTypoLineGap);
    printf("os2->usWinAscent: %d\n", os2->usWinAscent);
    printf("os2->usWinDescent: %d\n", os2->usWinDescent);
    printf("os2->ulCodePageRange1: %d\n", os2->ulCodePageRange1);
    printf("os2->ulCodePageRange2: %d\n", os2->ulCodePageRange2);
    printf("os2->sxHeight: %d\n", os2->sxHeight);
    printf("os2->sCapHeight: %d\n", os2->sCapHeight);
    printf("os2->usDefaultChar: %d\n", os2->usDefaultChar);
    printf("os2->usBreakChar: %d\n", os2->usBreakChar);
    printf("os2->usMaxContext: %d\n", os2->usMaxContext);
}
static void os2_delete(ttf_t*ttf)
{
    if (ttf->os2)
        free(ttf->os2);
    ttf->os2=0;
}

static table_maxp_t*maxp_new(ttf_t*ttf)
{
    table_maxp_t*maxp = calloc(1, sizeof(table_maxp_t));
    int t;
    maxp->maxContours=1;
    if (ttf->num_glyphs) {
        int max = 1;
        for(t=0;t<ttf->num_glyphs;t++) {
            if (ttf->glyphs[t].num_points>max)
                max = ttf->glyphs[t].num_points;
            int contours = 0;
            int s;
            for(s=0;s<ttf->glyphs[t].num_points;s++) {
                if (ttf->glyphs[t].points[s].flags&GLYPH_CONTOUR_END)
                    contours++;
            }
            if (maxp->maxContours < contours)
                maxp->maxContours = contours;
        }
        maxp->maxPoints = max;

        /* we don't generate composite glyphs yet */
        maxp->maxComponentPoints = 0;
        maxp->maxComponentContours = 0;
    }
    maxp->maxZones = 2; // we don't use the Z0 zone
    return maxp;
}
static table_maxp_t* maxp_parse(ttf_t*ttf, memreader_t*r)
{
    uint32_t version = readuint32_t(r);
    ttf->num_glyphs = readuint16_t(r);
    /* according to freetype, older fonts (version<0x10000)
       apparently only contain the number of glyphs. this is
       rather rare, though. */
    if (version<0x10000 && r->size==6) return 0;

    if (r->size<32)
        msg("<warning> Truncated maxp table (version %d)", version);

    table_maxp_t*maxp = calloc(1, sizeof(table_maxp_t));
    maxp->maxPoints = readuint16_t(r);
    maxp->maxContours = readuint16_t(r);
    maxp->maxComponentPoints = readuint16_t(r);
    maxp->maxComponentContours = readuint16_t(r);
    maxp->maxZones = readuint16_t(r);
    maxp->maxTwilightPoints = readuint16_t(r);
    maxp->maxStorage = readuint16_t(r);
    maxp->maxFunctionDefs = readuint16_t(r);
    maxp->maxInstructionDefs = readuint16_t(r);
    maxp->maxStackElements = readuint16_t(r);
    maxp->maxSizeOfInstructions = readuint16_t(r);
    maxp->maxComponentElements = readuint16_t(r);
    maxp->maxComponentDepth = readuint16_t(r);
    return maxp;
}
static void maxp_write(ttf_t*ttf, ttf_table_t*w)
{
    table_maxp_t*maxp = ttf->maxp;
    if (!maxp) {
        /* version 0.5 simplified maxp table */
        writeuint32_t(w, 0x00005000);
        writeuint16_t(w, ttf->num_glyphs);
        return;
    }
    writeuint32_t(w, 0x10000); //version
    writeuint16_t(w, ttf->num_glyphs);
    writeuint16_t(w, maxp->maxPoints);
    writeuint16_t(w, maxp->maxContours);
    writeuint16_t(w, maxp->maxComponentPoints);
    writeuint16_t(w, maxp->maxComponentContours);
    writeuint16_t(w, maxp->maxZones);
    writeuint16_t(w, maxp->maxTwilightPoints);
    writeuint16_t(w, maxp->maxStorage);
    writeuint16_t(w, maxp->maxFunctionDefs);
    writeuint16_t(w, maxp->maxInstructionDefs);
    writeuint16_t(w, maxp->maxStackElements);
    writeuint16_t(w, maxp->maxSizeOfInstructions);
    writeuint16_t(w, maxp->maxComponentElements);
    writeuint16_t(w, maxp->maxComponentDepth);
}
static void maxp_dump(ttf_t*ttf)
{
    table_maxp_t*maxp = ttf->maxp;
    if (!maxp) return;
    printf("maxp->maxPoints: %d\n", maxp->maxPoints);
    printf("maxp->maxContours: %d\n", maxp->maxContours);
    printf("maxp->maxComponentPoints: %d\n", maxp->maxComponentPoints);
    printf("maxp->maxComponentContours: %d\n", maxp->maxComponentContours);
    printf("maxp->maxZones: %d\n", maxp->maxZones);
    printf("maxp->maxTwilightPoints: %d\n", maxp->maxTwilightPoints);
    printf("maxp->maxStorage: %d\n", maxp->maxStorage);
    printf("maxp->maxFunctionDefs: %d\n", maxp->maxFunctionDefs);
    printf("maxp->maxInstructionDefs: %d\n", maxp->maxInstructionDefs);
    printf("maxp->maxStackElements: %d\n", maxp->maxStackElements);
    printf("maxp->maxSizeOfInstructions: %d\n", maxp->maxSizeOfInstructions);
    printf("maxp->maxComponentElements: %d\n", maxp->maxComponentElements);
    printf("maxp->maxComponentDepth: %d\n", maxp->maxComponentDepth);
}
static void maxp_delete(ttf_t*ttf)
{
    if (ttf->maxp)
        free(ttf->maxp);
    ttf->maxp=0;
}

static table_hea_t*hea_new(ttf_t*ttf)
{
    table_hea_t*hea = calloc(1, sizeof(table_hea_t));
    if (ttf->num_glyphs) {
        int t;
        for(t=0;t<ttf->num_glyphs;t++) {
            if (ttf->glyphs[t].advance > hea->advanceWidthMax)
                hea->advanceWidthMax = ttf->glyphs[t].advance;
            if (ttf->glyphs[t].bearing < hea->minLeftSideBearing)
                hea->minLeftSideBearing = ttf->glyphs[t].bearing;
            if (ttf->glyphs[t].xmax < hea->minRightSideBearing)
                hea->minRightSideBearing = ttf->glyphs[t].xmax;
            int width = ttf->glyphs[t].xmax - ttf->glyphs[t].xmin;
            if (width > hea->xMaxExtent)
                hea->xMaxExtent = width;
        }
        hea->caretSlopeRise = 1;
    }
    return hea;
}
static int hea_parse(memreader_t*r, ttf_t*ttf)
{
    table_hea_t*hea = ttf->hea = calloc(1, sizeof(table_hea_t));
    uint32_t version = readuint32_t(r);
    ttf->ascent = readint16_t(r);
    ttf->descent = readint16_t(r);
    ttf->lineGap = readint16_t(r);
    hea->advanceWidthMax = readuint16_t(r);
    hea->minLeftSideBearing = readint16_t(r);
    hea->minRightSideBearing = readint16_t(r);
    hea->xMaxExtent = readint16_t(r);
    hea->caretSlopeRise = readint16_t(r);
    hea->caretSlopeRun = readint16_t(r);
    hea->caretOffset = readint16_t(r);
    readint16_t(r); //reserved[0]
    readint16_t(r); //reserved[1]
    readint16_t(r); //reserved[2]
    readint16_t(r); //reserved[3]
    int16_t metricDataFormat = readint16_t(r); //should be 0
    if (metricDataFormat!=0) {
        msg("<error> Unknown metric format %d", metricDataFormat);
    }
    int num_advances = readuint16_t(r);
    if (num_advances > ttf->num_glyphs) {
        msg("<error> bad number of horizontal metrics: %d", num_advances);
        num_advances = ttf->num_glyphs;
    }
    return num_advances;
}
static table_hea_t*hea_write(ttf_t*ttf, ttf_table_t*w, int num_advances)
{
    table_hea_t*hea = ttf->hea;
    writeuint32_t(w, 0x00010000);
    writeint16_t(w, ttf->ascent);
    writeint16_t(w, ttf->descent);
    writeint16_t(w, ttf->lineGap);
    writeuint16_t(w, hea->advanceWidthMax);
    writeint16_t(w, hea->minLeftSideBearing);
    writeint16_t(w, hea->minRightSideBearing);
    writeint16_t(w, hea->xMaxExtent);
    writeint16_t(w, hea->caretSlopeRise);
    writeint16_t(w, hea->caretSlopeRun);
    writeint16_t(w, hea->caretOffset);
    writeint16_t(w, 0); //reserved
    writeint16_t(w, 0); //reserved
    writeint16_t(w, 0); //reserved
    writeint16_t(w, 0); //reserved
    writeint16_t(w, 0); //metricDataFormat
    writeuint16_t(w, num_advances);
    return hea;
}
static void hea_dump(ttf_t*ttf)
{
    table_hea_t*hea = ttf->hea;
    if (!hea) return;
    const char*dir = ttf->is_vertical?"v":"h";
    printf("%shea->ascent: %d\n", dir, ttf->ascent);
    printf("%shea->descent: %d\n", dir, ttf->descent);
    printf("%shea->lineGap: %d\n", dir, ttf->lineGap);
    printf("%shea->advanceWidthMax: %d\n", dir, hea->advanceWidthMax);
    printf("%shea->minLeftSideBearing: %d\n", dir, hea->minLeftSideBearing);
    printf("%shea->minRightSideBearing: %d\n", dir, hea->minRightSideBearing);
    printf("%shea->xMaxExtent: %d\n", dir, hea->xMaxExtent);
    printf("%shea->caretSlopeRise: %d\n", dir, hea->caretSlopeRise);
    printf("%shea->caretSlopeRun: %d\n", dir, hea->caretSlopeRun);
    printf("%shea->caretOffset: %d\n", dir, hea->caretOffset);
}
static void hea_delete(ttf_t*ttf)
{
    if (ttf->hea) {
        free(ttf->hea);
        ttf->hea=0;
    }
}

static void mtx_parse(memreader_t*r, ttf_t*ttf, int num_advances)
{
    uint16_t old_advance = 0;
    int t;
    if (num_advances > r->size/4)
        num_advances = r->size/4;
    for(t=0;t<num_advances;t++) {
        old_advance = ttf->glyphs[t].advance = readuint16_t(r);
        ttf->glyphs[t].bearing = readint16_t(r);
    }
    int rest = (r->size - num_advances*4)/2;
    if (ttf->num_glyphs < num_advances+rest) {
        rest = ttf->num_glyphs-num_advances;
    }
    for(t=0;t<rest;t++) {
        ttf->glyphs[t].advance = old_advance;
        ttf->glyphs[t].bearing = readint16_t(r);
    }
}
static int mtx_write(ttf_t*ttf, ttf_table_t*w)
{
    int num_advances = ttf->num_glyphs;
    if (ttf->num_glyphs>=2) {
        int t;
        for(t=ttf->num_glyphs-1;t>0;t--) {
            if (ttf->glyphs[t-1].advance !=
               ttf->glyphs[t].advance) break;
        }
        /* we need to store all individual advances as well
           as one entry for the constant */
        num_advances = t+1;
    }

    int t;
    for(t=0;t<num_advances;t++) {
        writeuint16_t(w, ttf->glyphs[t].advance);
        writeint16_t(w, ttf->glyphs[t].bearing);
    }
    for(;t<ttf->num_glyphs;t++) {
        writeint16_t(w, ttf->glyphs[t].bearing);
    }
    return num_advances;
}

static uint32_t*loca_parse(memreader_t*r, ttf_t*ttf, int size)
{
    int t;
    int num = ttf->num_glyphs+1;
    uint32_t*locations = calloc(1, num*sizeof(uint32_t));
    uint32_t lastloc = 0;
    uint32_t loc = 0;
    char warn_unsorted = 1;
    if (size) {
        if (num*4 > r->size) {
            msg("<warning> Short 'loca' table (32 bit): %d/%d", r->size/4, num);
            num=r->size/4;
        }
        if (num*4 < r->size) {
            msg("<warning> Extraneous data (%d bytes) in 'loca' table (32 bit)", r->size-num*4);
        }
        for(t=0;t<num;t++) {
            locations[t] = loc = readuint32_t(r);
            if (lastloc > loc && warn_unsorted) {
                msg("<warning> Unsorted 'loca' table (32 bit)");
                warn_unsorted=0;
            }
            lastloc = loc;
        }
    } else {
        if (num*2 > r->size) {
            msg("<warning> Short 'loca' table (16 bit)");
            num=r->size/2;
        }
        if (num*2 < r->size) {
            msg("<warning> Extraneous data (%d bytes) in 'loca' table (16 bit)", r->size-num*2);
        }
        for(t=0;t<num;t++) {
            locations[t] = loc = readuint16_t(r)*2;
            if (lastloc > loc && warn_unsorted) {
                msg("<warning> Unsorted 'loca' table");
                warn_unsorted=0;
            }
            lastloc = loc;
        }
    }
    return locations;
}
static int loca_write(ttf_t*ttf, ttf_table_t*w, uint32_t*locations)
{
    int t;
    char use_32bit = 0;
    for(t=0;t<=ttf->num_glyphs;t++) {
        if (locations[t]>=0x20000 || (locations[t]&1)) {
            use_32bit = 1;
            break;
        }
    }

    if (use_32bit) {
        for(t=0;t<=ttf->num_glyphs;t++) {
            writeuint32_t(w, locations[t]);
        }
        return 1;
    } else {
        for(t=0;t<=ttf->num_glyphs;t++) {
            writeuint16_t(w, locations[t]/2);
        }
        return 0;
    }
}

static int parse_simple_glyph(ttf_t*ttf, memreader_t*r, int num_contours, int glyphnr)
{
    ttfglyph_t*glyph = &ttf->glyphs[glyphnr];

    uint16_t*endpoints = 0;
    if (num_contours>0) {
        endpoints = malloc(sizeof(uint16_t)*num_contours);
        int s;
        int lastpos = -1;
        for(s=0;s<num_contours;s++) {
            int pos = endpoints[s] = readuint16_t(r);
            if (pos<=lastpos) {
                msg("<warning> Unsorted endpoints array (len:%d) last=%d now=%d", s, pos, lastpos);
            }
            lastpos = pos;
        }
    }
    uint16_t code_len = readuint16_t(r);
    if (code_len) {
        glyph->code = malloc(sizeof(uint16_t)*code_len);
        readBlock(r, glyph->code, code_len);
        glyph->code_size = code_len;
    }

    if (!endpoints)
        return 1;

    /*msg("<notice> TTF Glyph %d) code_size=%d num_contours=%d glyph->num_points=%d %d/%d/%d/%d",
            glyphnr, code_len, num_contours, glyph->num_points,
            xmin, ymin, xmax, ymax);*/
    INIT_READ(fx, r->mem, r->size, r->pos);
    INIT_READ(fy, r->mem, r->size, r->pos);

    glyph->num_points = endpoints[num_contours-1] + 1;
    glyph->points = calloc(1, sizeof(ttfpoint_t)*glyph->num_points);

    /* parse flag array (1st pass- to determine start of coordinates) */
    int num=0;
    while (num<glyph->num_points) {
        uint8_t flag = readuint8_t(r);
        if (flag&0xc0) {
            msg("<error> Bad flags in glyph outline: %02x (at pos %d)", flag, num);
            free(glyph->points);
            glyph->points = 0;
            glyph->num_points = 0;
            return 0;
        }
        int count = 1;
        if (flag & 0x08)
            count += readuint8_t(r);
        if (count+num>glyph->num_points) {
            msg("<warning> Bad count (%d) in glyph (%d) (at pos %d)", count, glyphnr, num);
            count = glyph->num_points-num;
        }
        num+=count;
    }

    /* parse flag array (2nd pass) and x coordinates */
    num=0;
    int x = 0;
    char is_start=1;
    int contour_pos=0;
    int bytepos = r->pos;
    while (num<glyph->num_points) {
        uint8_t flag = readuint8_t(&fx);
        int count = flag&8?readuint8_t(&fx)+1:1;
        count=count>glyph->num_points-num?glyph->num_points-num:(count?count:1);
        do {
            char is_end=0;
            if (contour_pos<num_contours && num==endpoints[contour_pos]) {
                contour_pos++;
                is_end=1;
            }
            int oldx = x;
            if ((flag&0x12) == 0x12) x += readuint8_t(r);
            else if ((flag&0x12) == 0x02) x -= readuint8_t(r);
            else if ((flag&0x12) == 0x00) x += readint16_t(r);

            glyph->points[num].x = x;
            uint8_t f = flag&GLYPH_ON_CURVE;
            if (is_start) f|=GLYPH_CONTOUR_START;
            if (is_end) f|=GLYPH_CONTOUR_END;
            glyph->points[num].flags = f;
            num++;
            is_start = is_end;
        } while (--count);
    }

    /* parse flag array (3rd pass) and y coordinates */
    num=0;
    int y = 0;
    while (num<glyph->num_points) {
        uint8_t flag = readuint8_t(&fy);
        int count = flag&8?readuint8_t(&fy)+1:1;
        count=count>glyph->num_points-num?glyph->num_points-num:(count?count:1);
        do {
            if ((flag&0x24) == 0x24) y += readuint8_t(r);
            else if ((flag&0x24) == 0x04) y -= readuint8_t(r);
            else if ((flag&0x24) == 0x00) y += readint16_t(r);
            glyph->points[num].y = y;
            num++;
        } while (--count);
    }
    free(endpoints);
    return 1;
}
static void glyf_parse(memreader_t*rr, ttf_t*ttf, uint32_t*loca)
{
    int t;
    char warn_about_compound_glyphs=1;
    for(t=0;t<ttf->num_glyphs;t++) {
        INIT_READ(r, rr->mem, rr->size, loca[t]);
        if (loca[t]==loca[t+1] || loca[t]==r.size) {
            continue; //empty glyph
        }
        if (r.pos+10>r.size) {
            msg("<warning> Truncated glyph entry %d/%d (or bad loca entry %d/%d, next loca: %d)",
                    t, ttf->num_glyphs, loca[t], r.size, loca[t+1]);
            break;
        }
        int16_t num_contours = readint16_t(&r);
        ttf->glyphs[t].xmin = readint16_t(&r);
        ttf->glyphs[t].ymin = readint16_t(&r);
        ttf->glyphs[t].xmax = readint16_t(&r);
        ttf->glyphs[t].ymax = readint16_t(&r);

        if (num_contours<0) {
            if (warn_about_compound_glyphs)
                msg("<error> Compound glyphs not supported yet");
            warn_about_compound_glyphs=0;
        } else {
            if (!parse_simple_glyph(ttf, &r, num_contours, t))
                return;
        }
    }

}
void write_simple_glyph(ttf_table_t*w, ttfglyph_t*g)
{
    /* endpoints array */
    int s;
    for(s=0;s<g->num_points;s++) {
        if (g->points[s].flags&GLYPH_CONTOUR_END)
            writeuint16_t(w, s);
    }

    /* bytecode */
    writeuint16_t(w, g->code_size);
    if (g->code_size)
        writeBlock(w, g->code, g->code_size);

    /* flags */
    int lastx=0;
    int lasty=0;
    int lastflag=-1;
    int flagcount=0;
    for(s=0;s<g->num_points;s++) {
        ttfpoint_t*p = &g->points[s];
        int dx = p->x - lastx;
        int dy = p->y - lasty;
        uint8_t flags = p->flags&GLYPH_ON_CURVE;
        if (!dx) {
            flags|=0x10;
        } else if (dx<0 && dx>=-255) {
            flags|=0x02;
        } else if (dx>0 && dx<=255) {
            flags|=0x12;
        }
        if (!dy) {
            flags|=0x20;
        } else if (dy<0 && dy>=-255) {
            flags|=0x04;
        } else if (dy>0 && dy<=255) {
            flags|=0x24;
        }
        if (flags == lastflag && flagcount<255) {
            flagcount++;
        } else {
            if (lastflag>=0) {
                if (flagcount) {
                    writeuint8_t(w, lastflag|8);
                    writeuint8_t(w, flagcount);
                } else {
                    writeuint8_t(w, lastflag);
                }
            }
            lastflag = flags;
            flagcount = 0;
        }
        lastx = p->x;
        lasty = p->y;
    }
    if (lastflag>=0) {
        if (flagcount) {
            writeuint8_t(w, lastflag|8);
            writeuint8_t(w, flagcount);
        } else {
            writeuint8_t(w, lastflag);
        }
    }
    /* coordinates */
    lastx=0;
    int bytepos = w->len;
    for(s=0;s<g->num_points;s++) {
        ttfpoint_t*p = &g->points[s];
        int dx = p->x - lastx;
        if (dx>32767 || dx<-32768) {
            msg("<error> Coordinate overflow in glyph");
        }
        lastx = p->x;
        if (dx>0 && dx<=255) writeuint8_t(w, dx);
        else if (dx<0 && dx>=-255) writeuint8_t(w, -dx);
        else if (dx) writeint16_t(w, dx);
    }

    lasty=0;
    for(s=0;s<g->num_points;s++) {
        ttfpoint_t*p = &g->points[s];
        int dy = p->y - lasty;
        if (dy>32767 || dy<-32768) {
            msg("<error> Coordinate overflow in glyph");
        }
        lasty = p->y;
        if (dy>0 && dy<=255) writeuint8_t(w, dy);
        else if (dy<0 && dy>=-255) writeuint8_t(w, -dy);
        else if (dy) writeint16_t(w, dy);
    }
}
uint32_t* glyf_write(ttf_t* ttf, ttf_table_t*w)
{
    uint32_t*locations = malloc(sizeof(uint32_t)*(ttf->num_glyphs+1));
    int t;
    for(t=0;t<ttf->num_glyphs;t++) {
        locations[t] = w->len;
        ttfglyph_t*g = &ttf->glyphs[t];
        int s;
        int num_contours = 0;
        for(s=0;s<g->num_points;s++) {
            if (g->points[s].flags&GLYPH_CONTOUR_END)
                num_contours++;
        }
        writeint16_t(w, num_contours?num_contours:1);
        writeint16_t(w, g->xmin);
        writeint16_t(w, g->ymin);
        writeint16_t(w, g->xmax);
        writeint16_t(w, g->ymax);

        if (!num_contours) {
            /* some ttf parsers can't deal with zero contours, so in the case
               of an empty glyph, write a single point (0,0) */
            writeuint16_t(w, 0); //endpoint of 1st contour
            writeuint16_t(w, g->code_size);
            if (g->code_size)
                writeBlock(w, g->code, g->code_size);
            writeuint8_t(w, 0x31); //flag (xy=(0,0),on curve)
        } else {
            write_simple_glyph(w, g);
        }
    }
    locations[t] = w->len;
    return locations;
}
void glyf_dump(ttf_t* ttf)
{
    if (!ttf->glyphs) return;
    int t;
    for(t=0;t<ttf->num_glyphs;t++) {
        ttfglyph_t*g = &ttf->glyphs[t];
        printf("glyph %d)\n", t);
        printf("  advance=%d\n", g->advance);
        printf("  bearing=%d\n", g->bearing);
        printf("  bbox=(%d/%d/%d/%d)\n", g->xmin, g->ymin, g->xmax, g->ymax);
        printf("  points=(");
        int s;
        for(s=0;s<g->num_points;s++) {
            if (s) printf(",");
            printf("%d/%d/0x%02x", g->points[s].x, g->points[s].y, g->points[s].flags);
        }
        printf(")\n");
        if (g->code_size)
            hexdump(g->code, g->code_size, "  ");
    }
}
void glyf_delete(ttf_t* ttf)
{
    if (!ttf->glyphs)
        return;
    int t;
    for(t=0;t<ttf->num_glyphs;t++) {
        if (ttf->glyphs[t].code) {
            free(ttf->glyphs[t].code);
            ttf->glyphs[t].code = 0;
        }
        if (ttf->glyphs[t].points) {
            free(ttf->glyphs[t].points);
            ttf->glyphs[t].points = 0;
        }
    }
    free(ttf->glyphs);ttf->glyphs=0;
}

ttfglyph_t* ttf_find_unicode(ttf_t*ttf, uint32_t unicode)
{
    if (ttf->unicode && unicode < ttf->unicode_size) {
        return &ttf->glyphs[ttf->unicode[unicode]];
    }
    return NULL;
}


static void grow_unicode(ttf_t*ttf, int index)
{
    int size = index+1;
    if (!ttf->unicode) {
        ttf->unicode = calloc(size, sizeof(ttf->unicode[0]));
    } else if (ttf->unicode_size<size) {
        ttf->unicode = realloc(ttf->unicode, sizeof(ttf->unicode[0])*size);
        memset(ttf->unicode+ttf->unicode_size, 0, sizeof(ttf->unicode[0])*(size - ttf->unicode_size));
    }
    ttf->unicode_size = size;
}
void cmap_parse(memreader_t*r, ttf_t*ttf)
{
    readuint16_t(r); // version (0)
    int num_subtables = readuint16_t(r);
    int t;
    char warn=1;
    if (r->pos+num_subtables*8 > r->size) {
        msg("<warning> CMap overflow");
        num_subtables = (r->size-r->pos)/8;
    }
    unicode_t*data = 0;
    for(t=0;t<num_subtables;t++) {
        uint16_t platform = readuint16_t(r);
        uint16_t encoding = readuint16_t(r);
        uint32_t offset = readuint32_t(r);
        if (offset>r->size) {
            msg("<warning> CMAP table %d %d is out of bounds (%d)", platform, encoding, offset);
            continue;
        }

        int is_unicode = platform==0 ||
                         platform==3 && encoding == 1 ||
                         platform==3 && encoding == 10;

        if (!is_unicode)
            continue;

        INIT_READ(t, r->mem, r->size, offset);
        uint16_t format = readuint16_t(&t);
        int length = readuint16_t(&t);
        uint16_t language = readuint16_t(&t);

        if (language)
            msg("<warning> Language code %02x in unicode mapping", language);

        int num = 0;
        if (format == 0) {
            num = length-6;
            if (t.pos+length > t.size) {
                msg("<warning> overflow in format 0 cmap table");
                num = t.size-t.pos;
            }
            data = malloc(num*sizeof(unicode_t));
            int s;
            grow_unicode(ttf, num);
            for(s=0;s<num;s++) {
                ttf->unicode[s] = readuint8_t(&t);
            }
        } else if (format == 4) {
            uint16_t segment_count = readuint16_t(&t);
            if (segment_count&1) {
                msg("<error> Bad segmentx2 count %d", segment_count);
                continue;
            }
            segment_count>>=1;
            readuint16_t(&t); //searchrange
            readuint16_t(&t); //entry selector
            readuint16_t(&t); //range shift
            INIT_READ(r_end, t.mem, t.size, t.pos);
            INIT_READ(r_start, t.mem, t.size, t.pos+2+segment_count*2);
            INIT_READ(r_delta, t.mem, t.size, t.pos+2+segment_count*4);
            INIT_READ(r_range, t.mem, t.size, t.pos+2+segment_count*6);
            int glyphmap_start = t.pos+2+segment_count*8;
            int glyphmap_size = t.size - glyphmap_start;
            int s;
            for(s=0;s<segment_count;s++) {
                uint16_t start = readuint16_t(&r_start);
                uint16_t end = readuint16_t(&r_end);
                uint16_t delta = readuint16_t(&r_delta);
                uint16_t range = readuint16_t(&r_range);
                if (start==0xffff && end==0xffff && delta==1) {
                    /* this is a common (maybe even required) occurence in fonts
                       which explicitly map "unicode undefined" (0xffff) to
                       "glyph undefined" (0).
                       We don't want to blow our unicode table up to 65536 just
                       because of this, so ignore this entry.
                     */
                    continue;
                }
                grow_unicode(ttf, end);
                int u;
                if (!range) {
                    for(u=start;u<=end;u++) {
                        ttf->unicode[u] = (u + delta) & 0xffff;
                    }
                } else {
                    int pos = r_range.pos-2+range;
                    if (warn && pos+end-start+1 > t.size) {
                        msg("<warning> glyphmap index out of bounds (%d-%d/%d)", pos, pos+end-start, t.size);
                        warn=0;
                    }
                    INIT_READ(g, t.mem, t.size, pos);
                    for(u=start;u<=end;u++) {
                        ttf->unicode[u] = readuint16_t(&g);
                    }
                }
            }
        }
    }
}
static int segment_size(unicode_t*unicode, int pos, int size)
{
    int s;
    int count=0;
    for(s=pos;s<size;s++) {
        if (!unicode[s])
            count++;
        if (count>4) {
            /* a segment costs us 8 bytes, so for more than 4 consecutive
               zero entries (16 bit each) in the glyph index array,
               it pays off to start a new segment */
            break;
        }
    }
    s -= count; // go to the last filled in entry
    if (s==size)
        return size-1;
    return s;
}
void cmap_write(ttf_t* ttf, ttf_table_t*w)
{
    writeuint16_t(w, 0);  //version
    writeuint16_t(w, 2);  //two tables

    writeuint16_t(w, 0);  //platform (unicode)
    writeuint16_t(w, 3);  //encoding (unicode 2.0)
    writeuint32_t(w, 20); //offset

    writeuint16_t(w, 3);  //platform (windows)
    writeuint16_t(w, 1);  //encoding (unicode basic multilingual plane UCS-2)
    writeuint32_t(w, 20); //offset

    writeuint16_t(w, 4); // format=4
    int length_pos = w->len;
    writeuint16_t(w, 0); // length: we don't know yet
    writeuint16_t(w, 0); // language (n/a for unicode)
    int num_segments_pos = w->len;
    writeuint16_t(w, 0); //number of segments: we don't know yet either
    writeuint16_t(w, 0); //searchrange
    writeuint16_t(w, 0); //entry selector
    writeuint16_t(w, 0); //range shift

    int pos=0;
    int num_segments=0;
    while (pos < ttf->unicode_size) {
        if (!ttf->unicode[pos]) {
            pos++;
            continue;
        }
        int s = segment_size(ttf->unicode, pos, ttf->unicode_size);
        pos = s+1;
        num_segments++;
    }

    num_segments++; // account for 0xffff mapping

    int glyphmap_start = w->len+2+num_segments*8;

    int t;
    int end_pos = w->len;
    for(t=0;t<num_segments;t++) {writeuint16_t(w, 0);} //end array
    writeuint16_t(w, 0); //reserved byte
    int start_pos = w->len;
    for(t=0;t<num_segments;t++) {writeuint16_t(w, 0);} //start array
    int delta_pos = w->len;
    for(t=0;t<num_segments;t++) {writeuint16_t(w, 0);} //delta array
    int range_pos = w->len;
    for(t=0;t<num_segments;t++) {writeuint16_t(w, 0);} //range array

    /* backpatch number of segments */
    w->data[num_segments_pos++]=(num_segments*2)>>8;
    w->data[num_segments_pos++]=(num_segments*2);
    /* backpatch search range */
    int tmp = num_segments;
    int search_range = 0;
    while (tmp) {
        search_range = tmp;
        tmp = tmp&(tmp-1);
    }
    w->data[num_segments_pos++]=(search_range*2)>>8;
    w->data[num_segments_pos++]=(search_range*2);
    /* backpatch entry selector */
    int entry_selector = 0;
    tmp = search_range;
    while (tmp>1) {tmp>>=1;entry_selector++;}
    w->data[num_segments_pos++]=entry_selector>>8;
    w->data[num_segments_pos++]=entry_selector;
    /* backpatch range shift */
    int range_shift = num_segments*2 - search_range*2;
    w->data[num_segments_pos++]=range_shift>>8;
    w->data[num_segments_pos++]=range_shift;

    pos=0;
    num_segments = 0;
    while (pos < ttf->unicode_size) {
        if (!ttf->unicode[pos]) {
            pos++;
            continue;
        }
        uint16_t end = segment_size(ttf->unicode, pos, ttf->unicode_size);
        w->data[end_pos++]=end>>8;
        w->data[end_pos++]=end;
        w->data[start_pos++]=pos>>8;
        w->data[start_pos++]=pos;
        int s;
        uint16_t delta = ttf->unicode[pos]-pos;
        char do_delta=1;
        for(s=pos+1;s<=end;s++) {
            uint16_t delta2 = ttf->unicode[s]-s;
            if (delta2!=delta) {
                do_delta=0;
                break;
            }
        }
        uint16_t range;
        if (do_delta) {
            range = 0;
        } else {
            delta = 0;
            range = w->len - range_pos;
            for(s=pos;s<=end;s++) {
                writeuint16_t(w, ttf->unicode[s]);
            }
        }
        w->data[delta_pos++]=delta>>8;
        w->data[delta_pos++]=delta;
        w->data[range_pos++]=range>>8;
        w->data[range_pos++]=range;
        num_segments++;
        pos = end+1;
    }

    /* write out a mapping from 0xffff to 0- seems to be required
       by some libraries (e.g. fonttools) */
    w->data[end_pos++]=0xff;
    w->data[end_pos++]=0xff;
    w->data[start_pos++]=0xff;
    w->data[start_pos++]=0xff;
    w->data[delta_pos++]=0;
    w->data[delta_pos++]=1;
    w->data[range_pos++]=0;
    w->data[range_pos++]=0;

    w->data[length_pos]=(w->len-20)>>8;
    w->data[length_pos+1]=w->len-20;
}
void cmap_delete(ttf_t*ttf)
{
    if (ttf->unicode) {
        free(ttf->unicode);
        ttf->unicode=0;
    }
    ttf->unicode_size=0;
}
static char*readString(memreader_t*r, int len)
{
    char*s = malloc(len+1);
    readBlock(r, s, len);
    s[len] = 0;
    return s;
}
void name_parse(memreader_t*r, ttf_t*ttf)
{
    uint16_t format = readuint16_t(r);
    uint16_t count = readuint16_t(r);
    uint16_t offset = readuint16_t(r);

    int t;
    for(t=0;t<count;t++) {
        uint16_t platform = readuint16_t(r);
        uint16_t encoding = readuint16_t(r);
        uint16_t language = readuint16_t(r);
        uint16_t name_id = readuint16_t(r);
        uint16_t len = readuint16_t(r);
        uint16_t offset_2 = readuint16_t(r);

        char ** read_name = 0;

        INIT_READ(ss, r->mem, r->size, offset+offset_2);
        if (!(platform==0 || (platform==1 && encoding==0)))
                continue;

        INIT_READ(s, r->mem, r->size, offset+offset_2);

        switch (name_id) {
            case 1: read_name = &ttf->family_name; break;
            case 2: read_name = &ttf->subfamily_name; break;
            case 3: read_name = &ttf->font_uid; break;
            case 4: read_name = &ttf->full_name; break;
            case 5: read_name = &ttf->version_string; break;
            case 6: read_name = &ttf->postscript_name; break;
            default: read_name = 0;
        }

        if (read_name) {
            if (*read_name) free(*read_name);
            *read_name = readString(&s, len);
        }
    }
}
void name_write(ttf_t*ttf, ttf_table_t*table)
{
    char*strings[6] = {ttf->family_name, ttf->subfamily_name, ttf->font_uid, ttf->full_name, ttf->version_string, ttf->postscript_name};
    int codes[6] = {1,2,3,4,5,6};

    writeuint16_t(table, 0); //format
    int count = 0;
    int t;
    int nr = sizeof(strings)/sizeof(strings[0]);

    for(t=0;t<nr;t++) {
        if (strings[t])
            count+=2;
    }
    writeuint16_t(table, count); //count

    int offset_pos = table->len;
    writeuint16_t(table, 0); //offset (will be filled in later)

    /* Windows expects the name table to be sorted by platform/encoding/language/name_id */
    int offset = 0;
    for(t=0;t<nr;t++) {
        if (strings[t]) {
            writeuint16_t(table, 1); //platform id (mac)
            writeuint16_t(table, 0); //encoding id (latin-1)
            writeuint16_t(table, 0); //language (english)
            writeuint16_t(table, codes[t]);
            int len = strlen(strings[t]);
            writeuint16_t(table, len);
            writeuint16_t(table, offset);
            offset += len;
        }
    }
    for(t=0;t<nr;t++) {
        if (strings[t]) {
            writeuint16_t(table, 3); //platform id (windows)
            writeuint16_t(table, 1); //encoding id (ucs-2)
            writeuint16_t(table, 0x409); //language (US)
            writeuint16_t(table, codes[t]);
            int len2 = strlen(strings[t]) * 2;
            writeuint16_t(table, len2);
            writeuint16_t(table, offset);
            offset += len2;
        }
    }

    table->data[offset_pos] = table->len>>8;
    table->data[offset_pos+1] = table->len;

    for(t=0;t<nr;t++) {
        if (strings[t]) {
            int len = strlen(strings[t]);
            writeBlock(table, strings[t], len);
        }
    }
    for(t=0;t<nr;t++) {
        if (strings[t]) {
            int s;
            int len = strlen(strings[t]);
            for(s=0;s<len;s++) {
                writeuint8_t(table, 0);
                writeuint8_t(table, strings[t][s]);
            }
        }
    }
}
void name_delete(ttf_t*ttf)
{
    if (ttf->full_name) {
        free(ttf->full_name);
        ttf->full_name=0;
    }
    if (ttf->family_name) {
        free(ttf->family_name);
        ttf->family_name=0;
    }
    if (ttf->subfamily_name) {
        free(ttf->subfamily_name);
        ttf->subfamily_name=0;
    }
    if (ttf->version_string) {
        free(ttf->version_string);
        ttf->version_string=0;
    }
    if (ttf->font_uid) {
        free(ttf->font_uid);
        ttf->font_uid=0;
    }
    if (ttf->postscript_name) {
        free(ttf->postscript_name);
        ttf->postscript_name=0;
    }
}

static table_post_t*post_new(ttf_t*ttf)
{
    table_post_t*post = calloc(1, sizeof(table_post_t));
    return post;
}
void post_parse(memreader_t*r, ttf_t*ttf)
{
    table_post_t*post = ttf->post = calloc(1, sizeof(table_post_t));
    uint32_t format = readuint32_t(r);
    post->italic_angle = readuint32_t(r);
    post->underline_position = readuint16_t(r);
    post->underline_thickness = readuint16_t(r);
    uint16_t is_monospaced = readuint32_t(r);
    readuint32_t(r); // min mem 42
    readuint32_t(r);
    readuint32_t(r); // min mem 1
    readuint32_t(r);
}
void post_write(ttf_t*ttf, ttf_table_t*table)
{
    table_post_t*post = ttf->post;
    writeuint32_t(table, 0x00030000);
    writeuint32_t(table, post->italic_angle);
    writeuint16_t(table, post->underline_position);
    writeuint16_t(table, post->underline_thickness);
    writeuint32_t(table, 0); //is monospaced TODO
    writeuint32_t(table, 0); //min mem 42
    writeuint32_t(table, 0);
    writeuint32_t(table, 0); //min mem 1
    writeuint32_t(table, 0);
}
void post_delete(ttf_t*ttf)
{
    if (ttf->post) {
        free(ttf->post);
        ttf->post = 0;
    }
}

void cvt_parse(memreader_t*r, ttf_t*ttf)
{
    table_cvt_t*cvt = ttf->cvt = calloc(1, sizeof(table_cvt_t));
    cvt->num = r->size/2;
    cvt->values = malloc(cvt->num*sizeof(int16_t));
    int t;
    for(t=0;t<cvt->num;t++) {
        cvt->values[t] = readint16_t(r);
    }
}
void cvt_write(ttf_t*ttf, ttf_table_t*table)
{
    table_cvt_t*cvt = ttf->cvt;
    int t;
    for(t=0;t<cvt->num;t++) {
        writeint16_t(table, cvt->values[t]);
    }
}
void cvt_delete(ttf_t*ttf)
{
    if (ttf->cvt) {
        if (ttf->cvt->values) 
            free(ttf->cvt->values);
        free(ttf->cvt);
        ttf->cvt = 0;
    }
}

static table_gasp_t*gasp_new(ttf_t*ttf)
{
    table_gasp_t*gasp = calloc(1, sizeof(table_gasp_t));
    gasp->num = 1;
    gasp->records = calloc(1, sizeof(gasp->records[0])*gasp->num);

    gasp->records[0].size = 65535;
    gasp->records[0].behaviour = 15; //gridfit+grayscale rendering
    return gasp;
}
void gasp_parse(memreader_t*r, ttf_t*ttf)
{
    table_gasp_t*gasp = ttf->gasp = calloc(1, sizeof(table_gasp_t));
    readuint16_t(r); //version
    int num = readuint16_t(r);
    int t;
    if (!num) return;
    gasp->records = malloc(sizeof(gasp->records[0])*num);
    for(t=0;t<num;t++) {
        gasp->records[t].size = readuint16_t(r);
        gasp->records[t].behaviour = readuint16_t(r);
    }
}

#define GASP_SYMMETRIC_GRIDFIT 0x0008
#define GASP_SYMMETRIC_SMOOTHING 0x0004
#define GASP_DOGRAY 0x0002
#define GASP_GRIDFIT 0x0001

void gasp_write(ttf_t*ttf, ttf_table_t*table)
{
    table_gasp_t*gasp = ttf->gasp;
    int version = 0;
    int t;
    for(t=0;t<gasp->num;t++) {
        if (gasp->records[t].behaviour & ~(GASP_GRIDFIT | GASP_DOGRAY)) {
            version = 1;
        }
    }
    writeuint16_t(table, version);
    writeuint16_t(table, gasp->num);
    for(t=0;t<gasp->num;t++) {
        writeuint16_t(table, gasp->records[t].size);
        writeuint16_t(table, gasp->records[t].behaviour);
    }
}
void gasp_delete(ttf_t*ttf)
{
    if (ttf->gasp) {
        if (ttf->gasp->records)
            free(ttf->gasp->records);
        free(ttf->gasp);
        ttf->gasp = 0;
    }
}

table_code_t*prep_new(ttf_t*ttf)
{
    table_code_t*prep = ttf->prep = calloc(1, sizeof(table_code_t));
    ttf_table_t*t = ttf_table_new(0);
    writeuint8_t(t,0xb8);writeuint16_t(t,0x1ff); // pushword(0x1ff)
    writeuint8_t(t,0x85); //scanctrl (always do dropout, for all sizes)
    writeuint8_t(t,0xb0);writeuint8_t(t,1); // pushbyte(1)
    writeuint8_t(t,0x8d); //scantype (simple dropout control w/o stubs)
    writeuint8_t(t,0xb0);writeuint8_t(t,5); // pushbyte(5)
    writeuint8_t(t,0x8d); //scantype (for windows) smart dropout control w/o stubs
    prep->code = t->data;
    prep->size = t->len;
    free(t);
    return prep;

}
void fpgm_parse(memreader_t*r, ttf_t*ttf)
{
    table_code_t*fpgm = ttf->fpgm  = calloc(1, sizeof(table_code_t));
    if (!r->size) return;
    fpgm->size = r->size;
    fpgm->code = malloc(r->size);
    readBlock(r, fpgm->code, r->size);
}
void fpgm_write(ttf_t*ttf, ttf_table_t*table)
{
    table_code_t*code = ttf->fpgm;
    writeBlock(table, code->code, code->size);
}
void fpgm_delete(ttf_t*ttf)
{
    if (ttf->fpgm) {
        if (ttf->fpgm->code)
            free(ttf->fpgm->code);
        free(ttf->fpgm);
        ttf->fpgm = 0;
    }
}

void prep_parse(memreader_t*r, ttf_t*ttf)
{
    table_code_t*prep = ttf->prep  = calloc(1, sizeof(table_code_t));
    if (!r->size) return;
    prep->size = r->size;
    prep->code = malloc(r->size);
    readBlock(r, prep->code, r->size);
}
void prep_write(ttf_t*ttf, ttf_table_t*table)
{
    table_code_t*code = ttf->prep;
    writeBlock(table, code->code, code->size);
}
void prep_delete(ttf_t*ttf)
{
    if (ttf->prep) {
        if (ttf->prep->code)
            free(ttf->prep->code);
        free(ttf->prep);
        ttf->prep = 0;
    }
}

static int ttf_parse_tables(ttf_t*ttf)
{
    ttf_table_t*table;

    table = ttf_find_table(ttf, TAG_HEAD);
    if (!table) {
        msg("<error> Font has no head table");
        return 0;
    }
    INIT_READ(m, table->data, table->len, 0);
    int loc_index = head_parse(ttf, &m);
    ttf_table_delete(ttf, table);

    table = ttf_find_table(ttf, TAG_MAXP);
    if (!table) {
        msg("<error> Font has no maxp table");
        return 0;
    }
    INIT_READ(m2, table->data, table->len, 0);
    ttf->maxp = maxp_parse(ttf, &m2);
    ttf_table_delete(ttf, table);

    if (!ttf->num_glyphs) {
        msg("<error> Invalid number of characters");
        return 0;
    }
    ttf->glyphs = calloc(ttf->num_glyphs, sizeof(ttfglyph_t));

    table = ttf_find_table(ttf, TAG_OS2);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        ttf->os2 = os2_parse(&m);
        ttf_table_delete(ttf, table);
    }


    table = ttf_find_table(ttf, TAG_HHEA);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        int num_advances = hea_parse(&m, ttf);
        ttf_table_delete(ttf, table);

        table = ttf_find_table(ttf, TAG_HMTX);
        if (table) {
            INIT_READ(m, table->data, table->len, 0);
            mtx_parse(&m, ttf, num_advances);
            ttf_table_delete(ttf, table);
        }
    } else {
        table = ttf_find_table(ttf, TAG_VHEA);
        if (table) {
            ttf->is_vertical=1;
            INIT_READ(m, table->data, table->len, 0);
            int num_advances = hea_parse(&m, ttf);
            ttf_table_delete(ttf, table);

            table = ttf_find_table(ttf, TAG_VMTX);
            if (table) {
                INIT_READ(m, table->data, table->len, 0);
                mtx_parse(&m, ttf, num_advances);
                ttf_table_delete(ttf, table);
            }
        } else {
            msg("<error> Font contains neither HHEA nor VHEA");
        }
    }
    table = ttf_find_table(ttf, TAG_LOCA);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        uint32_t*loca = loca_parse(&m, ttf, loc_index);
        ttf_table_delete(ttf, table);
        table = ttf_find_table(ttf, TAG_GLYF);
        if (table) {
            INIT_READ(m, table->data, table->len, 0);
            glyf_parse(&m, ttf, loca);
            ttf_table_delete(ttf, table);
        }
        free(loca);
    }

    table = ttf_find_table(ttf, TAG_CMAP);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        cmap_parse(&m, ttf);
        ttf_table_delete(ttf, table);
    }

    table = ttf_find_table(ttf, TAG_POST);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        post_parse(&m, ttf);
        ttf_table_delete(ttf, table);
    }

    table = ttf_find_table(ttf, TAG_NAME);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        name_parse(&m, ttf);
        ttf_table_delete(ttf, table);
    }

    table = ttf_find_table(ttf, TAG_CVT);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        cvt_parse(&m, ttf);
        ttf_table_delete(ttf, table);
    }
    
    table = ttf_find_table(ttf, TAG_GASP);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        gasp_parse(&m, ttf);
        ttf_table_delete(ttf, table);
    }
    
    table = ttf_find_table(ttf, TAG_PREP);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        prep_parse(&m, ttf);
        ttf_table_delete(ttf, table);
    }
    
    table = ttf_find_table(ttf, TAG_FPGM);
    if (table) {
        INIT_READ(m, table->data, table->len, 0);
        fpgm_parse(&m, ttf);
        ttf_table_delete(ttf, table);
    }

    return 1;
}
static void ttf_collapse_tables(ttf_t*ttf)
{
    ttf_table_t*table;

    ttf_table_t*head = ttf_find_table(ttf, TAG_HEAD);
    if (head)
        return; //already collapsed

    if (ttf->maxp) {
        table = ttf_addtable(ttf, TAG_MAXP);
        maxp_write(ttf, table);
        maxp_delete(ttf);
    }

    if (ttf->os2) {
        table = ttf_addtable(ttf, TAG_OS2);
        os2_write(ttf, table);
        os2_delete(ttf);
    }

    if (ttf->hea) {
        if (!ttf->is_vertical) {
            table = ttf_addtable(ttf, TAG_HMTX);
            int num_advances = mtx_write(ttf, table);
            table = ttf_addtable(ttf, TAG_HHEA);
            hea_write(ttf, table, num_advances);
            hea_delete(ttf);
        } else {
            table = ttf_addtable(ttf, TAG_VMTX);
            int num_advances = mtx_write(ttf, table);
            table = ttf_addtable(ttf, TAG_VHEA);
            hea_write(ttf, table, num_advances);
            hea_delete(ttf);
        }
    }

    int loca_size=0;
    if (ttf->num_glyphs) {
        if (ttf->unicode) {
            table = ttf_addtable(ttf, TAG_CMAP);
            cmap_write(ttf, table);
            cmap_delete(ttf);
        }

        if (ttf->glyphs) {
            table = ttf_addtable(ttf, TAG_GLYF);
            uint32_t*locations = glyf_write(ttf, table);
            table = ttf_addtable(ttf, TAG_LOCA);
            loca_size = loca_write(ttf, table, locations);
            free(locations);
            glyf_delete(ttf);
        }
    }

    if (ttf->full_name || ttf->family_name || ttf->subfamily_name || ttf->font_uid || ttf->postscript_name) {
        table = ttf_addtable(ttf, TAG_NAME);
        name_write(ttf, table);
        name_delete(ttf);
    }
    if (ttf->post) {
        table = ttf_addtable(ttf, TAG_POST);
        post_write(ttf, table);
        post_delete(ttf);
    }
    if (ttf->cvt) {
        table = ttf_addtable(ttf, TAG_CVT);
        cvt_write(ttf, table);
        cvt_delete(ttf);
    }
    if (ttf->gasp) {
        table = ttf_addtable(ttf, TAG_GASP);
        gasp_write(ttf, table);
        gasp_delete(ttf);
    }
    if (ttf->fpgm) {
        table = ttf_addtable(ttf, TAG_FPGM);
        fpgm_write(ttf, table);
        fpgm_delete(ttf);
    }
    if (ttf->prep) {
        table = ttf_addtable(ttf, TAG_PREP);
        prep_write(ttf, table);
        prep_delete(ttf);
    }

    table = ttf_addtable(ttf, TAG_HEAD);
    head_write(ttf, table, loca_size);
    head_delete(ttf);
}

ttf_t*ttf_new()
{
    ttf_t*ttf = calloc(1, sizeof(ttf_t));
    ttf->version = VERSION_1_0;
    return ttf;
}

ttf_t* ttf_load(void*data, int length)
{
    INIT_READ(r,data,length, 0);

    if (length<12) {
        msg("<error> Truncated Truetype file (%d bytes)", length);
        return 0;
    }

    ttf_t*ttf = calloc(1, sizeof(ttf_t));
    ttf->version = readuint32_t(&r);
    if (ttf->version == SWAP32(length)) {
        uint32_t fontDataSize = readuint32_t(&r);
        uint32_t version = readuint32_t(&r);
        uint32_t flags  = readuint32_t(&r);
        uint8_t panose[10];
        readBlock(&r, panose, 10);
        readuint8_t(&r); //charset
        readuint8_t(&r); //italoc
        readuint32_t(&r); //weight
        readuint16_t(&r); //fstype
        uint16_t magic = readuint16_t(&r); //magicNumber
        /* we're being paranoid: it's entirely possible for the font
           size to be exactly 0x10000. Only treat this font as eot if
           it has the right magic number */
        if (magic == 0x4c50) {
            readuint32_t(&r); //unicoderange[0]
            readuint32_t(&r); //unicoderange[1]
            readuint32_t(&r); //unicoderange[2]
            readuint32_t(&r); //unicoderange[3]
            readuint32_t(&r); //codepagerange[0]
            readuint32_t(&r); //codepagerange[1]
            readuint32_t(&r); //checksumadjustment
            readuint32_t(&r); //reserved[0]
            readuint32_t(&r); //reserved[1]
            readuint32_t(&r); //reserved[2]
            readuint32_t(&r); //reserved[3]
            readuint16_t(&r); //padding

            int nr=0;
            for(nr=0;nr<4;nr++) {
                int t, len;
                /* All of ttf is big-endian. All of ttf? No. One small eot table
                   of indomitable little-endian... */
                len = readuint8_t(&r);
                len |= readuint8_t(&r)<<8;
                len /= 2;
                for(t=0;t<len;t++) {
                    uint8_t c = readuint16_t(&r)>>8;
                }
                readuint16_t(&r); // zero terminator
            }
            readuint16_t(&r); // more padding

            /* adjust the offset to the start of the actual truetype
               data- the positions in the table header will be relative
               to the ttf data after the header, not to the file */
            r.mem += r.pos;
            r.size -= r.pos;
            r.pos = 0;
            ttf->version = readuint32_t(&r);
        } else {
            reader_reset(&r);
            ttf->version = readuint32_t(&r);
        }
    }

    if (ttf->version == TTCFTAG) {
        /* a ttc collection is a number of truetype fonts
           packaged together */
        if (length<16) {
            msg("<error> Truncated TTC file (%d bytes)", length);
            return 0;
        }
        uint32_t ttcf_version = readuint32_t(&r); // 0x00000100: v1.0, 0x00000200: v2.0, includes DSIG table
        uint32_t num_fonts = readuint32_t(&r); // number of fonts
        uint32_t font1_position = readuint32_t(&r);
        if (font1_position+12 > length) {\
            msg("<error> Truncated TTC file (%d bytes, first font at %d)", length, font1_position);
            return 0;
        }
        r.pos = font1_position;
        ttf->version = readuint32_t(&r);
    }

    int num_tables = readuint16_t(&r);

    readuint16_t(&r); //search range
    readuint16_t(&r); //entry selector
    readuint16_t(&r); //range shift

    if (num_tables*16 > length) {
        msg("<error> Truncated TTF file (table entries: %d)", num_tables);
        if (ttf->version != OPENTYPE &&
           ttf->version != TRUETYPE_MACOS &&
           ttf->version != VERSION_1_0) {
            // bad table length, weird version. This is probably not a ttf file.
            return 0;
        }
    }

    uint32_t*table_data = malloc(16*num_tables);
    int t;
    for(t=0;t<num_tables*4;t++) {
        table_data[t] = readuint32_t(&r);
    }
    for(t=0;t<num_tables;t++) {
        uint32_t tag = table_data[t*4];
        uint32_t checksum = table_data[t*4+1];
        uint32_t pos = table_data[t*4+2];
        uint32_t len = table_data[t*4+3];
        
        if (pos+len > length) {
            msg("<error> TTF Table %02x%02x%02x%02x outside of stream (pos %d)", (tag>>24)&0xff, (tag>>16)&0xff, (tag>>8)&0xff, (tag)&0xff, pos);
        } else {
            uint8_t*mem = malloc(len);
            r.pos = pos;
            readBlock(&r, mem, len);

            ttf_table_t*table = ttf_addtable(ttf, tag);
            table->data = mem;
            table->len = table->memsize = len;
#if 0
            uint32_t checksum2 = ttf_table_checksum(table);
            if (checksum2!=checksum) {
                msg("<warning> Checksum mismatch in tag %02x%02x%02x%02x %c%c%c%c (%d bytes) %08x!=%08x",
                        (tag>>24)&0xff, (tag>>16)&0xff, (tag>>8)&0xff, (tag)&0xff,
                        (tag>>24)&0xff, (tag>>16)&0xff, (tag>>8)&0xff, (tag)&0xff,
                        len, checksum2, checksum);
            }
#endif
        }
    }
    free(table_data);

    if (!ttf_parse_tables(ttf))
        return 0;

    return ttf;
}
void ttf_create_truetype_tables(ttf_t*ttf)
{
    if (!ttf->head)
        ttf->head = head_new(ttf);
    if (!ttf->maxp)
        ttf->maxp = maxp_new(ttf);
    if (!ttf->hea)
        ttf->hea = hea_new(ttf);
    if (!ttf->os2)
        ttf->os2 = os2_new(ttf);
    if (!ttf->post)
        ttf->post = post_new(ttf);
    if (!ttf->gasp)
        ttf->gasp = gasp_new(ttf);
    if (!ttf->prep)
        ttf->prep = prep_new(ttf);
}

ttf_table_t* ttf_write(ttf_t*ttf, uint32_t*checksum_adjust)
{
    ttf_collapse_tables(ttf);

    ttf_table_t*file = ttf_table_new(0);
    writeuint32_t(file, VERSION_1_0);

    /* write number of tables */
    int num_tables=0;
    ttf_table_t*t = ttf->tables;
    while (t) {
        num_tables++;
        t = t->next;
    }
    writeuint16_t(file, num_tables);

    /* write search range */
    int tmp = num_tables;
    int search_range = 0;
    while (tmp) {
        search_range = tmp;
        tmp = tmp&(tmp-1);
    }
    tmp = search_range;
    search_range*=16;
    writeuint16_t(file, search_range);

    /* write entry selector */
    int entry_selector = 0;
    while (tmp>1) {
        tmp>>=1;
        entry_selector++;
    }
    writeuint16_t(file, entry_selector);

    /* write range shift */
    int range_shift = num_tables*16 - search_range;
    writeuint16_t(file, range_shift);

    /* write table dictionary */
    int table_dictionary_pos = file->len;
    int data_pos = file->len + num_tables*16;
    for(t=ttf->tables;t;t=t->next) {
        writeuint32_t(file, t->id);
        writeuint32_t(file, ttf_table_checksum(t));
        writeuint32_t(file, data_pos);
        writeuint32_t(file, t->len);
        data_pos += t->len;
        data_pos += (-t->len)&3; //pad
    }

    /* write tables */
    int head_pos = 0;
    uint8_t zero[4]={0,0,0,0};
    for(t=ttf->tables;t;t=t->next) {
        if (t->id == TAG_HEAD)
            head_pos = file->len;
        writeBlock(file, t->data, t->len);
        writeBlock(file, zero, (-t->len)&3); //pad
    }
    uint32_t checksum = 0xb1b0afba - ttf_table_checksum(file);
    if (checksum_adjust)
        *checksum_adjust = checksum;
    uint8_t*checksum2 = file->data + head_pos + 8;
    checksum2[0] = checksum>>24;
    checksum2[1] = checksum>>16;
    checksum2[2] = checksum>>8;
    checksum2[3] = checksum>>0;
    return file;
}

ttf_table_t* ttf_eot_head(ttf_t*ttf)
{
    ttf_table_t*file = ttf_table_new(0);
    writeuint32_t(file, 0); //file size (filled in later)
    writeuint32_t(file, 0); //fontdatasize (filled in later)
    writeuint32_t(file, 0x01000200);
    writeuint32_t(file, 0); //flags
    writeuint8_t(file, ttf->os2->panose_FamilyType);
    writeuint8_t(file, ttf->os2->panose_SerifStyle);
    writeuint8_t(file, ttf->os2->panose_Weight);
    writeuint8_t(file, ttf->os2->panose_Proportion);
    writeuint8_t(file, ttf->os2->panose_Contrast);
    writeuint8_t(file, ttf->os2->panose_StrokeVariation);
    writeuint8_t(file, ttf->os2->panose_ArmStyle);
    writeuint8_t(file, ttf->os2->panose_Letterform);
    writeuint8_t(file, ttf->os2->panose_Midline);
    writeuint8_t(file, ttf->os2->panose_XHeight);
    writeuint8_t(file, 1); //charset (default)
    writeuint8_t(file, ttf->os2->fsSelection&1); //italic
    writeuint32_t_LE(file, ttf->os2->usWeightClass);
    writeuint16_t(file, 0); //fstype
    writeuint16_t(file, 0x4c50); //magic
    writeuint32_t_LE(file, ttf->os2->ulCharRange[0]);
    writeuint32_t_LE(file, ttf->os2->ulCharRange[1]);
    writeuint32_t_LE(file, ttf->os2->ulCharRange[2]);
    writeuint32_t_LE(file, ttf->os2->ulCharRange[3]);
    writeuint32_t_LE(file, ttf->os2->ulCodePageRange1);
    writeuint32_t_LE(file, ttf->os2->ulCodePageRange2);
    writeuint32_t(file, 0); //checksum adjust (filled in later)
    writeuint32_t(file, 0); //reserved[0]
    writeuint32_t(file, 0); //reserved[1]
    writeuint32_t(file, 0); //reserved[2]
    writeuint32_t(file, 0); //reserved[3]
    writeuint16_t(file, 0); //padding(1)

    int i,t,len;

    char* strings[] = {ttf->family_name, ttf->subfamily_name, ttf->version_string, ttf->full_name};
    int nr = sizeof(strings)/sizeof(strings[0]);

    for(i=0;i<nr;i++) {
        char *string = strings[i];

        //family name
        len = strlen(string);
        writeuint16_t_LE(file, len*2);
        for(t=0;t<len;t++) {
            writeuint8_t(file, 0);
            writeuint8_t(file, string[t]);
        }
        writeuint16_t(file, 0); //zero byte pad
    }

    writeuint16_t(file, 0); //zero byte pad

    writeuint16_t(file, 0); //padding(2)
    return file;
}

void ttf_save_eot(ttf_t*ttf, const char*filename)
{
    ttf_table_t* eot = ttf_eot_head(ttf);
    uint32_t checksum_adjust = 0;
    ttf_table_t* t = ttf_write(ttf, &checksum_adjust);

    uint8_t*len_data = eot->data;
    uint32_t full_len = eot->len + t->len;
    len_data[0] = full_len>>0;
    len_data[1] = full_len>>8;
    len_data[2] = full_len>>16;
    len_data[3] = full_len>>24;

    uint8_t*len_data2 = eot->data+4;
    len_data2[0] = t->len>>0;
    len_data2[1] = t->len>>8;
    len_data2[2] = t->len>>16;
    len_data2[3] = t->len>>24;

    uint8_t*checksum_data = eot->data + 60;
    checksum_data[0] = checksum_adjust>>0;
    checksum_data[1] = checksum_adjust>>8;
    checksum_data[2] = checksum_adjust>>16;
    checksum_data[3] = checksum_adjust>>24;

    FILE*fi = fopen(filename, "wb");
    if (!fi) {
        perror(filename);
        return;
    }

    fwrite(eot->data, eot->len, 1, fi);
    fwrite(t->data, t->len, 1, fi);
    fclose(fi);
    ttf_table_delete(0, t);
    ttf_table_delete(0, eot);
}

void ttf_save(ttf_t*ttf, const char*filename)
{
    ttf_table_t* t = ttf_write(ttf, 0);
    FILE*fi = fopen(filename, "wb");
    if (!fi) {
        perror(filename);
        return;
    }
    fwrite(t->data, t->len, 1, fi);
    fclose(fi);
    ttf_table_delete(0, t);
}

void ttf_dump(ttf_t*ttf)
{
    msg("<notice> Truetype file version %08x%s", ttf->version, ttf->version == OPENTYPE?" (opentype)":"");
    ttf_table_t*table = ttf->tables;
    while (table) {
        uint32_t tag = table->id;
        msg("<notice> Tag %02x%02x%02x%02x [%c%c%c%c] (length: %d)",
                (tag>>24)&0xff, (tag>>16)&0xff, (tag>>8)&0xff, (tag)&0xff,
                (tag>>24)&0xff, (tag>>16)&0xff, (tag>>8)&0xff, (tag)&0xff, table->len);
        table = table->next;
    }
    //ttf_table_dump(ttf_find_table(ttf, TAG_MAXP));

    head_dump(ttf);
    hea_dump(ttf);
    os2_dump(ttf);
    maxp_dump(ttf);
    glyf_dump(ttf);
}
void ttf_destroy_tables(ttf_t*ttf)
{
    ttf_table_t*table = ttf->tables;
    while (table) {
        ttf_table_t*next = table->next;
        free(table->data);
        free(table);
        table = next;
    }
    ttf->tables = 0;
}
void ttf_reduce(ttf_t*ttf)
{
    ttf_destroy_tables(ttf);
}
void ttf_destroy(ttf_t*ttf)
{
    ttf_destroy_tables(ttf);
    maxp_delete(ttf);
    os2_delete(ttf);
    head_delete(ttf);
    hea_delete(ttf);
    glyf_delete(ttf);
    post_delete(ttf);
    cvt_delete(ttf);
    name_delete(ttf);
    free(ttf);
}

typedef struct _memfile {
    void*data;
    int len;
} memfile_t;

memfile_t* memfile_open(const char*path)
{
    memfile_t*file = malloc(sizeof(memfile_t));
#if defined(HAVE_MMAP) && defined(HAVE_STAT)
    int fi = open(path, O_RDONLY);
    if (fi<0) {
        perror(path);
        free(file);
        return 0;
    }
    struct stat sb;
    if (fstat(fi, &sb)<0) {
        perror(path);
        return 0;
    }
    file->len = sb.st_size;
    file->data = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fi, 0);
    close(fi);
#else
    FILE*fi = fopen(path, "rb");
    if (!fi) {
        perror(path);
        free(file);
        return 0;
    }
    fseek(fi, 0, SEEK_END);
    file->len = ftell(fi);
    fseek(fi, 0, SEEK_SET);
    file->data = malloc(file->len);
    if (!file->data) {
        msg("<error> Out of memory while allocating memory for file %s\n", path);
        free(file);
        return 0;
    }
    fread(file->data, file->len, 1, fi);
    fclose(fi);
#endif
    return file;
}

void memfile_close(memfile_t*file)
{
#if defined(HAVE_MMAP) && defined(HAVE_STAT)
    munmap(file->data, file->len);
#else
    free(file->data);
#endif
    file->data = 0;
    file->len = 0;
    free(file);
}

ttf_t* ttf_open(const char*filename)
{
    memfile_t*m = memfile_open(filename);
    ttf_t*ttf = ttf_load(m->data, m->len);
    memfile_close(m);
    return ttf;
}

#ifdef MAIN
int main(int argn, const char*argv[])
{
    setConsoleLogging(7);
    const char*filename = "comic.ttf";
    if (argn>1)
        filename = argv[1];
    //msg("<notice> Loading %s", filename);
    memfile_t*m = memfile_open(filename);
    ttf_t*ttf = ttf_load(m->data, m->len);

    if (!ttf) {
        msg("<error> Couldn't load %s", filename);
        return 1;
    }
    ttf_reduce(ttf);

    ttf_create_truetype_tables(ttf);

    if (!ttf) return 1;
    memfile_close(m);
    //ttf_dump(ttf);
    //printf("os2 version: %04x (%d), maxp size: %d\n",
//          ttf->os2->version, ttf->os2->size, ttf->maxp->size);
    ttf_save_eot(ttf, "testfont.eot");
    ttf_save(ttf, "testfont.ttf");
    ttf_destroy(ttf);
    return 0;

}
#endif
