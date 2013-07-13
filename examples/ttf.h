/* ttf.h

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

#ifndef __ttf_h__
#define __ttf_h__

#include <stdint.h>

typedef struct _ttf_table {
    uint32_t id;
    struct _ttf_table*prev;
    struct _ttf_table*next;

    uint8_t*data;
    int len;
    int memsize;
} ttf_table_t;

typedef struct _table_maxp {
    uint16_t maxPoints;
    uint16_t maxContours;
    uint16_t maxComponentPoints;
    uint16_t maxComponentContours;
    uint16_t maxZones;
    uint16_t maxTwilightPoints;
    uint16_t maxStorage;
    uint16_t maxFunctionDefs;
    uint16_t maxInstructionDefs;
    uint16_t maxStackElements;
    uint16_t maxSizeOfInstructions;
    uint16_t maxComponentElements;
    uint16_t maxComponentDepth;
} table_maxp_t;

typedef struct _table_os2 {
    int16_t xAvgCharWidth;
    uint16_t usWeightClass;
    uint16_t usWidthClass;
    uint16_t ySubscriptXSize;
    uint16_t ySubscriptYSize;
    uint16_t ySubscriptXOffset;
    uint16_t ySubscriptYOffset;
    uint16_t ySuperscriptXSize;
    uint16_t ySuperscriptYSize;
    uint16_t ySuperscriptXOffset;
    uint16_t ySuperscriptYOffset;
    uint16_t yStrikeoutSize;
    uint16_t yStrikeoutPosition;
    uint16_t sFamilyClass;
    uint8_t panose_FamilyType;
    uint8_t panose_SerifStyle;
    uint8_t panose_Weight;
    uint8_t panose_Proportion;
    uint8_t panose_Contrast;
    uint8_t panose_StrokeVariation;
    uint8_t panose_ArmStyle;
    uint8_t panose_Letterform;
    uint8_t panose_Midline;
    uint8_t panose_XHeight;
    uint32_t ulCharRange[4];

    uint16_t fsSelection;
    uint16_t fsFirstCharIndex;
    uint16_t fsLastCharIndex;

    int16_t sTypoAscender;
    int16_t sTypoDescender;
    int16_t sTypoLineGap;
    uint16_t usWinAscent;
    uint16_t usWinDescent;

    /* for version >= 0x0001 */
    uint32_t ulCodePageRange1;
    uint32_t ulCodePageRange2;

    /* for version >= 0x0002 */
    int16_t sxHeight;
    int16_t sCapHeight;
    uint16_t usDefaultChar;
    uint16_t usBreakChar;
    uint16_t usMaxContext;
} table_os2_t;

typedef struct _table_hea
{
    uint16_t advanceWidthMax;
    int16_t minLeftSideBearing;
    int16_t minRightSideBearing;
    int16_t xMaxExtent;
    int16_t caretSlopeRise;
    int16_t caretSlopeRun;
    int16_t caretOffset;
} table_hea_t;

#define GLYPH_ON_CURVE 0x01
#define GLYPH_CONTOUR_START 0x40
#define GLYPH_CONTOUR_END 0x80

typedef uint32_t unicode_t;

typedef struct _ttfpoint {
    int x,y;
    uint8_t flags;
} ttfpoint_t;

typedef struct _ttfglyph {
    uint16_t advance;
    int16_t bearing;
    int16_t xmin,ymin,xmax,ymax;
    int code_size;
    uint8_t*code;
    int num_points;
    ttfpoint_t*points;
} ttfglyph_t;

typedef struct _table_head {
    uint16_t flags;
    uint16_t units_per_em;
    int16_t xmin,ymin,xmax,ymax;
    uint16_t macStyle;
    uint16_t lowest_readable_size;
    int16_t dir_hint;
} table_head_t;

typedef struct _table_post {
    uint32_t italic_angle;
    uint16_t underline_position;
    uint16_t underline_thickness;
} table_post_t;

typedef struct _table_cvt {
    int16_t*values;
    int num;
} table_cvt_t;

typedef struct _table_gasp {
    int num;
    struct {
        uint16_t size;
        uint16_t behaviour;
    } *records;
} table_gasp_t;

typedef struct _table_code {
    uint8_t*code;
    int size;
} table_code_t;

typedef struct _ttf {
    char*family_name;     /* nameId 1 */
    char*subfamily_name;  /* nameId 2 */
    char*font_uid;        /* nameId 3 */
    char*full_name;       /* nameId 4 */
    char*version_string;  /* nameId 5 */
    char*postscript_name; /* nameId 6 */

    ttf_table_t*tables;

    table_head_t*head;
    table_maxp_t*maxp;
    table_os2_t*os2;
    table_hea_t*hea;
    table_post_t*post;
    table_cvt_t*cvt;
    table_gasp_t*gasp;
    table_code_t*prep;
    table_code_t*fpgm;

    char is_vertical;

    int16_t ascent;
    int16_t descent; // ymin, *not* negative ymin
    int16_t lineGap;

    int num_glyphs;
    ttfglyph_t*glyphs;

    int unicode_size;
    unicode_t*unicode;

    uint32_t version;
} ttf_t;


ttf_t*ttf_new();
ttf_t* ttf_open(const char*filename);
void ttf_reduce(ttf_t*ttf);
ttf_t*ttf_load(void*data, int length);
ttf_table_t*ttf_addtable(ttf_t*ttf, uint32_t tag);
void ttf_create_truetype_tables(ttf_t*ttf);
void ttf_dump(ttf_t*ttf);
void ttf_destroy(ttf_t*ttf);
void ttf_save(ttf_t*ttf, const char*filename);
void ttf_save_eot(ttf_t*ttf, const char*filename);
ttfglyph_t* ttf_find_unicode(ttf_t*ttf, uint32_t unicode);

#endif
