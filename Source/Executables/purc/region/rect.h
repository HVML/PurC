/*
** @file rect.h
** @author Vincent Wei
** @date 2022/11/21
** @brief Interface of rectangle.
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

#ifndef purc_foil_rect_h
#define purc_foil_rect_h

#include "foil.h"

/**
 * \fn void foil_rect_set (foil_rect* prc, int left, int top, int right, int bottom)
 * \brief Set a rectangle.
 *
 * This function sets the rectangle with specified values.
 *
 * \param prc The pointer to the rectangle.
 * \param left The x coordinate of the upper-left corner of the rectangle.
 * \param top The y coordinate of the upper-left corner of the rectangle.
 * \param right The x coordinate of the lower-right corner of the rectangle.
 * \param bottom The y coordinate of the lower-right corner of the rectangle.
 *
 * \sa foil_rect_empty
 */
static inline void
foil_rect_set (foil_rect* prc, int left, int top, int right, int bottom)
{
    (prc)->left = left; (prc)->top = top;
    (prc)->right = right; (prc)->bottom = bottom;
}

/**
 * \fn void foil_rect_empty (foil_rect* prc)
 * \brief Empties a rectangle.
 *
 * This function empties the rectangle pointed to by \a prc.
 * An empty rectangle in MiniGUI is a rectangle whose width and height both
 * are zero. This function will sets all coordinates of the rectangle to
 * be zero.
 *
 * \param prc The pointer to the rectangle.
 *
 * \sa foil_rect_set
 */
static inline void
foil_rect_empty (foil_rect* prc)
{
    (prc)->left = (prc)->top = (prc)->right = (prc)->bottom = 0;
}

/**
 * \fn void foil_rect_copy (foil_rect* pdrc, const foil_rect* psrc)
 * \brief Copies one rectangle to another.
 *
 * This function copies the coordinates of the source rectangle
 * pointed to by \a psrc to the destination rectangle pointed to by \a pdrc.
 *
 * \param pdrc The pointer to the destination rectangle.
 * \param psrc The pointer to the source rectangle.
 *
 * \sa foil_rect_set
 */
static inline void foil_rect_copy
(foil_rect* pdrc, const foil_rect* psrc)
{
    (pdrc)->left = (psrc)->left; (pdrc)->top = (psrc)->top;
    (pdrc)->right = (psrc)->right; (pdrc)->bottom = (psrc)->bottom;
}

/**
 * \fn void foil_rect_offset (foil_rect* prc, int x, int y)
 * \brief Moves a rectangle by offsets.
 *
 * This function moves the specified rectangle by the specified offsets.
 * \a x and \a y specify the amount to move the rectangle left/right or up/down
 * respectively. \a x must be a negative value to move the rectangle to
 * the left, and \a y must be a negative value to move the rectangle up.
 *
 * \param prc The pointer to the rectangle.
 * \param x The x offset.
 * \param y The y offset.
 *
 * \sa foil_rect_inflate
 */
static inline void
foil_rect_offset (foil_rect* prc, int x, int y)
{
    (prc)->left += x; (prc)->top += y; (prc)->right += x; (prc)->bottom += y;
}

/**
 * \fn void foil_rect_inflate (foil_rect* prc, int cx, int cy)
 * \brief Increases or decreases the width and height of an rectangle.
 *
 * This function increases or decreases the width and height of
 * the specified rectangle \a prc. This function adds \a cx units
 * to the left and right ends of the rectangle and \a cy units to the
 * top and bottom. the \a cx and \a cy are signed values; positive values
 * increases the width and height, and negative values decreases them.
 *
 * \param prc The pointer to the rectangle.
 * \param cx The inflating x value.
 * \param cy The inflating y value.
 *
 * \sa foil_rect_inflate_to_point
 */
static inline void
foil_rect_inflate (foil_rect* prc, int cx, int cy)
{
    (prc)->left -= cx; (prc)->top -= cy;
    (prc)->right += cx; (prc)->bottom += cy;
}

/**
 * \fn void foil_rect_inflate_to_point (foil_rect* prc, int x, int y)
 * \brief Inflates a rectangle to contain a point.
 *
 * This function inflates the rectangle \a prc to contain the specified
 * point \a (x,y).
 *
 * \param prc The pointer to the rectangle.
 * \param x x,y: The point.
 * \param y x,y: The point.
 *
 * \sa foil_rect_inflate
 */
static inline void
foil_rect_inflate_to_point (foil_rect* prc, int x, int y)
{
    if ((x) < (prc)->left) (prc)->left = (x);
    if ((y) < (prc)->top) (prc)->top = (y);
    if ((x) > (prc)->right) (prc)->right = (x);
    if ((y) > (prc)->bottom) (prc)->bottom = (y);
}

/**
 * \fn bool foil_rect_is_point_in(const foil_rect* prc, int x, int y)
 * \brief Determine whether a point lies within an rectangle.
 *
 * This function determines whether the specified point \a (x,y) lies within
 * the specified rectangle \a prc.
 *
 * A point is within a rectangle if it lies on the left or top side or is
 * within all four sides. A point on the right or bottom side is considered
 * outside the rectangle.
 *
 * \param prc The pointer to the rectangle.
 * \param x x,y: The point.
 * \param y x,y: The point.
 */
static inline bool
foil_rect_is_point_in(const foil_rect* prc, int x, int y)
{
    if (x >= prc->left && x < prc->right && y >= prc->top && y < prc->bottom)
        return true;
    return false;
}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \fn bool foil_rect_is_empty (const foil_rect* prc)
 * \brief Determine whether an rectangle is empty.
 *
 * This function determines whether the specified rectangle \a prc is empty.
 * An empty rectangle is one that has no area; that is, the coordinates
 * of the right side is equal to the coordinate of the left side, or the
 * coordinates of the bottom side is equal to the coordinate of the top side.
 *
 * \param prc The pointer to the rectangle.
 * \return true for empty, otherwise false.
 */
