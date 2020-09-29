#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include "gfxpoly.h"
#include "../src/render.h"
#include <dirent.h>

char* allocprintf(const char*format, ...)
{
    va_list arglist1;
    va_start(arglist1, format);
    char dummy;
    int l = vsnprintf(&dummy, 1, format, arglist1);
    va_end(arglist1);

    va_list arglist2;
    va_start(arglist2, format);
    char*buf = malloc(l+1);
    vsnprintf(buf, l+1, format, arglist2);
    va_end(arglist2);
    return buf;
}

void rotate90(gfxpoly_t*poly)
{
    int i,j;
    gfxsegmentlist_t*stroke = poly->strokes;
    for(;stroke;stroke=stroke->next) {
        for(j=0;j<stroke->num_points;j++) {
            point_t a = stroke->points[j];
            stroke->points[j].x = a.y;
            stroke->points[j].y = a.x;
        }
    }
}

int main(int argn, char*argv[])
{
    if (argn<=1) {
        printf("Usage:\n\trun_ps <dir>\n");
        exit(0);
    }
    char*dir = argv[1];
    DIR*_dir = opendir(dir);
    if (!_dir) return;
    struct dirent*file;
    while (1) {
        file = readdir(_dir);
        if (!file)
            break;
        if (!strstr(file->d_name, ".ps"))
            continue;

        char* filename = allocprintf("%s/%s", dir, file->d_name);

        windrule_t*rule = &windrule_evenodd;
        gfxpoly_t*poly1 = gfxpoly_from_file(filename);

        double zoom = 1.0;

        if (!gfxpoly_check(poly1, 0)) {
            printf("bad polygon %s\n", filename);
            free(filename);
            continue;
        }
        free(filename);

        gfxpoly_t*poly2 = gfxpoly_process(poly1, 0, rule, &onepolygon, 0);
        assert(gfxpoly_check(poly2, 1));

        int pass;
        for(pass=0;pass<2;pass++) {
            intbbox_t bbox = intbbox_from_polygon(poly1, zoom);
            unsigned char*bitmap1 = render_polygon(poly1, &bbox, zoom, rule, &onepolygon);
            unsigned char*bitmap2 = render_polygon(poly2, &bbox, zoom, &windrule_circular, &onepolygon);
            if (!bitmap_ok(&bbox, bitmap1) || !bitmap_ok(&bbox, bitmap2)) {
                save_two_bitmaps(&bbox, bitmap1, bitmap2, "error.png");
                assert(!"error in bitmaps");
            }
            if (!compare_bitmaps(&bbox, bitmap1, bitmap2)) {
                save_two_bitmaps(&bbox, bitmap1, bitmap2, "error.png");
                assert(!"bitmaps don't match");
            }
            free(bitmap1);
            free(bitmap2);

            // second pass renders the 90° rotated version
            rotate90(poly1);
            rotate90(poly2);
        }

        gfxpoly_destroy(poly1);
        gfxpoly_destroy(poly2);
    }
    closedir(_dir);
}

