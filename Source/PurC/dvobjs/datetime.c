/*
 * @file datetime.c
 * @author Vincent Wei
 * @date 2022/03/19
 * @brief The implementation of DATETIME dynamic variant object.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// #undef NDEBUG

#include "config.h"
#include "helper.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/atom-buckets.h"
#include "private/dvobjs.h"
#include "private/ports.h"

#include "purc-variant.h"
#include "purc-dvobjs.h"
#include "purc-version.h"

#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#define LEN_MAX_KEYWORD     64

#define _KN_usec            "usec"
#define _KN_sec             "sec"
#define _KN_min             "min"
#define _KN_hour            "hour"
#define _KN_mday            "mday"
#define _KN_mon             "mon"
#define _KN_year            "year"
#define _KN_wday            "wday"
#define _KN_yday            "yday"
#define _KN_isdst           "isdst"
#define _KN_tz              "tz"

enum {
    K_KW_FORMAT_NAME_FIRST  = 0,

// Atom (example: 2005-08-15T15:52:01+00:00)
#define _KW_atom        "atom"
#define _TF_atom        "%Y-%m-%dT%H:%M:%S{%z:}"     // "Y-m-d\TH:i:sP"
    K_KW_atom = K_KW_FORMAT_NAME_FIRST,
// HTTP Cookies (example: Monday, 15-Aug-2005 15:52:01 UTC)
#define _KW_cookie      "cookie"
#define _TF_cookie      "%A, %d-%m-%Y %H:%M:%S %Z"  // "l, d-M-Y H:i:s T"
    K_KW_cookie,
// Same as 'ATOM' (example: 2005-08-15T15:52:01+0000)
#define _KW_iso8601     "iso8601"
#define _TF_iso8601     "%Y-%m-%dT%H:%M:%S%z"       // "Y-m-d\TH:i:sO"
    K_KW_iso8601,
// RFC 822 (example: Mon, 15 Aug 05 15:52:01 +0000)
#define _KW_rfc822      "rfc822"
#define _TF_rfc822      "%a, %d %b %y %H:%M:%S %z"  // "D, d M y H:i:s O"
    K_KW_rfc822,
// RFC 850 (example: Monday, 15-Aug-05 15:52:01 UTC)
#define _KW_rfc850      "rfc850"
#define _TF_rfc850      "%A, %d-%b-%y %H:%M:%S %Z"  // "l, d-M-y H:i:s T"
    K_KW_rfc850,
// RFC 1036 (example: Mon, 15 Aug 05 15:52:01 +0000)
#define _KW_rfc1036     "rfc1036"
#define _TF_rfc1036     "%a, %d %b %y %H:%M:%S %z"  // "D, d M y H:i:s O"
    K_KW_rfc1036,
// RFC 1123 (example: Mon, 15 Aug 2005 15:52:01 +0000)
#define _KW_rfc1123     "rfc1123"
#define _TF_rfc1123     "%a, %d %b %y %H:%M:%S %z"  // "D, d M Y H:i:s O"
    K_KW_rfc1123,
// RFC 7231 (example: Sat, 30 Apr 2016 17:52:13 GMT)
#define _KW_rfc7231     "rfc7231"
#define _TF_rfc7231     "{UTC}%a, %d %b %y %H:%M:%S GMT"// "D, d M Y H:i:s \G\M\T"
    K_KW_rfc7231,
// RFC 2822 (example: Mon, 15 Aug 2005 15:52:01 +0000)
#define _KW_rfc2822     "rfc2822"
#define _TF_rfc2822     "%a, %d %b %y %H:%M:%S %z"  // "D, d M Y H:i:s O"
    K_KW_rfc2822,
// Same as 'ATOM'
#define _KW_rfc3339     "rfc3339"
#define _TF_rfc3339     "%Y-%m-%dT%H:%M:%S{%z:}"   // "Y-m-d\TH:i:sP"
    K_KW_rfc3339,
// RFC 3339 EXTENDED format (example: 2005-08-15T15:52:01.000+00:00)
#define _KW_rfc3339_ex  "rfc3339-ex"
#define _TF_rfc3339_ex  "%Y-%m-%dT%H:%M:%S.{m}{%z:}"   // "Y-m-d\TH:i:s.vP"
    K_KW_rfc3339_ex,
// RSS (example: Mon, 15 Aug 2005 15:52:01 +0000)
#define _KW_rss         "rss"
#define _TF_rss         "%a, %d %b %y %H:%M:%S %z"  // "D, d M Y H:i:s O"
    K_KW_rss,
// World Wide Web Consortium (example: 2005-08-15T15:52:01+00:00)
#define _KW_w3c         "w3c"
#define _TF_w3c         "%Y-%m-%dT%H:%M:%S{%z:}"   // "Y-m-d\TH:i:sP"
    K_KW_w3c,

    K_KW_FORMAT_NAME_LAST  = K_KW_w3c,
};

static struct keyword_to_atom {
    const char *keyword;
    purc_atom_t atom;
} keywords2atoms [] = {
    { _KW_atom, 0 },        // "atom"
    { _KW_cookie, 0 },      // "cookie"
    { _KW_iso8601, 0 },     // "iso8601"
    { _KW_rfc822, 0 },      // "rfc822"
    { _KW_rfc850, 0 },      // "rfc850"
    { _KW_rfc1036, 0 },     // "rfc1036"
    { _KW_rfc1123, 0 },     // "rfc1123"
    { _KW_rfc7231, 0 },     // "rfc7231"
    { _KW_rfc2822, 0 },     // "rfc2822"
    { _KW_rfc3339, 0 },     // "rfc3339"
    { _KW_rfc3339_ex, 0 },  // "rfc3339-ex"
    { _KW_rss, 0 },         // "rss"
    { _KW_w3c, 0 },         // "w3c"
};

static const char *timeformats[] = {
    _TF_atom,
    _TF_cookie,
    _TF_iso8601,
    _TF_rfc822,
    _TF_rfc850,
    _TF_rfc1036,
    _TF_rfc1123,
    _TF_rfc7231,
    _TF_rfc2822,
    _TF_rfc3339,
    _TF_rfc3339_ex,
    _TF_rss,
    _TF_w3c,
};

static char *set_tz(const char *timezone)
{
    char *tz_old = NULL;

    if (timezone) {
        char *env = getenv("TZ");
        if (env)
            tz_old = strdup(env);

        if (env == NULL || strcmp(env, timezone)) {
            /* change timezone temporarily. */
            char new_timezone[strlen(timezone) + 1];
            strcpy(new_timezone, ":");
            strcat(new_timezone, timezone);
