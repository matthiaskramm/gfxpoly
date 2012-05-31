/* logo.c

   gfxpoly example: the gfxpoly logo

   Copyright (c) 2012 Matthias Kramm <kramm@quiss.org>

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

#include <string.h>
#include <stdio.h>
#include "gfxpoly.h"
#include "ttf.h"

#define FONT "/usr/lib/win32/fonts/georgia.ttf"

static void save(gfxpoly_t*poly, double z, const char*filename)
{
    FILE*fi = fopen(filename, "wb");
    fprintf(fi, "%% begin\n");
    int s,t;
    gfxsegmentlist_t*stroke = poly->strokes;
    for(;stroke;stroke=stroke->next) {
            fprintf(fi, "%g setgray\n", stroke->dir==DIR_UP ? 0.7 : 0);
        point_t p = stroke->points[0];
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

int main(int argn, char*argv[])
{
    double grid = 2;

    gfxcanvas_t*canvas = gfxcanvas_new(grid);

    char*str = "gfxpoly";
    int length = strlen(str);
    ttf_t*ttf = ttf_open(FONT);
    int i;
    float offset_x = 0;
    float offset_y = 10;
    double s = 30;
    double scale_x = 0.003 * s;
    double scale_y = 0.003 * s;

    float y_pos[] = {10,10.9,9.3,10.3,9.4,10.3,10.5};

    for(i=0;i<length;i++) {
        offset_y = y_pos[i]*s;
        ttfglyph_t*glyph = ttf_find_unicode(ttf, str[i]);
        if(!glyph)
            continue;
        int j;
        for(j=0;j<glyph->num_points;j++) {
            ttfpoint_t*p = &glyph->points[j];
            if(p->flags&GLYPH_CONTOUR_START) {
                canvas->moveTo(canvas, offset_x + p->x * scale_x, offset_y + p->y * scale_y);
            } else {
                canvas->lineTo(canvas, offset_x + p->x * scale_x, offset_y + p->y * scale_y);
            }
            if(p->flags&GLYPH_CONTOUR_END) {
                canvas->close(canvas);
            }
        }
        offset_x += glyph->advance * 3 / 5 * scale_x;
    }

    gfxpoly_t*poly = (gfxpoly_t*)canvas->result(canvas);
    save(poly, grid, "test.ps");

    gfxpoly_destroy(poly);
}
