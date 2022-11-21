/*
** @file region.c
** @author Vincent Wei
** @date 2022/11/21
** @brief Implemetation of region.
**  Note that we copied most of code from GPL'd MiniGUI:
**
**      <https://github.com/VincentWei/MiniGUI/>
**
** Copyright (C) 2002~2022, Beijing FMSoft Technologies Co., Ltd.
** Copyright (C) 1998~2002, WEI Yongming
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
** Shamelessly ripped out from the X11 distribution
** Thanks for the nice licence.
**
** Copyright 1993, 1994, 1995 Alexandre Julliard
** Modifications and additions: Copyright 1998 Huw Davies
*/

/************************************************************************

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIfoil_rect OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/
/*
 * The functions in this file implement the Region abstraction, similar to one
 * used in the X11 sample server. A Region is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each Region is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (y1) will have the same lower-right y
 * coordinate (y2) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "region.h"
#include "rect.h"

typedef void (*voidProcp1)(foil_region *region,
        const foil_rgnrc *r1, const foil_rgnrc *r1End,
        const foil_rgnrc *r2, const foil_rgnrc *r2End, int top, int bottom);
typedef void (*voidProcp2)(foil_region *region,
        const foil_rgnrc *r, const foil_rgnrc *rEnd, int top, int bottom);

/*  1 if two foil_rects overlap.
 *  0 if two foil_rects do not overlap.
 */
#define EXTENTCHECK(r1, r2) \
        ((r1)->right > (r2)->left && \
         (r1)->left < (r2)->right && \
         (r1)->bottom > (r2)->top && \
         (r1)->top < (r2)->bottom)

/*
 * Allocate a new clipping rect and add it to the region.
 */
#define NEWfoil_rgnrc(region, rect) \
       {\
            rect = foil_region_rect_alloc(region->heap);\
            rect->next = NULL;\
            rect->prev = region->tail;\
            if (region->tail)\
                region->tail->next = rect;\
            region->tail = rect;\
            if (region->head == NULL)\
                region->head = rect;\
       }

#define REGION_NOT_EMPTY(region) region->head

#define INfoil_rect(r, x, y) \
      ( ( ((r).right >  x)) && \
        ( ((r).left <= x)) && \
        ( ((r).bottom >  y)) && \
        ( ((r).top <= y)) )

extern foil_block_heap __mg_foil_region_rect_freeList;

/* return true if point is in region */
bool foil_region_is_point_in (const foil_region_p region, int x, int y)
{
    int top;
    foil_rgnrc_p cliprect = region->head;

    if (region->type == NULLREGION) {
        return false;
    }

    /* check with bounding rect of clipping region */
    if (y >= region->tail->rc.bottom || y < region->head->rc.top)
        return false;

    /* find the ban in which this point lies */
    cliprect = region->head;
    while (cliprect && y >= cliprect->rc.bottom) {
        cliprect = cliprect->next;
    }

    if (!cliprect) return false;

    /* check in this ban */
    top = cliprect->rc.top;
    while (cliprect && cliprect->rc.top == top) {
        if (INfoil_rect (cliprect->rc, x, y))
            return true;

        cliprect = cliprect->next;
    }

    return false;
}

/* Returns true if rect is at least partly inside region */
bool foil_region_is_rect_in (const foil_region_p region, const foil_rect* rect)
{
    foil_rgnrc_p cliprect = region->head;
    bool ret = false;

    if (cliprect && EXTENTCHECK (&region->rcBound, rect)) {
        while (cliprect) {
            if (cliprect->rc.bottom <= rect->top) {
                cliprect = cliprect->next;
                continue;             /* not far enough down yet */
            }

            if (cliprect->rc.top >= rect->bottom) {
                ret = false;          /* too far down */
                break;
            }

            if (cliprect->rc.right <= rect->left) {
                cliprect = cliprect->next;
                continue;              /* not far enough over yet */
            }

            if (cliprect->rc.left >= rect->right) {
                cliprect = cliprect->next;
                continue;
            }

            ret = true;
            break;
        }
    }

    return ret;
}

/* Init a region */
void foil_region_init (foil_region_p pRgn, foil_block_heap_p heap)
{
     pRgn->type = NULLREGION;
     foil_rect_empty (&pRgn->rcBound);
     pRgn->head = NULL;
     pRgn->tail = NULL;

     pRgn->heap = heap;   // read-only field.
}

#if 0
void foil_region_get_bound_rect (foil_region_p pRgn, foil_rect *pRect)
{
     *pRect = pRgn->rcBound;
}

