/* logo.c

logo

Copyright (c) 2012 Matthias Kramm <kramm@quiss.org>
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
#include <stdio.h>
#include <stdlib.h>
#include "gfxpoly.h"
#include "ttf.h"

//#define FONT "/usr/lib/win32/fonts/georgia.ttf"
#define FONT "/usr/lib/win32/fonts/ariblk.ttf"

static void save(gfxpoly_t*poly, double z, const char*filename)
{
    FILE*fi = fopen(filename, "wb");
    fprintf(fi, "%% begin\n");
    int s,t;
    gfxsegmentlist_t*stroke = poly->strokes;
    for(;stroke;stroke=stroke->next) {
            fprintf(fi, "%g setgray\n", stroke->dir==DIR_UP ? 0.7 : 0);
        gridpoint_t p = stroke->points[0];
        fprintf(fi, "%f %f moveto\n", p.x * z, p.y * z);
        for(s=1;s<stroke->num_points;s++) {
            p = stroke->points[s];
            fprintf(fi, "%f %f lineto\n", p.x * z, p.y * z);
        }
        fprintf(fi, "stroke\n");
    }
    fprintf(fi, "showpage\n");
    fclose(fi);
}

#include <pdflib.h>
static void draw_on_pdf(gfxpoly_t*raw, const char*filename, double scale)
{
    gfxbbox_t bbox = gfxpoly_calculate_bbox(raw);
    bbox.x1 *= scale;
    bbox.y1 *= scale;
    bbox.x2 *= scale;
    bbox.y2 *= scale;
    double tx = -bbox.x1 + 5;
    double ty = -bbox.y1 + 5;
    int width = bbox.x2 - bbox.x1 + 10;
    int height = bbox.y2 - bbox.y1 + 10;

    gfxpoly_t*evenodd = gfxpoly_process(raw, NULL, &windrule_evenodd, &onepolygon, NULL);
    gfxpoly_t*circular = gfxpoly_process(raw, NULL, &windrule_circular, &onepolygon, NULL);

    PDF*pdf = PDF_new();
    PDF_open_file(pdf, filename);
    PDF_set_parameter(pdf, "usercoordinates", "true");
    PDF_set_parameter(pdf, "topdown", "false");

    double z = raw->gridsize * scale;

    PDF_begin_page(pdf, width, height);

    double fx,fy;
    PDF_setlinecap(pdf, /*round*/1);
    PDF_setlinejoin(pdf, /*round*/1);
    PDF_setrgbcolor_stroke(pdf, 0.7,0.7,0.7);
    PDF_setlinewidth(pdf, 2.0);
    for(fy=0;fy<=height;fy+=z) {
        for(fx=0;fx<=width;fx+=z) {
	    PDF_moveto(pdf, fx, fy);
	    PDF_lineto(pdf, fx, fy);
	    PDF_stroke(pdf);
        }
    }

    gfxline_t* filled1 = gfxline_from_gfxpoly_with_direction(circular);
    gfxline_t* filled2 = gfxline_from_gfxpoly_with_direction(evenodd);

    gfxline_t*l;

    PDF_setrgbcolor_fill(pdf, 0xe8/255.0,0xec/255.0,0xff/255.0);
    for(l=filled1;l;l=l->next) {
        if(l->type == gfx_moveTo) {
	    PDF_moveto(pdf, tx+l->x*scale, ty+l->y*scale);
        } else if(l->type == gfx_lineTo) {
	    PDF_lineto(pdf, tx+l->x*scale, ty+l->y*scale);
        }
    }
    PDF_fill(pdf);

    PDF_setrgbcolor_fill(pdf, 0xd0/255.0,0xd8/255.0,0xff/255.0);
    for(l=filled2;l;l=l->next) {
        if(l->type == gfx_moveTo) {
	    PDF_moveto(pdf, tx+l->x*scale, ty+l->y*scale);
        } else if(l->type == gfx_lineTo) {
	    PDF_lineto(pdf, tx+l->x*scale, ty+l->y*scale);
        }
    }
    PDF_fill(pdf);

    gfxsegmentlist_t*stroke;
    PDF_setrgbcolor_stroke(pdf, 0.5,0.5,0.5);
    PDF_setlinewidth(pdf, 1.0);
    for(stroke=evenodd->strokes;stroke;stroke=stroke->next) {
        gridpoint_t p = stroke->points[0];
	PDF_moveto(pdf, tx+p.x*z, ty+p.y*z);
        int s;
        for(s=1;s<stroke->num_points;s++) {
            p = stroke->points[s];
	    PDF_lineto(pdf, tx+p.x*z, ty+p.y*z);
        }
	PDF_stroke(pdf);
    }

    PDF_setlinecap(pdf, /*round*/1);
    PDF_setrgbcolor_stroke(pdf, 0.0,0.0,0.0);
    PDF_setlinewidth(pdf, 3.0);
    for(stroke=evenodd->strokes;stroke;stroke=stroke->next) {
        gridpoint_t p = stroke->points[0];
        int s;
        for(s=0;s<stroke->num_points;s++) {
            p = stroke->points[s];
	    PDF_moveto(pdf, tx+p.x*z, ty+p.y*z);
	    PDF_lineto(pdf, tx+p.x*z, ty+p.y*z);
	    PDF_stroke(pdf);
        }
    }

    PDF_end_page(pdf);
    PDF_close(pdf);
    PDF_delete(pdf);
}

