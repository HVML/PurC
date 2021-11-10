/* Rectangular class for Midnight Commander widgets

   Copyright (C) 2020-2021
   The Free Software Foundation, Inc.

   Written by:
   Andrew Borodin <aborodin@vmail.ru>, 2020

   This file is part of the Midnight Commander.

   The Midnight Commander is free software: you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   The Midnight Commander is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file widget-common.c
 *  \brief Source: shared stuff of widgets
 */

#include <config.h>

#include <stdlib.h>

#include "lib/global.h"

#include "rect.h"

/*** global variables ****************************************************************************/

/*** file scope macro definitions ****************************************************************/

/*** file scope type declarations ****************************************************************/

/*** file scope variables ************************************************************************/

/*** file scope functions ************************************************************************/

/* --------------------------------------------------------------------------------------------- */
/*** public functions ****************************************************************************/
/* --------------------------------------------------------------------------------------------- */
/**
  * Create new WRect object.
  *
  * @param y y-coordinate of left-up corner
  * @param x x-coordinate of left-up corner
  * @param lines heigth
  * @param cols width
  *
  * @return newly allocated WRect object.
  */

WRect *
rect_new (int y, int x, int lines, int cols)
{
    WRect *r;

    r = g_try_new (WRect, 1);

    if (r != NULL)
        rect_init (r, y, x, lines, cols);

    return r;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Initialize WRect object.
  *
  * @param r WRect object
  * @param y y-coordinate of left-up corner
  * @param x x-coordinate of left-up corner
  * @param lines heigth
  * @param cols width
  */

void
rect_init (WRect * r, int y, int x, int lines, int cols)
{
    r->y = y;
    r->x = x;
    r->lines = lines;
    r->cols = cols;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Change position of rectangle area.
  *
  * @param r WRect object
  * @param dy y-shift of left-up corner
  * @param dx x-shift of left-up corner
  */

void
rect_move (WRect * r, int dy, int dx)
{
    r->y += dy;
    r->x += dx;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Change size of rectangle area.
  *
  * @param r WRect object
  * @param dl change size value of heigth
  * @param dc change size value of width
  */

void
rect_resize (WRect * r, int dl, int dc)
{
    r->lines += dl;
    r->cols += dc;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Calculates the intersection of two rectangle areas.
  * The resulting rectangle is the largest rectangle which contains intersection of rectangle areas.
  *
  * @param r first WRect object
  * @param r1 second WRect object
  *
  * The resulting rectangle is stored in r.
  */

void
rect_intersect (WRect * r, const WRect * r1)
{
    int y, x;
    int y1, x1;

    /* right-down corners */
    y = r->y + r->lines;
    x = r->x + r->cols;
    y1 = r1->y + r1->lines;
    x1 = r1->x + r1->cols;

    /* right-down corner of intersection */
    y = MIN (y, y1);
    x = MIN (x, x1);

    /* left-up corner of intersection */
    r->y = MAX (r->y, r1->y);
    r->x = MAX (r->x, r1->x);

    /* intersection sizes */
    r->lines = y - r->y;
    r->cols = x - r->x;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Calculates the union of two rectangle areas.
  * The resulting rectangle is the largest rectangle which contains both rectangle areas.
  *
  * @param r first WRect object
  * @param r1 second WRect object
  *
  * The resulting rectangle is stored in r.
  */

void
rect_union (WRect * r, const WRect * r1)
{
    int x, y;
    int x1, y1;

    /* right-down corners */
    y = r->y + r->lines;
    x = r->x + r->cols;
    y1 = r1->y + r1->lines;
    x1 = r1->x + r1->cols;

    /* right-down corner of union */
    y = MAX (y, y1);
    x = MAX (x, x1);

    /* left-up corner of union */
    r->y = MIN (r->y, r1->y);
    r->x = MIN (r->x, r1->x);

    /* union sizes */
    r->lines = y - r->y;
    r->cols = x - r->x;
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Check whether two rectangle areas are overlapped or not.
  *
  * @param r1 WRect object
  * @param r2 WRect object
  *
  * @return TRUE if rectangle areas are overlapped, FALSE otherwise.
  */

gboolean
rects_are_overlapped (const WRect * r1, const WRect * r2)
{
    return !((r2->x >= r1->x + r1->cols) || (r1->x >= r2->x + r2->cols)
             || (r2->y >= r1->y + r1->lines) || (r1->y >= r2->y + r2->lines));
}

/* --------------------------------------------------------------------------------------------- */
/**
  * Check whether two rectangle areas are equal or not.
  *
  * @param r1 WRect object
  * @param r2 WRect object
  *
  * @return TRUE if rectangle areas are equal, FALSE otherwise.
  */

gboolean
rects_are_equal (const WRect * r1, const WRect * r2)
{
    return (r1->y == r2->y && r1->x == r2->x && r1->lines == r2->lines && r1->cols == r2->cols);
}

/* --------------------------------------------------------------------------------------------- */
