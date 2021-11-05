/** \file widget.h
 *  \brief Header: MC widget and dialog manager: main include file.
 */
#ifndef MC__WIDGET_H
#define MC__WIDGET_H

#include "lib/global.h"         /* GLib */

/* main forward declarations */
struct Widget;
typedef struct Widget Widget;
struct WGroup;
typedef struct WGroup WGroup;

/* Please note that the first element in all the widgets is a     */
/* widget variable of type Widget.  We abuse this fact everywhere */

#include "lib/widget/rect.h"
#include "lib/widget/widget-common.h"
#include "lib/widget/group.h"
#include "lib/widget/background.h"
#include "lib/widget/frame.h"
#include "lib/widget/dialog.h"
#include "lib/widget/history.h"
#include "lib/widget/button.h"
#include "lib/widget/buttonbar.h"
#include "lib/widget/check.h"
#include "lib/widget/hline.h"
#include "lib/widget/gauge.h"
#include "lib/widget/groupbox.h"
#include "lib/widget/label.h"
#include "lib/widget/listbox.h"
#include "lib/widget/menu.h"
#include "lib/widget/radio.h"
#include "lib/widget/input.h"
#include "lib/widget/listbox-window.h"
#include "lib/widget/quick.h"
#include "lib/widget/wtools.h"
#include "lib/widget/dialog-switch.h"

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

/*** inline functions ****************************************************************************/

#endif /* MC__WIDGET_H */