int main(int argn, char*argv[])
{
    double grid = 2;

    gfxcanvas_t*canvas = gfxcanvas_new(grid);

    char*str = "gfxpoly";
    int length = strlen(str);
    ttf_t*ttf = ttf_open(FONT);
    int i;
    double s = 20;
    double scale_x = 0.003 * s;
    double scale_y = 0.001 * s;

    float x_pos[] = {0,0.2,0,0,0,0,0};
    float y_pos[] = {10,10.7,9.3,10.3,9.4,10.3,10.5};

    float pos_x = 0;
    for(i=0;i<length;i++) {
        float offset_x = x_pos[i]*scale_x*300 + pos_x;
        float offset_y = y_pos[i]*scale_y*300;
        ttfglyph_t*glyph = ttf_find_unicode(ttf, str[i]);
        if(!glyph)
            continue;
        int j;
        for(j=0;j<glyph->num_points;j++) {
            ttfpoint_t*p = &glyph->points[j];
            ttfpoint_t*next = j < glyph->num_points-1 ? &glyph->points[j+1] : NULL;
            if(p->flags&GLYPH_CONTOUR_START) {
                canvas->moveTo(canvas, offset_x + p->x * scale_x, offset_y + p->y * scale_y);
            } /*else if(!(p->flags&GLYPH_ON_CURVE) && next &&
                       (next->flags&GLYPH_ON_CURVE)) {
                canvas->splineTo(canvas, offset_x + p->x * scale_x,
                                         offset_y + p->y * scale_y,
                                         offset_x + next->x * scale_x,
                                         offset_y + next->y * scale_y);
                j++;
                p = next;
            }*/ else {
                canvas->lineTo(canvas, offset_x + p->x * scale_x, offset_y + p->y * scale_y);
            }
            if(p->flags&GLYPH_CONTOUR_END) {
                canvas->close(canvas);
            }
        }
        pos_x += glyph->advance * 3 / 5 * scale_x;
    }

    gfxpoly_t*poly = (gfxpoly_t*)canvas->result(canvas);

    draw_on_pdf(poly, "logo.pdf", 50.0/s);

    system("pdftoppm -r 72 logo.pdf logo");
    system("convert logo-000001.ppm doc/logo.png");
    system("rm -f logo.pdf logo-000001.ppm");
    system("convert doc/logo.png -crop 5x5+10+10 doc/background.png");

    gfxpoly_destroy(poly);
    return 0;
}