#if OS(WINDOWS)
            _putenv_s("TZ", new_timezone);
#else
            setenv("TZ", new_timezone, 1);
#endif
            tzset();
        }
    }

    return tz_old;
}

static void unset_tz(char *tz_old)
{
    if (tz_old) {
        if (strcmp(getenv("TZ"), tz_old)) {
            // restore old timezone.
#if OS(WINDOWS)
            _putenv_s("TZ", tz_old);
#else
            setenv("TZ", tz_old, 1);
#endif
            tzset();
        }
        free(tz_old);
    }
}

static void get_local_broken_down_time(struct tm *result,
        time_t sec, const char *timezone)
{
    char *tz_old = set_tz(timezone);
#if OS(WINDOWS)
    localtime_s(result, &sec);
#else
    localtime_r(&sec, result);
#endif
    unset_tz(tz_old);
}

static time_t get_time_from_broken_down_time(struct tm *tm,
        const char *timezone)
{
    char *tz_old = set_tz(timezone);
    time_t t = mktime(tm);
    unset_tz(tz_old);

    return t;
}

#define DEF_LEN_ABBR_NAME       32
#define DEF_LEN_FULL_NAME       64
#define DEF_LEN_DATE_ONLY       256
#define DEF_LEN_TIME_ONLY       128
#define DEF_LEN_FULL_DATE       512
#define DEF_LEN_TIMEZONE_NAME   128

