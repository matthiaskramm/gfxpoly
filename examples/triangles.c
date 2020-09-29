#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "gfxpoly.h"

char colors[]={
    0,0,0,  255,255,255, 255,0,0, 0,255,255, 255,0,255, 0,255,255,
    0,0,255, 255,255,0,   128,128,0, 64,64,0, 255,128,128, 64,64,64,
    96, 96, 96, 128,255,128, 128,128,255, 128,128,128,
    32,0,0,32,0,0
};

typedef struct _xy
{
    double x,y;
} xy_t;

typedef struct _triangle
{
    xy_t p1,p2,p3;
} triangle_t;

static triangle_t triangle_turn(triangle_t orig, int x, int y, double ua, double z)
{
    triangle_t tri;
    tri.p1.x = (orig.p1.x*cos(ua)-orig.p1.y*sin(ua))*z+x;
    tri.p1.y = (orig.p1.x*sin(ua)+orig.p1.y*cos(ua))*z+y;
    tri.p2.x = (orig.p2.x*cos(ua)-orig.p2.y*sin(ua))*z+x;
    tri.p2.y = (orig.p2.x*sin(ua)+orig.p2.y*cos(ua))*z+y;
    tri.p3.x = (orig.p3.x*cos(ua)-orig.p3.y*sin(ua))*z+x;
    tri.p3.y = (orig.p3.x*sin(ua)+orig.p3.y*cos(ua))*z+y;
    return tri;
}

static triangle_t t1 = {{0,-100},{-100,100},{10,100}};
static triangle_t t2 = {{-100,-100},{-100,100},{100,100}};
static triangle_t t3 = {{100,-100},{-100,-100},{100,100}};

static void add(gfxcanvas_t*canvas, triangle_t*t)
{
    canvas->setUserData(canvas, t);
    canvas->moveTo(canvas, t->p1.x, t->p1.y);
    canvas->lineTo(canvas, t->p2.x, t->p2.y);
    canvas->lineTo(canvas, t->p3.x, t->p3.y);
    canvas->close(canvas);
}

int main(int argn, char*argv[])
{
    xy_t rpix[16384];
    int i;
    for(i=0;i<16384;i++) {
        rpix[i].x=lrand48()%640-320,
        rpix[i].y=lrand48()%480-240;
    }

    double ua=0,ub=0,uc=0;


    while (1) {
        gfxcanvas_t*canvas = gfxcanvas_new(0.05);

        triangle_t b1 = triangle_turn(t2, -32, -24, ua, 1);
        add(canvas, &b1);
        triangle_t b2 = triangle_turn(t3, -32, -24, ua, 1);
        add(canvas, &b2);

        triangle_t turned[16384];
        for(i=0;i<128;i++) {
            turned[i] = triangle_turn(t1, rpix[i].x, rpix[i].y, ua+0.8*i, (60-i)/32.0);
            add(canvas, &turned[i]);
        }
        gfxpoly_t* poly = (gfxpoly_t*)canvas->result(canvas);

        gfxpoly_t*p2 = gfxpoly_selfintersect_evenodd(poly);

        ua+=0.03;
        ub+=0.04568;
        uc+=0.05693;
    }

    return 0;
}