bool foil_region_is_empty (const foil_region* pRgn)
{
     if (pRgn->head == NULL)
         return true;

     return false;
}
#endif

void foil_region_empty (foil_region_p pRgn)
{
    foil_rgnrc_p pCRect, pTemp;

    pCRect = pRgn->head;
    while (pCRect) {
        pTemp = pCRect->next;
        foil_region_rect_free (pRgn->heap, pCRect);
        pCRect = pTemp;
    }

    pRgn->type = NULLREGION;
    foil_rect_empty (&pRgn->rcBound);
    pRgn->head = NULL;
    pRgn->tail = NULL;
}

/* Creates a region */
foil_region_p foil_region_new (foil_block_heap_p heap)
{
    foil_region_p pRgn = malloc (sizeof(foil_region));
    foil_region_init (pRgn, heap);
    return pRgn;
}

/* Destroys a region */
void foil_region_delete (foil_region_p pRegion)
{
    foil_region_empty (pRegion);
    free (pRegion);
}

/* Reset a region */
bool foil_region_set (foil_region_p pRgn, const foil_rect* pRect)
{
    foil_rgnrc_p pClipRect;

    if (foil_rect_is_empty (pRect))
        return false;

    // empty rgn first
    foil_region_empty (pRgn);

    // get a new clip rect from free list
    pClipRect = foil_region_rect_alloc (pRgn->heap);
    if (pClipRect == NULL)
        return false;

    // set clip rect
    pClipRect->rc = *pRect;
    pClipRect->next = NULL;
    pClipRect->prev = NULL;

    pRgn->type = SIMPLEREGION;
    pRgn->head = pRgn->tail = pClipRect;
    pRgn->rcBound = *pRect;

    return true;
}

bool foil_region_copy (foil_region_p pDstRgn, const foil_region* pSrcRgn)
{
    foil_rgnrc_p pcr;
    foil_rgnrc_p pnewcr, prev;

    // return false if the destination region is not an empty one.
    if (pDstRgn == pSrcRgn)
        return false;

    foil_region_empty (pDstRgn);
    if (!(pcr = pSrcRgn->head))
        return true;

    pnewcr = foil_region_rect_alloc (pDstRgn->heap);

    pDstRgn->head = pnewcr;
    pnewcr->rc = pcr->rc;

    prev = NULL;
    while (pcr->next) {

        pnewcr->next = foil_region_rect_alloc (pDstRgn->heap);
        pnewcr->prev = prev;

        prev = pnewcr;
        pcr = pcr->next;
        pnewcr = pnewcr->next;

        pnewcr->rc = pcr->rc;
    }

    pnewcr->prev = prev;
    pnewcr->next = NULL;
    pDstRgn->tail = pnewcr;

    pDstRgn->type = pSrcRgn->type;
    pDstRgn->rcBound = pSrcRgn->rcBound;

    return true;
}

/* Re-calculate the rcBound of a region */
static void REGION_SetExtents (foil_region *region)
{
    foil_rgnrc_p cliprect;
    foil_rect *pExtents;

    if (region->head == NULL) {
        region->rcBound.left = 0; region->rcBound.top = 0;
        region->rcBound.right = 0; region->rcBound.bottom = 0;
        return;
    }

    pExtents = &region->rcBound;

    /*
     * Since head is the first rectangle in the region, it must have the
     * smallest top and since tail is the last rectangle in the region,
     * it must have the largest bottom, because of banding. Initialize left and
     * right from head and tail, resp., as good things to initialize them
     * to...
     */
    pExtents->left = region->head->rc.left;
    pExtents->top = region->head->rc.top;
    pExtents->right = region->tail->rc.right;
    pExtents->bottom = region->tail->rc.bottom;

    cliprect = region->head;
    while (cliprect) {
        if (cliprect->rc.left < pExtents->left)
            pExtents->left = cliprect->rc.left;
        if (cliprect->rc.right > pExtents->right)
            pExtents->right = cliprect->rc.right;

        cliprect = cliprect->next;
    }
}

/***********************************************************************
 *           REGION_Coalesce
 *
 *      Attempt to merge the rects in the current band with those in the
 *      previous one. Used only by REGION_RegionOp.
 *
 * Results:
 *      The new index for the previous band.
 *
 * Side Effects:
 *      If coalescing takes place:
 *          - rectangles in the previous band will have their bottom fields
 *            altered.
 *          - some clipping rect will be deleted.
 *
 */