bool foil_rect_is_empty (const foil_rect* prc);

/**
 * \fn bool foil_rect_is_equal (const foil_rect* prc1, const foil_rect* prc2)
 * \brief Determine whether two rectangles are equal.
 *
 * This function determines whether the two specified rectangles
 * (\a prc1 and \a prc2) are equal by comparing the coordinates of
 * the upper-left and lower-right corners.
 *
 * \param prc1 The pointers to the first rectangles.
 * \param prc2 The pointers to the second rectangles.
 * \return true for equal, otherwise false.
 */
bool foil_rect_is_equal (const foil_rect* prc1, const foil_rect* prc2);

/**
 * \fn void foil_rect_normalize (foil_rect* rect)
 * \brief Normalizes a rectangle.
 *
 * This function normalizes the rectangle pointed to by \a prc
 * so that both the height and width are positive.
 *
 * \param rect The pointer to the rectangle.
 */
void foil_rect_normalize (foil_rect* rect);

/**
 * \fn bool foil_rect_intersect (foil_rect* pdrc, \
                const foil_rect* psrc1, const foil_rect* psrc2)
 * \brief Calculates the intersection of two rectangles.
 *
 * This function calculates the intersection of two source rectangles
 * (\a psrc1 and \a psrc2) and places the coordinates of the intersection
 * rectangle into the destination rectangle pointed to by \a pdrc.
 * If the source rectangles do not intersect, and empty rectangle
 * (in which all coordinates are set to zero) is placed into the destination
 * rectangle.
 *
 * \param pdrc The pointer to the destination rectangle.
 * \param psrc1 The first source rectangles.
 * \param psrc2 The second source rectangles.
 *
 * \return true if the source rectangles intersect, otherwise false.
 *
 * \sa foil_rect_does_intersect, foil_rect_is_covered_by
 */
bool foil_rect_intersect (foil_rect* pdrc,
        const foil_rect* psrc1, const foil_rect* psrc2);

/**
 * \fn bool foil_rect_is_covered_by (const foil_rect* prc1, const foil_rect* prc2)
 * \brief Determine whether one rectangle is covered by another.
 *
 * This function determines whether one rectangle (\a prc1)
 * is covered by another rectangle (\a prc2).
 *
 * \param prc1 The first rectangles.
 * \param prc2 The second rectangles.
 *
 * \return true if the first rectangle is covered by the second,
 *         otherwise false.
 *
 * \sa foil_rect_does_intersect
 */
bool foil_rect_is_covered_by (const foil_rect* prc1, const foil_rect* prc2);

/**
 * \fn bool foil_rect_does_intersect (const foil_rect* psrc1,
 *      const foil_rect* psrc2)
 * \brief Determine whether two rectangles intersect.
 *
 * This function determines whether two rectangles (\a psrc1 and \a psrc2)
 * intersect.
 *
 * \param psrc1 The first source rectangles.
 * \param psrc2 The second source rectangles.
 * \return true if the source rectangles intersect, otherwise false.
 *
 * \sa foil_rect_intersect
 */
bool foil_rect_does_intersect (const foil_rect* psrc1,
        const foil_rect* psrc2);

/**
 * \fn bool foil_rect_union (foil_rect* pdrc,
 *      const foil_rect* psrc1, const foil_rect* psrc2)
 * \brief Unions two source rectangles.
 *
 * This function creates the union (\a pdrc) of two rectangles
 * (\a psrc1 and \a psrc2), if the source rectangles are border upon and
 * not stagger.
 *
 * \param pdrc The unioned rectangle.
 * \param psrc1 The first source rectangles.
 * \param psrc2 The second source rectangles.
 *
 * \return true if the source rectangles are border upon and not stagger,
 *         otherwise false.
 *
 * \sa foil_rect_get_bound
 */
bool foil_rect_union (foil_rect* pdrc,
        const foil_rect* psrc1, const foil_rect* psrc2);

/**
 * \fn void foil_rect_get_bound (foil_rect_p pdrc, \
                const foil_rect* psrc1, const foil_rect* psrc2)
 * \brief Get the bound rectangle of two source rectangles.
 *
 * This function creates the bound rect (\a pdrc) of two rectangles
 * (\a psrc1 and \a prsrc2). The bound rect is the smallest rectangle
 * that contains both source rectangles.
 *
 * \param pdrc The destination rectangle.
 * \param psrc1 The first source rectangle.
 * \param psrc2 The second source rectangle.
 *
 * \sa foil_rect_union
 */
void foil_rect_get_bound (foil_rect_p pdrc,
        const foil_rect* psrc1, const foil_rect* psrc2);

/**
 * \fn int foil_rect_get_subtract (foil_rect* rc,
 *      const foil_rect* psrc1, const foil_rect* psrc2)
 * \brief Obtains the rectangles when substracting one rectangle from another.
 *
 * This function obtains the rectangles substracting the rectangle \a psrc1
 * from the other \a psrc2. \a rc should be an array of foil_rect struct, and
 * may contain at most four rectangles. This function returns
 * the number of result rectangles.
 *
 * \param rc The pointer to the resule rectangle array.
 * \param psrc1 The pointer to the minuend rectangle.
 * \param psrc2 The pointer to the subtrahend rectangle.
 * \return The number of result rectangles.
 *
 * \sa foil_rect_union
 */
int foil_rect_get_subtract (foil_rect* rc,
        const foil_rect* psrc1, const foil_rect* psrc2);

#ifdef __cplusplus
}
#endif

#endif /* purc_foil_rect_h */

