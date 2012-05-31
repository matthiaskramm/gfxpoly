/* moments.c

   nth order moments of polygons

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

#include "gfxpoly.h"
#include "poly.h"
#include "wind.h"
#include "moments.h"

#define MOMENTS

void moments_update(moments_t*moments, actlist_t*actlist, int32_t y1, int32_t y2)
{
    segment_t* s = actlist_leftmost(actlist);
    segment_t* l = 0;

    /* the actual coordinate of grid points is at the bottom right, hence
       we have to add 1.0 to both coordinates (or just 1.0 to the sum) */
    double mid = (y1+y2)/2.0 + 1.0;

    double area = 0.0;

    while(s) {
        if(l && l->wind.is_filled) {
            area += (XPOS(s,mid) - XPOS(l,mid));

#ifdef MOMENTS
            double dx1 = (l->b.x - l->a.x) / (double)(l->b.y - l->a.y);
            double o1 = l->a.x - l->a.y*dx1;

            double dx2 = (s->b.x - s->a.x) / (double)(s->b.y - s->a.y);
            double o2 = s->b.x - s->b.y*dx2;

#ifdef DEBUG
            printf("y=%f-%f\n\tl1=([%f,%f,%f,%f] dx=%f o=%f)\n\tl2=([%f,%f,%f,%f] dx=%f o=%f)\n",
                    y1*0.05, y2*0.05,
                    l->a.x*0.05, l->a.y*0.05, l->b.x*0.05, l->b.y*0.05, dx1*0.05, o1*0.05,
                    s->a.x*0.05, s->a.y*0.05, s->b.x*0.05, s->b.y*0.05, dx2*0.05, o2*0.05);
#endif

#define S1(y) 0.5*(1/3.0*(dx2*dx2-dx1*dx1)*(y)*(y)*(y)+1/2.0*(2*dx2*o2-2*dx1*o1)*(y)*(y)+(o2*o2-o1*o1)*(y))
            double m1x = S1(y2)-S1(y1);
#define S2(y) (1/3.0)*(1/4.0*(dx2*dx2*dx2-dx1*dx1*dx1)*(y)*(y)*(y)*(y)+1/3.0*(3*dx2*dx2*o2-3*dx1*dx1*o1)*(y)*(y)*(y)+1/2.0*(3*dx2*o2*o2-3*dx1*o1*o1)*(y)*(y)+(o2*o2*o2-o1*o1*o1)*(y))
            double m2x = S2(y2)-S2(y1);
            moments->m[0][0] += (XPOS(s,mid) - XPOS(l,mid))*(y2-y1);
            moments->m[1][0] += m1x;
            moments->m[2][0] += m2x;
#endif
        }
        l = s;
        s = s->right;
    }

    area *= (y2-y1);

    moments->area += area;
}
void moments_normalize(moments_t*moments, double gridsize)
{
    moments->area *= gridsize*gridsize;
    moments->m[0][0] *= gridsize*gridsize;
    moments->m[1][0] *= gridsize*gridsize*gridsize*gridsize;
    moments->m[2][0] *= gridsize*gridsize*gridsize*gridsize*gridsize*gridsize;
}

double gfxpoly_area(gfxpoly_t*p)
{
    moments_t moments;
    gfxpoly_t*p2 = gfxpoly_process(p, 0, &windrule_evenodd, &onepolygon, &moments);
    gfxpoly_destroy(p2);
    moments_normalize(&moments, p->gridsize);
    return moments.area;
}
double gfxpoly_intersection_area(gfxpoly_t*p1, gfxpoly_t*p2)
{
    moments_t moments;
    gfxpoly_t*p3 = gfxpoly_process(p1, p2, &windrule_intersect, &twopolygons, &moments);
    gfxpoly_destroy(p3);

    moments_normalize(&moments, p1->gridsize);
    return moments.area;
}
moments_t gfxpoly_moments(gfxpoly_t*p)
{
    moments_t moments;
    gfxpoly_t*p2 = gfxpoly_process(p, 0, &windrule_evenodd, &onepolygon, &moments);
    gfxpoly_destroy(p2);
    moments_normalize(&moments, p->gridsize);
    return moments;
}