static foil_rgnrc* REGION_Coalesce (
             foil_region *region,      /* Region to coalesce */
             foil_rgnrc *prevStart,  /* start of previous band */
             foil_rgnrc *curStart    /* start of current band */
) {
    foil_rgnrc *newStart;         /* Start of new band */
    foil_rgnrc *pPrevRect;        /* Current rect in previous band */
    foil_rgnrc *pCurRect;         /* Current rect in current band */
    foil_rgnrc *temp;             /* Temporary clipping rect */
    int curNumRects;            /* Number of rectangles in current band */
    int prevNumRects;           /* Number of rectangles in previous band */
    int bandtop;                /* top coordinate for current band */

    if (prevStart == NULL) prevStart = region->head;
    if (curStart == NULL) curStart = region->head;

    if (prevStart == curStart)
        return prevStart;

    newStart = pCurRect = curStart;

    pPrevRect = prevStart;
    temp = prevStart;
    prevNumRects = 0;
    while (temp != curStart) {
        prevNumRects ++;
        temp = temp->next;
    }

    /*
     * Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in REGION_RegionOp
     * at the end when one region has been exhausted.
     */
    pCurRect = curStart;
    bandtop = pCurRect->rc.top;
    curNumRects = 0;
    while (pCurRect && (pCurRect->rc.top == bandtop)) {
        curNumRects ++;
        pCurRect = pCurRect->next;
    }

    if (pCurRect) {
        /*
         * If more than one band was added, we have to find the start
         * of the last band added so the next coalescing job can start
         * at the right place... (given when multiple bands are added,
         * this may be pointless -- see above).
         */
        temp = region->tail;
        while (temp->prev->rc.top == temp->rc.top) {
            temp = temp->prev;
        }
        newStart = temp;
    }

    if ((curNumRects == prevNumRects) && (curNumRects != 0)) {
        pCurRect = curStart;
        /*
         * The bands may only be coalesced if the bottom of the previous
         * matches the top scanline of the current.
         */
        if (pPrevRect->rc.bottom == pCurRect->rc.top) {
            /*
             * Make sure the bands have rects in the same places. This
             * assumes that rects have been added in such a way that they
             * cover the most area possible. I.e. two rects in a band must
             * have some horizontal space between them.
             */
            do {
                if ((pPrevRect->rc.left != pCurRect->rc.left) ||
                    (pPrevRect->rc.right != pCurRect->rc.right))
                {
                    /*
                     * The bands don't line up so they can't be coalesced.
                     */
                    return newStart;
                }
                pPrevRect = pPrevRect->next;
                pCurRect = pCurRect->next;
            } while (--prevNumRects);

            /*
             * If only one band was added to the region, we have to backup
             * newStart to the start of the previous band.
             */
            if (pCurRect == NULL) {
                newStart = prevStart;
            }

            /*
             * The bands may be merged, so set the bottom of each rect
             * in the previous band to that of the corresponding rect in
             * the current band.
             */
            /*
             * for implementation of MiniGUI, we should free
             * the clipping rects merged.
             */
            pCurRect = curStart;
            pPrevRect = prevStart;
            do {
                pPrevRect->rc.bottom = pCurRect->rc.bottom;
                pPrevRect = pPrevRect->next;

                if (pCurRect->next)
                    pCurRect->next->prev = pCurRect->prev;
                else
                    region->tail = pCurRect->prev;
                if (pCurRect->prev)
                    pCurRect->prev->next = pCurRect->next;
                else
                    region->head = pCurRect->next;

                temp = pCurRect->next;
                foil_region_rect_free (region->heap, pCurRect);
                pCurRect = temp;
            } while (--curNumRects);

            /*
             *
             * If more than one band was added to the region, copy the
             * other bands down. The assumption here is that the other bands
             * came from the same region as the current one and no further
             * coalescing can be done on them since it's all been done
             * already... newStart is already in the right place.
             */
            /* no need to copy for implementation of MiniGUI -- they are freed.
            if (temp == regionEnd) {
                newStart = prevStart;
            }

            else {
                do {
                    *pPrevRect++ = *pCurRect++;
                } while (pCurRect != regionEnd);
            }
            */

        }
    }
    return (newStart);
}

/***********************************************************************
 *           REGION_RegionOp
 *
 *      Apply an operation to two regions. Called by Union,
 *      Xor, Subtract, Intersect...
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The new region is overwritten.
 *
 * Notes:
 *      The idea behind this function is to view the two regions as sets.
 *      Together they cover a rectangle of area that this function divides
 *      into horizontal bands where points are covered only by one region
 *      or by both. For the first case, the nonOverlapFunc is called with
 *      each the band and the band's upper and lower rcBound. For the
 *      second, the overlapFunc is called to process the entire band. It
 *      is responsible for clipping the rectangles in the band, though
 *      this function provides the boundaries.
 *      At the end of each band, the new region is coalesced, if possible,
 *      to reduce the number of rectangles in the region.
 *
 */