static size_t length_of_specifier(int specifier)
{
    switch (specifier) {
        case 'a': // The abbreviated name of the day of the week according to the current locale.
            return DEF_LEN_ABBR_NAME;
        case 'A': // The full name of the day of the week according to the current locale.
            return DEF_LEN_FULL_NAME;
        case 'b': // The abbreviated month name according to the current locale.
            return DEF_LEN_ABBR_NAME;
        case 'B': // The full month name according to the current locale.
            return DEF_LEN_FULL_NAME;
        case 'c': // The preferred date and time representation for the current locale.
            return DEF_LEN_FULL_DATE;
        case 'C': // The century number (year/100) as a 2-digit integer.
            return 2;
        case 'd': // The day of the month as a decimal number (range 01 to 31). 
            return 2;
        case 'D': // Equivalent to %m/%d/%y.
            return 8;
        case 'e': // Like %d, the day of the month as a decimal number, but a leading zero is replaced by a space.
            return 2;
        case 'F': // Equivalent to %Y-%m-%d (the ISO 8601 date format).
            return 10;
        case 'G': // The ISO 8601 week-based year with century as a decimal number.
            return 4;
        case 'g': // Like %G, but without century, that is, with a 2-digit year (00–99).
            return 2;
        case 'h': // Equivalent to %b.
            return DEF_LEN_ABBR_NAME;
        case 'I': // The hour as a decimal number using a 12-hour clock (range 01 to 12).
            return 2;
        case 'j': // The day of the year as a decimal number (range 001 to 366).
            return 3;
        case 'k': // The hour (24-hour clock) as a decimal number (range 0 to 23); single digits are preceded by a blank.
            return 2;
        case 'l': // The hour (12-hour clock) as a decimal number (range 1 to 12); single digits are preceded by a blank.
            return 2;
        case 'm': // The month as a decimal number (range 01 to 12).
            return 2;
        case 'M': // The minute as a decimal number (range 00 to 59).
            return 2;
        case 'n': // A newline character.
            return 1;
        case 'O': // Modifier: use alternative format.
            break;
        case 'p': // Either "AM" or "PM" according to the given time value, or the corresponding strings for the current locale.
            return DEF_LEN_ABBR_NAME;
        case 'P': // Like %p but in lowercase: "am" or "pm" or a corresponding string for the current locale.
            return DEF_LEN_ABBR_NAME;
        case 'r': // The time in a.m. or p.m. notation. In the POSIX locale this is equivalent to %I:%M:%S %p.
            return DEF_LEN_TIME_ONLY;
        case 'R': // The time in 24-hour notation (%H:%M).
            return 5;
        case 's': // The number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
            return 32;
        case 'S': // The second as a decimal number (range 00 to 60).
            return 2;
        case 't': // A tab character. (SU)
            return 1;
        case 'T': // The time in 24-hour notation (%H:%M:%S).
            return 8;
        case 'U': // The  week number of the current year as a decimal number, range 00 to 53, starting with the first Sunday as the first day of week 01.
            return 2;
        case 'V': // The ISO 8601 week number of the current year as a decimal number, range 01 to 53, where week 1 is the first week that has at least 4 days in the new year.
            return 2;
        case 'w': // The day of the week as a decimal, range 0 to 6, Sunday being 0.
            return 1;
        case 'W': // The week number of the current year as a decimal number, range 00 to 53, starting with the first Monday as the first day of week 01.
            return 2;
        case 'x': // The preferred date representation for the current locale without the time.
            return DEF_LEN_DATE_ONLY;
        case 'X': // The preferred time representation for the current locale without the date.
            return DEF_LEN_TIME_ONLY;
        case 'y': // The year as a decimal number without a century (range 00 to 99).
            return 2;
        case 'Y': // The year as a decimal number including the century.
            return 4;
        case 'z': // The +hhmm or -hhmm numeric timezone (that is, the hour and minute offset from UTC).
            return 5;
        case 'Z': // The timezone name or abbreviation.
            return DEF_LEN_TIMEZONE_NAME;
        case '+': // The date and time in date(1) format.
            return DEF_LEN_FULL_DATE;
        case '%': // A literal '%' character.
            return 1;
        default:
            break;
    }

    return 2;
}

