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

#include "convert.h"

int main(int argn, char*argv[])
{
    gfxdrawer_t draw;
    gfxdrawer_target_poly(&draw, 0.05);

    gfxpoly_t*poly = (gfxpoly_t*)polydraw_result(&draw);
    gfxpoly_destroy(poly);
}