static void
REGION_RegionOp(
            foil_region *newReg, /* Place to store result */
            const foil_region *reg1,   /* First region in operation */
            const foil_region *reg2,   /* 2nd region in operation */
            voidProcp1 overlapFunc,     /* Function to call for over-lapping bands */
            voidProcp2 nonOverlap1Func, /* Function to call for non-overlapping bands in region 1 */
            voidProcp2 nonOverlap2Func  /* Function to call for non-overlapping bands in region 2 */
) {
    foil_region my_dst;
    foil_region* pdst;
    const foil_rgnrc *r1;                 /* Pointer into first region */
    const foil_rgnrc *r2;                 /* Pointer into 2d region */
    const foil_rgnrc *r1BandEnd;          /* End of current band in r1 */
    const foil_rgnrc *r2BandEnd;          /* End of current band in r2 */
    int ybot;                           /* Bottom of intersection */
    int ytop;                           /* Top of intersection */
    foil_rgnrc* prevBand;                 /* start of previous band in newReg */
    foil_rgnrc* curBand;                  /* start of current band in newReg */
    int top;                            /* Top of non-overlapping band */
    int bot;                            /* Bottom of non-overlapping band */

    /*
     * Initialization:
     *  set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    r1 = reg1->head;
    r2 = reg2->head;

    /*
     * newReg may be one of the src regions so we can't empty it. We keep a
     * note of its rects pointer (so that we can free them later), preserve its
     * rcBound and simply set numRects to zero.
     */
    /*
    oldRects = newReg->rects;
    newReg->numRects = 0;
     */

    /*
     * for implementation of MiniGUI, we create an empty region.
     */
    if (newReg == reg1 || newReg == reg2) {
        foil_region_init (&my_dst, newReg->heap);
        pdst = &my_dst;
    }
    else {
        foil_region_empty (newReg);
        pdst = newReg;
    }

    /*
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the Xrealloc() at the end of this function eventually.
     */

    /* for implementation of MiniGUI, dst always is an empty region.
    newReg->size = MAX(reg1->numRects,reg2->numRects) * 2;

    if (! (newReg->rects = malloc( sizeof(foil_rgnrc) * newReg->size )))
    {
        newReg->size = 0;
        return;
    }
     */


    /*
     * Initialize ybot and ytop.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     *  In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *  For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */
    if (reg1->rcBound.top < reg2->rcBound.top)
        ybot = reg1->rcBound.top;
    else
        ybot = reg2->rcBound.top;

    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = pdst->head;

    do {
        curBand = pdst->tail;

        /*
         * This algorithm proceeds one source-band (as opposed to a
         * destination band, which is determined by where the two regions
         * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
         * rectangle after the last one in the current band for their
         * respective regions.
         */
        r1BandEnd = r1;
        while (r1BandEnd && (r1BandEnd->rc.top == r1->rc.top))
            r1BandEnd = r1BandEnd->next;

        r2BandEnd = r2;
        while (r2BandEnd && (r2BandEnd->rc.top == r2->rc.top))
            r2BandEnd = r2BandEnd->next;

        /*
         * First handle the band that doesn't intersect, if any.
         *
         * Note that attention is restricted to one band in the
         * non-intersecting region at once, so if a region has n
         * bands between the current position and the next place it overlaps
         * the other, this entire loop will be passed through n times.
         */
        if (r1->rc.top < r2->rc.top) {
            top = MAX (r1->rc.top, ybot);
            bot = MIN (r1->rc.bottom, r2->rc.top);

            if ((top != bot) && (nonOverlap1Func != NULL))
                (* nonOverlap1Func) (pdst, r1, r1BandEnd, top, bot);

            ytop = r2->rc.top;
        }
        else if (r2->rc.top < r1->rc.top) {
            top = MAX (r2->rc.top, ybot);
            bot = MIN (r2->rc.bottom, r1->rc.top);

            if ((top != bot) && (nonOverlap2Func != NULL))
                (* nonOverlap2Func) (pdst, r2, r2BandEnd, top, bot);

            ytop = r1->rc.top;
        }
        else {
            ytop = r1->rc.top;
        }

        /*
         * If any rectangles got added to the region, try and coalesce them
         * with rectangles from the previous band. Note we could just do
         * this test in miCoalesce, but some machines incur a not
         * inconsiderable cost for function calls, so...
         */
        if (pdst->tail != curBand) {
            prevBand = REGION_Coalesce (pdst, prevBand, curBand);
        }

        /*
         * Now see if we've hit an intersecting band. The two bands only
         * intersect if ybot > ytop
         */
        ybot = MIN (r1->rc.bottom, r2->rc.bottom);
        curBand = pdst->tail;
        if (ybot > ytop)
            (* overlapFunc) (pdst, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);

        if (pdst->tail != curBand)
            prevBand = REGION_Coalesce (pdst, prevBand, curBand);

        /*
         * If we've finished with a band (bottom == ybot) we skip forward
         * in the region to the next band.
         */
        if (r1->rc.bottom == ybot)
            r1 = r1BandEnd;
        if (r2->rc.bottom == ybot)
            r2 = r2BandEnd;
    } while (r1 && r2);

    /*
     * Deal with whichever region still has rectangles left.
     */
    curBand = pdst->tail;
    if (r1) {
        if (nonOverlap1Func != NULL) {
            do {
                r1BandEnd = r1;
                while ((r1BandEnd) && (r1BandEnd->rc.top == r1->rc.top)) {
                    r1BandEnd = r1BandEnd->next;
                }
                (* nonOverlap1Func) (pdst, r1, r1BandEnd,
                                     MAX (r1->rc.top, ybot), r1->rc.bottom);
                r1 = r1BandEnd;
            } while (r1);
        }
    }
    else if ((r2) && (nonOverlap2Func != NULL))
    {
        do {
            r2BandEnd = r2;
            while ((r2BandEnd) && (r2BandEnd->rc.top == r2->rc.top)) {
                 r2BandEnd = r2BandEnd->next;
            }
            (* nonOverlap2Func) (pdst, r2, r2BandEnd,
                                MAX (r2->rc.top, ybot), r2->rc.bottom);
            r2 = r2BandEnd;
        } while (r2);
    }

    if (pdst->tail != curBand)
        (void) REGION_Coalesce (pdst, prevBand, curBand);

    /*
     * A bit of cleanup. To keep regions from growing without bound,
     * we shrink the array of rectangles to match the new number of
     * rectangles in the region. This never goes to 0, however...
     *
     * Only do this stuff if the number of rectangles allocated is more than
     * twice the number of rectangles in the region (a simple optimization...).
     */

    if (pdst != newReg) {
        foil_region_empty (newReg);
        *newReg = my_dst;
    }
}