static size_t estimate_buffer_size(const char *timeformat)
{
    size_t sz = 0;

    while (*timeformat) {

        if (timeformat[0] == '%' && timeformat[1] != '\0') {
            int specifier = 0;
            if ((timeformat[1] == 'E' || timeformat[1] == 'O') // Modifier
                    && timeformat[2] != '\0') {

                specifier = timeformat[2];
                timeformat++;
            }
            else {
                specifier = timeformat[1];
            }

            if (specifier)
                sz += length_of_specifier(specifier);
            else
                sz += 3;
            timeformat++;
        }
        else {
            sz++;
        }

        timeformat++;
    }

    return sz + 1;
}

typedef char *(*cb_on_found)(const char *needle, size_t len, void *ctxt,
        size_t *rep_len);

#define BRACE_STATE_OUT     0
#define BRACE_STATE_IN      1

static void handle_braces(char *haystack, size_t len,
        cb_on_found on_found, void *ctxt)
{
    int state = BRACE_STATE_OUT;
    char *needle = NULL;
    size_t needle_len = 0;
    char *p = haystack;

    while (len > 0 && p[0]) {
        switch (state) {
        case BRACE_STATE_OUT:
            if (p[0] == '\\' && p[1] == '{') {
                memmove(p, p + 1, len);
                len--;
            }
            else if (p[0] == '\\' && p[1] == '}') {
                memmove(p, p + 1, len);
                len--;
            }
            else if (p[0] == '{') {
                state = BRACE_STATE_IN;
                needle = p;
                needle_len = 1;
            }
            break;

        case BRACE_STATE_IN:
            if (p[0] == '\\' && p[1] == '{') {
                memmove(p, p + 1, len);
                len--;
                needle_len++;
            }
            else if (p[0] == '\\' && p[1] == '}') {
                memmove(p, p + 1, len);
                len--;
                needle_len++;
            }
            else if (p[0] == '}') {
                state = BRACE_STATE_OUT;
                needle_len++;

                size_t rep_len;
                char *replace;
                replace = on_found(needle, needle_len, ctxt, &rep_len);
                if (replace) {
                    if (rep_len < needle_len) {
                        memcpy(needle, replace, rep_len);
                        memmove(needle + rep_len, needle + needle_len, len);
                    }
                    else {
                        memcpy(needle, replace, needle_len);
                        if (rep_len > needle_len)
                            PC_WARN("replacement longer than needle.\n");
                    }

                    free(replace);
                }
            }
            else {
                needle_len++;
            }
            break;
        }

        p++;
        len--;
    }
}

static char *
on_found(const char *needle, size_t len, void *ctxt, size_t *rep_len)
{
    char *result = NULL;

    // PC_DEBUG("In %s: needle: %s, len: %lu\n", __func__, needle, len);

    if (len == 3 && needle[1] == 'm') {
        // {m}
        suseconds_t usec = *(suseconds_t *)ctxt;
        if (usec < 0) usec = 0;
        if (usec > 999999) usec = 999999;

        int msec = usec / 1000;
        assert(msec >= 0 && msec < 1000);

        result = malloc(4);
        int n = snprintf(result, 4, "%03d", msec);
        if (n < 0 || (size_t)n >= 4) {
            free(result);
            result = NULL;
        }
        *rep_len = 3;
    }
    else if (len == 8 && needle[len - 2] == ':' &&
            (needle[1] == '+' || needle[1] == '-') &&
            purc_isdigit(needle[2]) && purc_isdigit(needle[3]) &&
            purc_isdigit(needle[4]) && purc_isdigit(needle[5])) {

        // {+hhmm:}
        result = malloc(8);
        result[0] = needle[1];
        result[1] = needle[2];
        result[2] = needle[3];
        result[3] = ':';
        result[4] = needle[4];
        result[5] = needle[5];
        result[6] = '\0';
        *rep_len = 6;
    }
    else if (len == 7 && needle[len - 2] == ':' &&
            purc_isdigit(needle[1]) && purc_isdigit(needle[2]) &&
            purc_isdigit(needle[3]) && purc_isdigit(needle[4])) {

        // {hhmm:}
        result = malloc(8);
        result[0] = needle[1];
        result[1] = needle[2];
        result[2] = ':';
        result[3] = needle[3];
        result[4] = needle[4];
        result[5] = '\0';
        *rep_len = 5;
    }

    return result;
}

