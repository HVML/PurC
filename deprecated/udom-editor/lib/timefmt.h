
/** \file timefmt.h
 *  \brief Header: time formating functions
 */

#ifndef MC__UTIL_TIMEFMT_H
#define MC__UTIL_TIMEFMT_H

#include <sys/time.h>
#include <sys/types.h>

/*** typedefs(not structures) and defined constants **********************************************/

#define MAX_I18NTIMELENGTH 20
#define MIN_I18NTIMELENGTH 10
#define STD_I18NTIMELENGTH 12

#define INVALID_TIME_TEXT "(invalid)"

/* safe localtime formatting - strftime()-using version */
#define FMT_LOCALTIME(buffer, bufsize, fmt, when)               \
    {                                                           \
        struct tm *whentm;                                      \
        whentm = localtime(&when);                              \
        if (whentm == NULL)                                     \
        {                                                       \
            strncpy(buffer, INVALID_TIME_TEXT, bufsize);        \
            buffer[bufsize-1] = 0;                              \
        }                                                       \
        else                                                    \
        {                                                       \
            strftime(buffer, bufsize, fmt, whentm);             \
        }                                                       \
    }                                                           \

#define FMT_LOCALTIME_CURRENT(buffer, bufsize, fmt)             \
    {                                                           \
        time_t __current_time;                                  \
        time(&__current_time);                                  \
        FMT_LOCALTIME(buffer,bufsize,fmt,__current_time);       \
    }

/*** enums ***************************************************************************************/

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

extern char *user_recent_timeformat;    /* time format string for recent dates */
extern char *user_old_timeformat;       /* time format string for older dates */

/*** declarations of public functions ************************************************************/

size_t i18n_checktimelength (void);
const char *file_date (time_t when);

/*** inline functions ****************************************************************************/

#endif /* MC__UTIL_TIMEFMT_H */