/***********************************************************************
 *          Region Intersection
 ***********************************************************************/


/***********************************************************************
 *             REGION_IntersectO
 *
 * Handle an overlapping band for REGION_Intersect.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles may be added to the region.
 *
 */
static void
REGION_IntersectO (foil_region *region, const foil_rgnrc *r1, const foil_rgnrc *r1End,
                        const foil_rgnrc *r2, const foil_rgnrc *r2End, int top, int bottom)

{
    int       left, right;
    foil_rgnrc  *newcliprect;

    while (r1 && r2 && (r1 != r1End) && (r2 != r2End))
    {
        left  = MAX (r1->rc.left, r2->rc.left);
        right = MIN (r1->rc.right, r2->rc.right);

        /*
         * If there's any overlap between the two rectangles, add that
         * overlap to the new region.
         * There's no need to check for subsumption because the only way
         * such a need could arise is if some region has two rectangles
         * right next to each other. Since that should never happen...
         */
        if (left < right) {
            NEWfoil_rgnrc (region, newcliprect);

            newcliprect->rc.left = left;
            newcliprect->rc.top = top;
            newcliprect->rc.right = right;
            newcliprect->rc.bottom = bottom;
        }

        /*
         * Need to advance the pointers. Shift the one that extends
         * to the right the least, since the other still has a chance to
         * overlap with that region's next rectangle, if you see what I mean.
         */
        if (r1->rc.right < r2->rc.right) {
            r1 = r1->next;
        }
        else if (r2->rc.right < r1->rc.right) {
            r2 = r2->next;
        }
        else {
            r1 = r1->next;
            r2 = r2->next;
        }
    }
}

/***********************************************************************
 *             Region Union
 ***********************************************************************/

/***********************************************************************
 *             REGION_UnionNonO
 *
 *      Handle a non-overlapping band for the union operation. Just
 *      Adds the rectangles into the region. Doesn't have to check for
 *      subsumption or anything.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      region->numRects is incremented and the final rectangles overwritten
 *      with the rectangles we're passed.
 *
 */