static purc_variant_t
format_broken_down_time(const char *timeformat, const struct tm *tm,
        suseconds_t usec, const char *timezone)
{
    size_t max;
    char *result = NULL;

    max = estimate_buffer_size(timeformat);
    // PC_DEBUG("buffer size for %s: %lu\n", timeformat, max);

    result = malloc(max+1);
    if (result == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    char *tz_old = set_tz(timezone);
    if (strftime(result, max, timeformat, tm) == 0) {
        // should not occur.
        PC_ERROR("Too small buffer to format time\n");
        purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
        return PURC_VARIANT_INVALID;
    }
    unset_tz(tz_old);

    // PC_DEBUG("formated time: %s\n", result);

    /* replace {m}, and {+/-HHMM:} here */
#ifndef NDEBUG
#if 0                    /* { */
    {
        struct _testcase {
            const char *haystack;
            suseconds_t usec;

            const char *result;
        } testcases[] = {
            { "millisecond: {m}", 345000,
              "millisecond: 345" },
            { "colon: {+1030:}", 0,
              "colon: +10:30" },
            { "colon: {0000:}", 0,
              "colon: 00:00" },
            { "colon: {-0200:}", 0,
              "colon: -02:00" },
            { "millisecond: 12:30:55.{m} {+0430:} end of {m}.", 456789,
              "millisecond: 12:30:55.456 +04:30 end of 456." },
            { "{a} {+0430}", 0,
              "{a} {+0430}" },
            { "{abcd} {+abcd:}", 0,
              "{abcd} {+abcd:}" },
            { "\\{m} \\{+1234:}", 0,
              "{m} {+1234:}" },
            { "{m\\} {+1234:\\}", 0,
              "{m} {+1234:}" },
            { "bad {m", 0,
              "bad {m" },
            { "bad {+1234:", 0,
              "bad {+1234:" },
        };

        for (size_t i = 0; i < PCA_TABLESIZE(testcases); i++) {
            char *haystack = strdup(testcases[i].haystack);
            handle_braces(haystack, strlen(haystack),
                    on_found, &testcases[i].usec);

            PC_DEBUG("checking: %s\n", testcases[i].haystack);
            PC_DEBUG("result: %s <-> expected: %s\n",
                    haystack, testcases[i].result);
            assert(strcmp(haystack, testcases[i].result) == 0);
            free(haystack);
        }
    }
#endif                   /* } */
#endif

    handle_braces(result, max, on_found, &usec);

    return purc_variant_make_string_reuse_buff(result, max, false);
}

static purc_variant_t
format_time(const char *timeformat, const struct timeval *tv,
        const char *timezone)
{
    struct tm tm;

    /* check if use UTC */
    if (strncmp(timeformat, PURC_TFORMAT_PREFIX_UTC,
                sizeof(PURC_TFORMAT_PREFIX_UTC) - 1) == 0) {
#if OS(WINDOWS)
    	time_t sec = tv->tv_sec;
        gmtime_s(&tm, &sec);
#else
        gmtime_r(&tv->tv_sec, &tm);
#endif
        timeformat += sizeof(PURC_TFORMAT_PREFIX_UTC) - 1;
    }
    else {
        get_local_broken_down_time(&tm, tv->tv_sec, timezone);
    }

    return format_broken_down_time(timeformat, &tm, tv->tv_usec, timezone);
}

static purc_variant_t
time_prt_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);

    const char *timeformat = NULL;
    const char *timezone = NULL;
    struct timeval tv;

    if (nr_args == 0) {
        gettimeofday(&tv, NULL);
        timeformat = timeformats[K_KW_iso8601];
    }
    else if (nr_args > 0) {
        const char *name;
        purc_atom_t atom;

        if ((name = purc_variant_get_string_const(argv[0])) == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if ((atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, name)) == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        for (int i = K_KW_FORMAT_NAME_FIRST;
                i < (int)PCA_TABLESIZE(keywords2atoms); i++) {
            if (atom == keywords2atoms[i].atom) {
                timeformat = timeformats[i];
                break;
            }
        }

        if (timeformat == NULL) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        if (nr_args > 1) {
            if (purc_variant_is_null(argv[1])) {
                gettimeofday(&tv, NULL);
            }
            else {
                long double time_d, sec_d, usec_d;
                if (!purc_variant_cast_to_longdouble(argv[1], &time_d, false)) {
                    purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                    goto failed;
                }

                if (isinf(time_d) || isnan(time_d)) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    goto failed;
                }

                usec_d = modfl(time_d, &sec_d);
                tv.tv_sec = (time_t)sec_d;
                tv.tv_usec = (suseconds_t)(usec_d * 1000000.0);
            }
        }
        else {
            gettimeofday(&tv, NULL);
        }

        if (nr_args > 2) {
            const char *tz = NULL;
            if ((tz = purc_variant_get_string_const(argv[2])) == NULL) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            if (!pcdvobjs_is_valid_timezone(tz)) {
                goto failed;
            }

            timezone = tz;
        }
    }

    return format_time(timeformat, &tv, timezone);

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
make_broken_down_time(const struct tm *tm, suseconds_t usec,
        const char *timezone)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    char buff[MAX_LEN_TIMEZONE];
    if (timezone == NULL) {
        if (!pcdvobjs_get_current_timezone(buff, sizeof(buff))) {
            goto fatal;
        }
        timezone = buff;
    }

    retv = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (retv == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto fatal;
    }

    val = purc_variant_make_number(tm->tm_sec);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_sec, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_number(usec);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_usec, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_number(tm->tm_min);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_min, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_number (tm->tm_hour);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_hour, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_number(tm->tm_mday);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_mday, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_number(tm->tm_mon);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_mon, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_number(tm->tm_year);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_year, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_number(tm->tm_wday);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_wday, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_number(tm->tm_yday);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_yday, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_number(tm->tm_isdst);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_isdst, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string(timezone, false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv, _KN_tz, val))
        goto fatal;
    purc_variant_unref(val);

    return retv;

