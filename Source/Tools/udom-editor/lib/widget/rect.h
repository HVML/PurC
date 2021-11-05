
/** \file rect.h
 *  \brief Header: rectangular class
 */

#ifndef MC__WIDGET_RECT_H
#define MC__WIDGET_RECT_H

/*** typedefs (not structures) and defined constants *********************************************/

#define RECT(x) ((WRect *)(x))
#define CONST_RECT(x) ((const WRect *)(x))

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures) ****************************************/

struct WRect;
typedef struct WRect WRect;

struct WRect
{
    int y;
    int x;
    int lines;
    int cols;
};

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

WRect *rect_new (int y, int x, int lines, int cols);
void rect_init (WRect * r, int y, int x, int lines, int cols);
void rect_move (WRect * r, int dy, int dx);
void rect_resize (WRect * r, int dl, int dc);
void rect_intersect (WRect * r, const WRect * r1);
void rect_union (WRect * r, const WRect * r1);
gboolean rects_are_overlapped (const WRect * r1, const WRect * r2);
gboolean rects_are_equal (const WRect * r1, const WRect * r2);

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_RECT_H */