static void
REGION_UnionNonO (foil_region *region, const foil_rgnrc *r, const foil_rgnrc *rEnd, int top, int bottom)
{
    foil_rgnrc *newcliprect;

    while (r && r != rEnd) {
        NEWfoil_rgnrc (region, newcliprect);
        newcliprect->rc.left = r->rc.left;
        newcliprect->rc.top = top;
        newcliprect->rc.right = r->rc.right;
        newcliprect->rc.bottom = bottom;

        r = r->next;
    }
}

/***********************************************************************
 *             REGION_UnionO
 *
 *      Handle an overlapping band for the union operation. Picks the
 *      left-most rectangle each time and merges it into the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Rectangles are overwritten in region->rects and region->numRects will
 *      be changed.
 *
 */
static void
REGION_UnionO(foil_region *region, const foil_rgnrc *r1, const foil_rgnrc *r1End,
                           const foil_rgnrc *r2, const foil_rgnrc *r2End, int top, int bottom)
{
    foil_rgnrc *newcliprect;

#define MERGEfoil_rect(r) \
    if ((region->head) &&  \
        (region->tail->rc.top == top) &&  \
        (region->tail->rc.bottom == bottom) &&  \
        (region->tail->rc.right >= r->rc.left))  \
    {  \
        if (region->tail->rc.right < r->rc.right)  \
        {  \
            region->tail->rc.right = r->rc.right;  \
        }  \
    }  \
    else  \
    {  \
        NEWfoil_rgnrc(region, newcliprect);  \
        newcliprect->rc.top = top;  \
        newcliprect->rc.bottom = bottom;  \
        newcliprect->rc.left = r->rc.left;  \
        newcliprect->rc.right = r->rc.right;  \
    }  \
    r = r->next;

    while (r1 && r2 && (r1 != r1End) && (r2 != r2End))
    {
        if (r1->rc.left < r2->rc.left)
        {
            MERGEfoil_rect(r1);
        }
        else
        {
            MERGEfoil_rect(r2);
        }
    }

    if (r1 && r1 != r1End)
    {
        do {
            MERGEfoil_rect(r1);
        } while (r1 && r1 != r1End);
    }
    else while (r2 && r2 != r2End)
    {
        MERGEfoil_rect(r2);
    }
}

/***********************************************************************
 *             Region Subtraction
 ***********************************************************************/

/***********************************************************************
 *             REGION_SubtractNonO1
 *
 *      Deal with non-overlapping band for subtraction. Any parts from
 *      region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      region may be affected.
 *
 */
static void
REGION_SubtractNonO1 (foil_region *region, const foil_rgnrc *r, const foil_rgnrc *rEnd,
                        int top, int bottom)
{
    foil_rgnrc *newcliprect;

    while (r && r != rEnd) {
        NEWfoil_rgnrc(region, newcliprect);
        newcliprect->rc.left = r->rc.left;
        newcliprect->rc.top = top;
        newcliprect->rc.right = r->rc.right;
        newcliprect->rc.bottom = bottom;
        r = r->next;
    }
}


/***********************************************************************
 *             REGION_SubtractO
 *
 *      Overlapping band subtraction. x1 is the left-most point not yet
 *      checked.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      region may have rectangles added to it.
 *
 */