fatal:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}

static bool
reflect_changes_to_broken_down_time(purc_variant_t bdtime,
        const struct tm *tm)
{
    purc_variant_t val = PURC_VARIANT_INVALID;
    int32_t cval;

    /* we do not validate val and cval here, because we have done this
       in function get_broken_down_time() before calling this function. */
    val = purc_variant_object_get_by_ckey(bdtime, _KN_mday);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    purc_variant_cast_to_int32(val, &cval, false);
    if (cval != tm->tm_mday) {
        val = purc_variant_make_number((double)tm->tm_mday);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(bdtime, _KN_mday, val))
            goto fatal;
        purc_variant_unref(val);
    }

    val = purc_variant_object_get_by_ckey(bdtime, _KN_mon);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    purc_variant_cast_to_int32(val, &cval, false);
    if (cval != tm->tm_mon) {
        val = purc_variant_make_number((double)tm->tm_mon);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(bdtime, _KN_mon, val))
            goto fatal;
        purc_variant_unref(val);
    }

    val = purc_variant_object_get_by_ckey(bdtime, _KN_year);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    purc_variant_cast_to_int32(val, &cval, false);
    if (cval != tm->tm_year) {
        val = purc_variant_make_number((double)tm->tm_year);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(bdtime, _KN_year, val))
            goto fatal;
        purc_variant_unref(val);
    }

    val = purc_variant_object_get_by_ckey(bdtime, _KN_wday);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    purc_variant_cast_to_int32(val, &cval, false);
    if (cval != tm->tm_wday) {
        val = purc_variant_make_number((double)tm->tm_wday);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(bdtime, _KN_wday, val))
            goto fatal;
        purc_variant_unref(val);
    }

    val = purc_variant_object_get_by_ckey(bdtime, _KN_yday);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    purc_variant_cast_to_int32(val, &cval, false);
    if (cval != tm->tm_yday) {
        val = purc_variant_make_number((double)tm->tm_yday);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(bdtime, _KN_yday, val))
            goto fatal;
        purc_variant_unref(val);
    }

    val = purc_variant_object_get_by_ckey(bdtime, _KN_isdst);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    purc_variant_cast_to_int32(val, &cval, false);
    if (cval != tm->tm_isdst) {
        val = purc_variant_make_number((double)tm->tm_isdst);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(bdtime, _KN_isdst, val))
            goto fatal;
        purc_variant_unref(val);
    }

    return true;

fatal:
    if (val)
        purc_variant_unref(val);

    return false;
}

