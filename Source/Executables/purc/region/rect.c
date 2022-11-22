/*
** @file rect.c
** @author Vincent Wei
** @date 2022/11/21
** @brief Implemetation of rectangle.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rect.h"

/**************************** Rectangle support ******************************/
bool foil_rect_is_empty (const foil_rect* prc)
{
    if( prc->left == prc->right ) return true;
    if( prc->top == prc->bottom ) return true;
    return false;
}

bool foil_rect_is_equal (const foil_rect* prc1, const foil_rect* prc2)
{
    if(prc1->left != prc2->left) return false;
    if(prc1->top != prc2->top) return false;
    if(prc1->right != prc2->right) return false;
    if(prc1->bottom != prc2->bottom) return false;

    return true;
}

void foil_rect_normalize(foil_rect* pRect)
{
    int iTemp;

    if(pRect->left > pRect->right)
    {
         iTemp = pRect->left;
         pRect->left = pRect->right;
         pRect->right = iTemp;
    }

    if(pRect->top > pRect->bottom)
    {
         iTemp = pRect->top;
         pRect->top = pRect->bottom;
         pRect->bottom = iTemp;
    }
}

bool foil_rect_is_covered_by(const foil_rect* prc1, const foil_rect* prc2)
{
    if (prc1->left < prc2->left
            || prc1->top < prc2->top
            || prc1->right > prc2->right
            || prc1->bottom > prc2->bottom)
        return false;

    return true;
}

bool foil_rect_intersect(foil_rect* pdrc, const foil_rect* psrc1, const foil_rect* psrc2)
{
    pdrc->left = (psrc1->left > psrc2->left) ? psrc1->left : psrc2->left;
    pdrc->top  = (psrc1->top > psrc2->top) ? psrc1->top : psrc2->top;
    pdrc->right = (psrc1->right < psrc2->right) ? psrc1->right : psrc2->right;
    pdrc->bottom = (psrc1->bottom < psrc2->bottom)
                   ? psrc1->bottom : psrc2->bottom;

    if(pdrc->left >= pdrc->right || pdrc->top >= pdrc->bottom)
        return false;

    return true;
}

bool foil_rect_does_intersect (const foil_rect* psrc1, const foil_rect* psrc2)
{
    int left, top, right, bottom;

    left = (psrc1->left > psrc2->left) ? psrc1->left : psrc2->left;
    top  = (psrc1->top > psrc2->top) ? psrc1->top : psrc2->top;
    right = (psrc1->right < psrc2->right) ? psrc1->right : psrc2->right;
    bottom = (psrc1->bottom < psrc2->bottom)
                   ? psrc1->bottom : psrc2->bottom;

    if(left >= right || top >= bottom)
        return false;

    return true;
}

bool foil_rect_union(foil_rect* pdrc, const foil_rect* psrc1, const foil_rect* psrc2)
{
    foil_rect src1, src2;
    memcpy(&src1, psrc1, sizeof(foil_rect));
    memcpy(&src2, psrc2, sizeof(foil_rect));

    foil_rect_normalize(&src1);
    foil_rect_normalize(&src2);

    if (src1.left == src2.left
        && src1.right == src2.right) {
        if (src1.top <= src2.top && src2.top <= src1.bottom) {
            pdrc->left = src1.left;
            pdrc->right = src1.right;
            pdrc->top = src1.top;
            pdrc->bottom = MAX(src1.bottom, src2.bottom);

            return true;
        }
        else if (src1.top >= src2.top && src2.bottom >= src1.top) {
            pdrc->left = src1.left;
            pdrc->right = src1.right;
            pdrc->top = src2.top;
            pdrc->bottom = MAX(src1.bottom, src2.bottom);

            return true;
       }

       return false;
    }

    if (src1.top == src2.top
        && src1.bottom == src2.bottom) {
        if (src1.left <= src2.left && src2.left <= src1.right) {
            pdrc->top = src1.top;
            pdrc->bottom = src1.bottom;
            pdrc->left = src1.left;
            pdrc->right = MAX(src1.right, src2.right);

            return true;
        }
        else if (src1.left >= src2.left && src2.right >= src1.left) {
            pdrc->top = src1.top;
            pdrc->bottom = src1.bottom;
            pdrc->left = src2.left;
            pdrc->right = MAX(src1.right, src2.right);

            return true;
       }

       return false;
    }

    return false;
}

void foil_rect_get_bound (foil_rect_p pdrc,  const foil_rect* psrc1, const foil_rect* psrc2)
{
    foil_rect src1, src2;
    memcpy(&src1, psrc1, sizeof(foil_rect));
    memcpy(&src2, psrc2, sizeof(foil_rect));

    foil_rect_normalize(&src1);
    foil_rect_normalize(&src2);

    if (foil_rect_is_empty(&src1)) {
        foil_rect_copy(pdrc, &src2);
    }else if (foil_rect_is_empty(&src2)) {
        foil_rect_copy(pdrc, &src1);
    }else{
        pdrc->left = (src1.left < src2.left) ? src1.left : src2.left;
        pdrc->top  = (src1.top < src2.top) ? src1.top : src2.top;
        pdrc->right = (src1.right > src2.right) ? src1.right : src2.right;
        pdrc->bottom = (src1.bottom > src2.bottom)
            ? src1.bottom : src2.bottom;
    }
}

int foil_rect_get_subtract(foil_rect* rc, const foil_rect* psrc1, const foil_rect* psrc2)
{
    foil_rect src, rcExpect, *prcExpect;
    int nCount = 0;

    src = *psrc1;
    rcExpect = *psrc2;
    prcExpect = &rcExpect;

    if (!foil_rect_does_intersect (&src, prcExpect)) {
        nCount = 1;
        rc[0] = src;
    }
    else {
        if(prcExpect->top > src.top)
        {
            rc[nCount].left  = src.left;
            rc[nCount].top   = src.top;
            rc[nCount].right = src.right;
            rc[nCount].bottom = prcExpect->top;
            nCount++;
            src.top = prcExpect->top;
        }
        if(prcExpect->bottom < src.bottom)
        {
            rc[nCount].top  = prcExpect->bottom;
            rc[nCount].left   = src.left;
            rc[nCount].right = src.right;
            rc[nCount].bottom = src.bottom;
            nCount++;
            src.bottom = prcExpect->bottom;
        }
        if(prcExpect->left > src.left)
        {
            rc[nCount].left  = src.left;
            rc[nCount].top   = src.top;
            rc[nCount].right = prcExpect->left;
            rc[nCount].bottom = src.bottom;
            nCount++;
        }
        if(prcExpect->right < src.right)
        {
            rc[nCount].left  = prcExpect->right;
            rc[nCount].top   = src.top;
            rc[nCount].right = src.right;
            rc[nCount].bottom = src.bottom;
            nCount++;
        }
    }

    return nCount;
}