static void
REGION_SubtractO (foil_region *region, const foil_rgnrc *r1, const foil_rgnrc *r1End,
                        const foil_rgnrc *r2, const foil_rgnrc *r2End, int top, int bottom)
{
    foil_rgnrc *newcliprect;
    int left;

    left = r1->rc.left;
    while (r1 && r2 && (r1 != r1End) && (r2 != r2End)) {
        if (r2->rc.right <= left) {
            /*
             * Subtrahend missed the boat: go to next subtrahend.
             */
            r2 = r2->next;
        }
        else if (r2->rc.left <= left)
        {
            /*
             * Subtrahend preceeds minuend: nuke left edge of minuend.
             */
            left = r2->rc.right;
            if (left >= r1->rc.right)
            {
                /*
                 * Minuend completely covered: advance to next minuend and
                 * reset left fence to edge of new minuend.
                 */
                r1 = r1->next;
                if (r1 != r1End)
                    left = r1->rc.left;
            }
            else
            {
                /*
                 * Subtrahend now used up since it doesn't extend beyond
                 * minuend
                 */
                r2 = r2->next;
            }
        }
        else if (r2->rc.left < r1->rc.right)
        {
            /*
             * Left part of subtrahend covers part of minuend: add uncovered
             * part of minuend to region and skip to next subtrahend.
             */
            NEWfoil_rgnrc(region, newcliprect);
            newcliprect->rc.left = left;
            newcliprect->rc.top = top;
            newcliprect->rc.right = r2->rc.left;
            newcliprect->rc.bottom = bottom;
            left = r2->rc.right;
            if (left >= r1->rc.right)
            {
                /*
                 * Minuend used up: advance to new...
                 */
                r1 = r1->next;
                if (r1 != r1End)
                    left = r1->rc.left;
            }
            else
            {
                /*
                 * Subtrahend used up
                 */
                r2 = r2->next;
            }
        }
        else
        {
            /*
             * Minuend used up: add any remaining piece before advancing.
             */
            if (r1->rc.right > left)
            {
                NEWfoil_rgnrc(region, newcliprect);
                newcliprect->rc.left = left;
                newcliprect->rc.top = top;
                newcliprect->rc.right = r1->rc.right;
                newcliprect->rc.bottom = bottom;
            }
            r1 = r1->next;
            if (r1 != r1End)
                left = r1->rc.left;
        }
    }

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 && r1 != r1End)
    {
        NEWfoil_rgnrc(region, newcliprect);
        newcliprect->rc.left = left;
        newcliprect->rc.top = top;
        newcliprect->rc.right = r1->rc.right;
        newcliprect->rc.bottom = bottom;
        r1 = r1->next;
        if (r1 != r1End)
            left = r1->rc.left;
    }
}

/***********************************************************************
 *             IntersectRegion
 */