static purc_variant_t
utctime_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    struct timeval tv;

    if (nr_args == 0 || purc_variant_is_null(argv[0])) {
        gettimeofday(&tv, NULL);
    }
    else {
        long double time_d, sec_d, usec_d;
        if (!purc_variant_cast_to_longdouble(argv[0], &time_d, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (isinf(time_d) || isnan(time_d)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        usec_d = modfl(time_d, &sec_d);
        tv.tv_sec = (time_t)sec_d;
        tv.tv_usec = (suseconds_t)(usec_d * 1000000.0);
    }

    struct tm result;
#if OS(WINDOWS)
    time_t sec = tv.tv_sec;
    gmtime_s(&result, &sec);
#else
    gmtime_r(&tv.tv_sec, &result);
#endif
    return make_broken_down_time(&result, tv.tv_usec, PURC_TIMEZONE_UTC);

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
localtime_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    struct timeval tv;
    const char *timezone = NULL;

    if (nr_args == 0 || purc_variant_is_null(argv[0])) {
        gettimeofday(&tv, NULL);
    }
    else {
        long double time_d, sec_d, usec_d;
        if (!purc_variant_cast_to_longdouble(argv[0], &time_d, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (isinf(time_d) || isnan(time_d)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        usec_d = modfl(time_d, &sec_d);
        tv.tv_sec = (time_t)sec_d;
        tv.tv_usec = (suseconds_t)(usec_d * 1000000.0);
    }

    if (nr_args > 1) {
        const char *tz = NULL;
        if ((tz = purc_variant_get_string_const(argv[1])) == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (!pcdvobjs_is_valid_timezone(tz)) {
            goto failed;
        }

        timezone = tz;
    }

    struct tm result;
    get_local_broken_down_time(&result, tv.tv_sec, timezone);
    return make_broken_down_time(&result, tv.tv_usec, timezone);

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
fmttime_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    const char *timeformat = NULL;
    const char *timezone = NULL;
    struct timeval tv;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if ((timeformat = purc_variant_get_string_const(argv[0])) == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args == 1 || purc_variant_is_null(argv[1])) {
        gettimeofday(&tv, NULL);
    }
    else {
        long double time_d, sec_d, usec_d;
        if (!purc_variant_cast_to_longdouble(argv[1], &time_d, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (isinf(time_d) || isnan(time_d)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        usec_d = modfl(time_d, &sec_d);
        tv.tv_sec = (time_t)sec_d;
        tv.tv_usec = (suseconds_t)(usec_d * 1000000.0);
    }

    if (nr_args > 2) {
        const char *tz = NULL;
        if ((tz = purc_variant_get_string_const(argv[2])) == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (!pcdvobjs_is_valid_timezone(tz)) {
            goto failed;
        }

        timezone = tz;
    }

    return format_time(timeformat, &tv, timezone);

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static const char *
get_broken_down_time(purc_variant_t bdtime, struct tm *tm, suseconds_t *usec)
{
    const char *timezone;
    double number;
    purc_variant_t val;

    if (!purc_variant_is_object(bdtime)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return NULL;
    }

    val = purc_variant_object_get_by_ckey(bdtime, _KN_tz);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if ((timezone = purc_variant_get_string_const(val)) == NULL) {
        goto failed;
    }
    if (!pcdvobjs_is_valid_timezone(timezone)) {
        goto failed;
    }

    val = purc_variant_object_get_by_ckey(bdtime, _KN_usec);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false) ||
            number < 0 || number > 999999.0) {
        goto failed;
    }
    *usec = (suseconds_t)number;

    val = purc_variant_object_get_by_ckey(bdtime, _KN_sec);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false) ||
            number < 0 || number > 60.0) {
        goto failed;
    }
    tm->tm_sec = (int)number;

    val = purc_variant_object_get_by_ckey(bdtime, _KN_min);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false) ||
            number < 0 || number > 59.0) {
        goto failed;
    }
    tm->tm_min = (int)number;

    val = purc_variant_object_get_by_ckey(bdtime, _KN_hour);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false) ||
            number < 0 || number > 23.0) {
        goto failed;
    }
    tm->tm_hour = (int)number;

    val = purc_variant_object_get_by_ckey(bdtime, _KN_mday);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false) ||
            number < 1.0 || number > 31.0) {
        goto failed;
    }
    tm->tm_mday = (int)number;

    val = purc_variant_object_get_by_ckey(bdtime, _KN_mon);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false) ||
            number < 0 || number > 11.0) {
        goto failed;
    }
    tm->tm_mon = (int)number;

    val = purc_variant_object_get_by_ckey(bdtime, _KN_year);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false)) {
        goto failed;
    }
    tm->tm_year = (int)number;

    val = purc_variant_object_get_by_ckey(bdtime, _KN_wday);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false) ||
            number < 0 || number > 6.0) {
        goto failed;
    }
    tm->tm_wday = (int)number;

    val = purc_variant_object_get_by_ckey(bdtime, _KN_yday);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false) ||
            number < 0 || number > 365.0) {
        goto failed;
    }
    tm->tm_yday = (int)number;

    val = purc_variant_object_get_by_ckey(bdtime, _KN_isdst);
    if (val == PURC_VARIANT_INVALID) {
        goto failed;
    }
    if (!purc_variant_cast_to_number(val, &number, false)) {
        goto failed;
    }
    if (number == 0)
        tm->tm_isdst = 0;
    if (number > 0)
        tm->tm_isdst = 1;
    if (number < 0)
        tm->tm_isdst = -1;

    char *tz_old = set_tz(timezone);
    time_t t = mktime(tm);
