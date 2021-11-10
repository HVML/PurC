#ifndef MC__STRUTILS_ESCAPE_H
#define MC__STRUTILS_ESCAPE_H

#include <config.h>

#include "lib/global.h"         /* <glib.h> */

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

char *strutils_escape (const char *src, gsize src_len, const char *escaped_chars,
                       gboolean escape_non_printable);
char *strutils_unescape (const char *src, gsize src_len, const char *unescaped_chars,
                         gboolean unescape_non_printable);
char *strutils_shell_unescape (const char *text);
char *strutils_shell_escape (const char *text);

char *strutils_glob_escape (const char *text);
char *strutils_glob_unescape (const char *text);

char *strutils_regex_escape (const char *text);
char *strutils_regex_unescape (const char *text);

gboolean strutils_is_char_escaped (const char *start, const char *current);

#endif /* MC__STRUTILS_ESCAPE_H */
