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

#include "rect.h"
#include "foil.h"

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

#define in_rect(r, x, y) \
      ( ( ((r).right >  x)) && \
        ( ((r).left <= x)) && \
        ( ((r).bottom >  y)) && \
        ( ((r).top <= y)) )


static foil_rect max_rect(bool *matrix[], int row, int column, int *area,
        int min_w, int min_h)
{
    foil_rect rc = {0};
    if (row == 0) {
        if (area) {
            area = 0;
        }
        return rc;
    }

    int nums[row][column];
    int maximal = 0, statck[column + 1], top = 0, height, width;

    statck[0] = -1;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < column; j++) {
            if (i == 0) {
                nums[i][j] = matrix[i][j] - 0;
            }
            else if (matrix[i][j] == 1) {
                nums[i][j] = nums[i - 1][j] + 1;
            }
            else {
                nums[i][j] = 0;
            }

            while (top > 0 && nums[i][j] <= nums[i][statck[top]]) {
                height = nums[i][statck[top--]];
                width = j - statck[top] - 1;
                if ((min_w > 0 && width < min_w) ||
                        (min_h > 0 && height < min_h)) {
                    continue;
                }
                if (height * width > maximal) {
                    maximal = height * width;
                    rc.left = top;
                    rc.top = i + 1 - height;
                    rc.right = rc.left + width;
                    rc.bottom = rc.top + height;
                }
            }
            statck[++top] = j;
        }

        while (top > 0) {
            height = nums[i][statck[top]];
            width = statck[top] - statck[top - 1];
            if ((min_w > 0 && width < min_w) ||
                    (min_h > 0 && height < min_h)) {
                top--;
                continue;
            }
            if (height * width > maximal) {
                maximal = height * width;
                rc.left = top - 1;
                rc.top = i + 1 - height;
                rc.right = rc.left + width;
                rc.bottom = rc.top + height;
            }
            top--;
        }
    }

    if (area) {
        *area = maximal;
    }
    return rc;
}

int foil_rect_get_max_inscribed_rect(foil_rect *rects[], size_t nr_rects,
        int min_width, int min_height, foil_rect *dest)
{
    (void)rects;
    (void)nr_rects;
    if (nr_rects == 0) {
        return -1;
    }

    foil_rect rc = *rects[0];
    for (size_t i = 1; i < nr_rects; i++) {
        rc.left = rects[i]->left < rc.left ? rects[i]->left : rc.left;
        rc.top = rects[i]->top < rc.top ? rects[i]->top : rc.top;
        rc.right = rects[i]->right > rc.right ? rects[i]->right : rc.right;
        rc.bottom = rects[i]->bottom > rc.bottom ? rects[i]->bottom : rc.bottom;
    }

    int r = rc.bottom - rc.top;
    int c = rc.right - rc.left;
    bool bits[r][c];
    bool *p[r];

    for (int i = 0; i < r; i++) {
        for (int j = 0; j < c; j ++) {
            int x = rc.left + j;
            int y = rc.top + i;
            bits[i][j] = 0;
            for (size_t k = 0; k < nr_rects; k++) {
                if (in_rect(*rects[k], x, y)) {
                    bits[i][j] = 1;
                    break;
                }
            }
        }
        p[i] = bits[i];
    }

    int area;
    foil_rect max_rc = max_rect(p, r, c, &area, min_width, min_height);

    if (area > 0) {
        rc.left = rc.left + max_rc.left;
        rc.top = rc.top + max_rc.top;
        rc.right = rc.left + (max_rc.right - max_rc.left);
        rc.bottom = rc.top + (max_rc.bottom - max_rc.top);
        if (dest) {
            *dest = rc;
        }
        return 0;
    }

    return -1;
}