bool foil_region_intersect (foil_region *dst, const foil_region *src1, const foil_region *src2)
{
    /* check for trivial reject */
    if ( (!(src1->head)) || (!(src2->head))  ||
        (!EXTENTCHECK(&src1->rcBound, &src2->rcBound)))
    {
        foil_region_empty (dst);
        return false;
    }
    else
        REGION_RegionOp (dst, src1, src2,
            REGION_IntersectO, NULL, NULL);

    /*
     * Can't alter dst's rcBound before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the rcBound of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents(dst);
    dst->type = (dst->head) ? COMPLEXREGION : NULLREGION ;

    return true;
}

/***********************************************************************
 *             foil_region_subtract
 *
 *      Subtract rgnS from rgnM and leave the result in rgnD.
 *      S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *      true.
 *
 * Side Effects:
 *      regD is overwritten.
 *
 */
bool foil_region_subtract (foil_region *rgnD, const foil_region *rgnM, const foil_region *rgnS)
{
    /* check for trivial reject */
    if ( (!(rgnM->head)) || (!(rgnS->head))  ||
            (!EXTENTCHECK (&rgnM->rcBound, &rgnS->rcBound)) ) {
        foil_region_copy (rgnD, rgnM);
        return true;
    }

    REGION_RegionOp (rgnD, rgnM, rgnS, REGION_SubtractO,
                REGION_SubtractNonO1, NULL);

    /*
     * Can't alter newReg's rcBound before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the rcBound of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    REGION_SetExtents (rgnD);
    rgnD->type = (rgnD->head) ? COMPLEXREGION : NULLREGION;

    return true;
}

/***********************************************************************
 *             foil_region_union
 */
bool foil_region_union (foil_region *dst, const foil_region *src1, const foil_region *src2)
{
    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same or region 1 is empty
     */
    if ( (src1 == src2) || (!(src1->head)) ) {
        if (dst != src2)
            foil_region_copy (dst, src2);
        return true;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (!(src2->head)) {
        if (dst != src1)
            foil_region_copy (dst, src1);
        return true;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if ((src1->head == src1->tail) &&
        (src1->rcBound.left <= src2->rcBound.left) &&
        (src1->rcBound.top <= src2->rcBound.top) &&
        (src1->rcBound.right >= src2->rcBound.right) &&
        (src1->rcBound.bottom >= src2->rcBound.bottom))
    {
        if (dst != src1)
            foil_region_copy (dst, src1);
        return true;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if ((src2->head == src2->tail) &&
        (src2->rcBound.left <= src1->rcBound.left) &&
        (src2->rcBound.top <= src1->rcBound.top) &&
        (src2->rcBound.right >= src1->rcBound.right) &&
        (src2->rcBound.bottom >= src1->rcBound.bottom))
    {
        if (dst != src2)
            foil_region_copy(dst, src2);
        return true;
    }

    REGION_RegionOp (dst, src1, src2, REGION_UnionO,
                REGION_UnionNonO, REGION_UnionNonO);

    REGION_SetExtents (dst);
    dst->type = (dst->head) ? COMPLEXREGION : NULLREGION ;
    return true;
}

/* foil_region_xor */
bool foil_region_xor (foil_region *dst, const foil_region *src1, const foil_region *src2)
{
    foil_region tmpa, tmpb;

    foil_region_init (&tmpa, src1->heap);
    foil_region_init (&tmpb, src2->heap);

    foil_region_subtract (&tmpa, src1, src2);
    foil_region_subtract (&tmpb, src2, src1);
    foil_region_union (dst, &tmpa, &tmpb);

    foil_region_empty (&tmpa);
    foil_region_empty (&tmpb);

    return true;
}

/* Adds a rectangle to a region */
bool foil_region_add_rect (foil_region_p region, const foil_rect *rect)
{
    foil_region my_region;
    foil_rgnrc my_cliprect;

    if (foil_rect_is_empty (rect))
        return false;

    my_cliprect.rc = *rect;
    my_cliprect.next = NULL;
    my_cliprect.prev = NULL;

    my_region.type = SIMPLEREGION;
    my_region.rcBound = *rect;
    my_region.head = &my_cliprect;
    my_region.tail = &my_cliprect;
    my_region.heap = NULL;

    foil_region_union (region, region, &my_region);

    return true;
}

/* Intersect a rect with a region */
bool foil_region_intersect_rect (foil_region_p region, const foil_rect* rect)
{
    foil_region my_region;
    foil_rgnrc my_cliprect;

    if (foil_rect_is_empty (rect)) {
        foil_region_empty (region);
        return true;
    }

    my_cliprect.rc = *rect;
    my_cliprect.next = NULL;
    my_cliprect.prev = NULL;

    my_region.type = SIMPLEREGION;
    my_region.rcBound = *rect;
    my_region.head = &my_cliprect;
    my_region.tail = &my_cliprect;
    my_region.heap = NULL;

    foil_region_intersect (region, region, &my_region);

    return true;
}

bool foil_region_subtract_rect (foil_region_p region, const foil_rect* rect)
{
    foil_region my_region;
    foil_rgnrc my_cliprect;

    if (foil_rect_is_empty (rect) || !foil_rect_does_intersect (&region->rcBound, rect))
        return false;

    my_cliprect.rc = *rect;
    my_cliprect.next = NULL;
    my_cliprect.prev = NULL;

    my_region.type = SIMPLEREGION;
    my_region.rcBound = *rect;
    my_region.head = &my_cliprect;
    my_region.tail = &my_cliprect;
    my_region.heap = NULL;

    foil_region_subtract (region, region, &my_region);

    return true;
}

void foil_region_offset_ex (foil_region_p region,
        const foil_rect *rcClient, const foil_rect *rcScroll, int x, int y)
{
    foil_rgnrc* cliprect = region->head;
    //foil_rect old_cliprc, rc_array[4];
    //int i, nCount = 0;
    foil_rect old_cliprc, rc;
    foil_rgnrc_p pTemp;

    if (!rcClient || !rcScroll)
        return;

    if (!foil_rect_intersect (&rc, rcClient, rcScroll))
        return;

    while (cliprect) {
        /*not in scroll window region, return*/
        if (!foil_rect_does_intersect (&cliprect->rc, &rc)) {
            cliprect = cliprect->next;
            continue;
        }

        /*not covered, recalculate cliprect*/
        if (!foil_rect_is_covered_by (&cliprect->rc, &rc)) {
            foil_rect_copy (&old_cliprc, &cliprect->rc);
            foil_rect_intersect (&cliprect->rc, &old_cliprc, &rc);
        }

        foil_rect_offset (&cliprect->rc, x, y);

        /*if not intersect, remove current cliprect from list*/
        if (!foil_rect_does_intersect (&cliprect->rc, &rc)) {
            pTemp = cliprect->next;

            if(cliprect->next)
                cliprect->next->prev = cliprect->prev;
            else
                region->tail = cliprect->prev;
            if(cliprect->prev)
                cliprect->prev->next = cliprect->next;
            else
                region->head = cliprect->next;

            foil_region_rect_free (region->heap, cliprect);
            cliprect = pTemp;
            continue;
        }

        /*if intersect, tune cliprect*/
        if (!foil_rect_is_covered_by (&cliprect->rc, &rc)) {
            foil_rect_copy (&old_cliprc, &cliprect->rc);
            foil_rect_intersect (&cliprect->rc, &old_cliprc, &rc);
        }

        if (region->head) {
            REGION_SetExtents(region);
        }
        cliprect = cliprect->next;
    }

}