#if OS(WINDOWS)
    localtime_s(tm, &t);
#else
    localtime_r(&t, tm);
#endif
    unset_tz(tz_old);

    return timezone;

failed:
    purc_set_error(PURC_ERROR_INVALID_VALUE);
    return NULL;
}

static purc_variant_t
fmtbdtime_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    const char *timeformat = NULL;
    const char *timezone = NULL;
    struct tm tm;
    suseconds_t usec;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if ((timeformat = purc_variant_get_string_const(argv[0])) == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (purc_variant_is_null(argv[1])) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        get_local_broken_down_time(&tm, tv.tv_sec, NULL);
        usec = tv.tv_usec;
    }
    else {
        if ((timezone = get_broken_down_time(argv[1], &tm, &usec)) == NULL) {
            goto failed;
        }

        if (!pcdvobjs_is_valid_timezone(timezone)) {
            goto failed;
        }
    }

    /* skip the possible UTC prefix */
    if (strncmp(timeformat, PURC_TFORMAT_PREFIX_UTC,
                sizeof(PURC_TFORMAT_PREFIX_UTC) - 1) == 0) {
        timeformat += sizeof(PURC_TFORMAT_PREFIX_UTC) - 1;
    }
    return format_broken_down_time(timeformat, &tm, usec, timezone);

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
mktime_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    struct tm tm;
    suseconds_t usec;
    const char *timezone = NULL;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if ((timezone = get_broken_down_time(argv[0], &tm, &usec)) == NULL) {
        goto failed;
    }

    if (!pcdvobjs_is_valid_timezone(timezone)) {
        goto failed;
    }

    time_t result = get_time_from_broken_down_time(&tm, timezone);
    if (result == -1) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    /* mktime() may change fields in tm. So we should reflect
       the changes to the broken-down time object (argv[0]). */
    if (!reflect_changes_to_broken_down_time(argv[0], &tm)) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;    /* not ignorable. */
    }

    long double time_ld = (long double)result;
    time_ld += usec/1000000.0L;
    return purc_variant_make_longdouble(time_ld);

failed:
    if (silently)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_datetime_new(void)
{
    static const struct purc_dvobj_method methods[] = {
        { "time_prt",   time_prt_getter,    NULL },
        { "utctime",    utctime_getter,     NULL },
        { "localtime",  localtime_getter,   NULL },
        { "fmttime",    fmttime_getter,     NULL },
        { "fmtbdtime",  fmtbdtime_getter,   NULL },
        { "mktime",     mktime_getter,      NULL },
    };

    if (keywords2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
            keywords2atoms[i].atom =
                purc_atom_from_static_string_ex(ATOM_BUCKET_DVOBJ,
                    keywords2atoms[i].keyword);
        }
    }

    return purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
}

