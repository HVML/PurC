/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc-variant.h"
#include "purc-dvobjs.h"
#include "purc-ports.h"

#include "config.h"
#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>

#include <gtest/gtest.h>

#if 0
static void
_trim_tail_spaces(char *dest, size_t n)
{
    while (n>1) {
        if (!isspace(dest[n-1]))
            break;
        dest[--n] = '\0';
    }
}

static size_t
_fetch_cmd_output(const char *cmd, char *dest, size_t sz)
{
    FILE *fin = NULL;
    size_t n = 0;

    fin = popen(cmd, "r");
    if (!fin)
        return 0;

    n = fread(dest, 1, sz - 1, fin);
    dest[n] = '\0';

    if (pclose(fin)) {
        return 0;
    }

    _trim_tail_spaces(dest, n);
    return n;
}
#endif

TEST(dvobjs, basic)
{
    purc_instance_extra_info info = {};
    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobjs", &info);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t dvobj;

    dvobj = purc_dvobj_datetime_new();
    ASSERT_EQ(purc_variant_is_object(dvobj), true);
    purc_variant_unref(dvobj);

    purc_cleanup();
}

static purc_variant_t get_dvobj_datetime(void* ctxt, const char* name)
{
    if (strcmp(name, "DATETIME") == 0) {
        return (purc_variant_t)ctxt;
    }

    return PURC_VARIANT_INVALID;
}

typedef purc_variant_t (*fn_expected)(purc_variant_t dvobj, const char* name);
typedef bool (*fn_cmp)(purc_variant_t result, purc_variant_t expected);

struct ejson_result {
    const char             *name;
    const char             *ejson;

    fn_expected             expected;
    fn_cmp                  vrtcmp;
    int                     errcode;
};

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
// Similiar to 'ATOM' (example: 2005-08-15T15:52:01+0000)
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

static struct keyword_to_format {
    const char *keyword;
    const char *format;
} keywords2formats [] = {
    { _KW_atom, _TF_atom },        // "atom"
    { _KW_cookie, _TF_cookie },      // "cookie"
    { _KW_iso8601, _TF_iso8601 },     // "iso8601"
    { _KW_rfc822, _TF_rfc822 },      // "rfc822"
    { _KW_rfc850, _TF_rfc850 },      // "rfc850"
    { _KW_rfc1036, _TF_rfc1036 },     // "rfc1036"
    { _KW_rfc1123, _TF_rfc1123 },     // "rfc1123"
    { _KW_rfc7231, _TF_rfc7231 },     // "rfc7231"
    { _KW_rfc2822, _TF_rfc2822 },     // "rfc2822"
    { _KW_rfc3339, _TF_rfc3339 },     // "rfc3339"
    { _KW_rfc3339_ex, _TF_rfc3339_ex },  // "rfc3339-ex"
    { _KW_rss, _TF_rss },         // "rss"
    { _KW_w3c, _TF_w3c },         // "w3c"
};

purc_variant_t time_prt(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;
    const char *timeformat = NULL;
    const char *timezone = NULL;
    time_t t;

    if (strcmp(name, "default") == 0) {
        timeformat = _TF_iso8601;
        t = time(NULL);
    }
    else if (strcmp(name, "iso8601-timezone") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = time(NULL);
        timezone = ":America/New_York";
    }
    else if (strcmp(name, "iso8601-epoch") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = 0;
    }
    else if (strcmp(name, "iso8601-epoch-timezone") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = 0;
        timezone = ":America/New_York";
    }
    else if (strcmp(name, "iso8601-before-epoch") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = -3600;
    }
    else if (strcmp(name, "iso8601-before-epoch-timezone") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = -3600;
        timezone = ":America/New_York";
    }
    else {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2formats); i++) {
            if (strcmp(name, keywords2formats[i].keyword) == 0) {
                timeformat = keywords2formats[i].format;
            }
        }

        t = time(NULL);
    }

    if (timeformat) {
        char buf[256];
        struct tm tm;

        if (strncmp(timeformat, PURC_TFORMAT_PREFIX_UTC,
                    sizeof(PURC_TFORMAT_PREFIX_UTC) - 1) == 0) {
            gmtime_r(&t, &tm);
            timeformat += sizeof(PURC_TFORMAT_PREFIX_UTC) - 1;
        }
        else {
            char *tz_old = NULL;
            if (timezone) {
                char *env = getenv("TZ");
                if (env)
                    tz_old = strdup(env);

                setenv("TZ", timezone, 1);
                tzset();
            }

            localtime_r(&t, &tm);

            if (tz_old) {
                setenv("TZ", tz_old, 1);
                tzset();
                free(tz_old);
            }
        }
        strftime(buf, sizeof(buf), timeformat, &tm);

        return purc_variant_make_string(buf, false);
    }

    return purc_variant_make_boolean(false);
}

static bool time_prt_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    const char *t1, *t2;

    t1 = purc_variant_get_string_const(result);
    t2 = purc_variant_get_string_const(expected);

    if (t1 && t2) {
        int diff = strcmp(t1, t2);
        if (diff) {
            purc_log_info("result: %s <-> expected: %s\n", t1, t2);
        }
        return (diff == 0);
    }

    return false;
}

static bool time_prt_vrtcmp_ex(purc_variant_t result, purc_variant_t expected)
{
    const char *t1, *t2;

    t1 = purc_variant_get_string_const(result);
    t2 = purc_variant_get_string_const(expected);

    if (t1 && t2) {
        int diff = strncmp(t1, t2, 10);
        if (diff) {
            purc_log_info("result: %s <-> expected: %s\n", t1, t2);
        }
        return (diff == 0);
    }

    return false;
}

static purc_variant_t
eval(const char *ejson, purc_variant_t dvobj)
{
    purc_variant_t result;

    struct purc_ejson_parse_tree *ptree;
    ptree = purc_variant_ejson_parse_string(ejson, strlen(ejson));
    result = purc_variant_ejson_parse_tree_evalute(ptree,
            get_dvobj_datetime, dvobj, true);
    purc_variant_ejson_parse_tree_destroy(ptree);

    return result;
}

TEST(dvobjs, time_prt)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$DATETIME.time_prt('bad')",
            time_prt, NULL, PURC_ERROR_INVALID_VALUE },
        { "bad",
            "$DATETIME.time_prt('iso8601', false, false)",
            time_prt, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$DATETIME.time_prt('iso8601', 3600, false)",
            time_prt, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$DATETIME.time_prt('iso8601', 3600, 'Bad/Timezone')",
            time_prt, NULL, PURC_ERROR_INVALID_VALUE },
        { "default",
            "$DATETIME.time_prt",
            time_prt, time_prt_vrtcmp, 0 },
        { "default",
            "$DATETIME.time_prt()",
            time_prt, time_prt_vrtcmp, 0 },
        { "atom",
            "$DATETIME.time_prt('atom')",
            time_prt, time_prt_vrtcmp_ex, 0 },
        { "cookie",
            "$DATETIME.time_prt('cookie')",
            time_prt, time_prt_vrtcmp, 0 },
        { "iso8601",
            "$DATETIME.time_prt('iso8601')",
            time_prt, time_prt_vrtcmp, 0 },
        { "rfc822",
            "$DATETIME.time_prt('rfc822')",
            time_prt, time_prt_vrtcmp, 0 },
        { "rfc850",
            "$DATETIME.time_prt('rfc850')",
            time_prt, time_prt_vrtcmp, 0 },
        { "rfc1036",
            "$DATETIME.time_prt('rfc1036')",
            time_prt, time_prt_vrtcmp, 0 },
        { "rfc1123",
            "$DATETIME.time_prt('rfc1123')",
            time_prt, time_prt_vrtcmp, 0 },
        { "rfc7231",
            "$DATETIME.time_prt('rfc7231')",
            time_prt, time_prt_vrtcmp, 0 },
        { "rfc2822",
            "$DATETIME.time_prt('rfc2822')",
            time_prt, time_prt_vrtcmp, 0 },
        { "rfc3339",
            "$DATETIME.time_prt('rfc3339')",
            time_prt, time_prt_vrtcmp_ex, 0 },
        { "rfc3339-ex",
            "$DATETIME.time_prt('rfc3339-ex')",
            time_prt, time_prt_vrtcmp_ex, 0 },
        { "rss",
            "$DATETIME.time_prt('rss')",
            time_prt, time_prt_vrtcmp, 0 },
        { "w3c",
            "$DATETIME.time_prt('w3c')",
            time_prt, time_prt_vrtcmp_ex, 0 },
        { "iso8601",
            "$DATETIME.time_prt('iso8601', null)",
            time_prt, time_prt_vrtcmp, 0 },
        { "iso8601-timezone",
            "$DATETIME.time_prt('iso8601', null, 'America/New_York')",
            time_prt, time_prt_vrtcmp, 0 },
        { "iso8601-epoch",
            "$DATETIME.time_prt('iso8601', 0)",
            time_prt, time_prt_vrtcmp, 0 },
        { "iso8601-epoch-timezone",
            "$DATETIME.time_prt('iso8601', 0, 'America/New_York')",
            time_prt, time_prt_vrtcmp, 0 },
        { "iso8601-before-epoch",
            "$DATETIME.time_prt('iso8601', -3600)",
            time_prt, time_prt_vrtcmp, 0 },
        { "iso8601-before-epoch-timezone",
            "$DATETIME.time_prt('iso8601', -3600, 'America/New_York')",
            time_prt, time_prt_vrtcmp, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobjs", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t dvobj = purc_dvobj_datetime_new();
    ASSERT_NE(dvobj, nullptr);
    ASSERT_EQ(purc_variant_is_object(dvobj), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        result = eval(test_cases[i].ejson, dvobj);

        /* FIXME: purc_variant_ejson_parse_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(dvobj, test_cases[i].name);

            if (purc_variant_get_type(result) != purc_variant_get_type(expected)) {
                purc_log_error("result type: %s, error message: %s\n",
                        purc_variant_typename(purc_variant_get_type(result)),
                        purc_get_error_message(purc_get_last_error()));
            }

            if (test_cases[i].vrtcmp) {
                if (test_cases[i].vrtcmp(result, expected) != true) {
                    PURC_VARIANT_SAFE_CLEAR(result);
                    result = eval(test_cases[i].ejson, dvobj);
                }
                ASSERT_EQ(test_cases[i].vrtcmp(result, expected), true);
            }
            else {
                ASSERT_EQ(purc_variant_is_equal_to(result, expected), true);
            }

            if (test_cases[i].errcode) {
                ASSERT_EQ(purc_get_last_error(), test_cases[i].errcode);
            }

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }

    purc_variant_unref(dvobj);
    purc_cleanup();
}

purc_variant_t fmttime(purc_variant_t dvobj, const char* name)
{
    (void)dvobj;
    const char *timeformat = NULL;
    const char *timezone = NULL;
    time_t t;

    if (strcmp(name, "bad") == 0) {
        // do not assign timeformat
    }
    else if (strcmp(name, "object") == 0) {
        return purc_variant_make_object(0, NULL, NULL);
    }
    else if (strcmp(name, "iso8601") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = time(NULL);
    }
    else if (strcmp(name, "iso8601-timezone") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = time(NULL);
        timezone = ":America/New_York";
    }
    else if (strcmp(name, "iso8601-epoch") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = 0;
    }
    else if (strcmp(name, "iso8601-epoch-utc") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = 0;
        timezone = ":UTC";
    }
    else if (strcmp(name, "iso8601-epoch-timezone") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = 0;
        timezone = ":America/New_York";
    }
    else if (strcmp(name, "iso8601-before-epoch") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = -3600;
    }
    else if (strcmp(name, "iso8601-before-epoch-utc") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = -3600;
        timezone = ":UTC";
    }
    else if (strcmp(name, "iso8601-before-epoch-timezone") == 0) {
        timeformat = keywords2formats[K_KW_iso8601].format;
        t = -3600;
        timezone = ":America/New_York";
    }
    else {
        timeformat = name;
        t = time(NULL);
    }

    if (timeformat) {
        char buf[256];
        struct tm tm;

        char *tz_old = NULL;
        if (timezone) {
            char *env = getenv("TZ");
            if (env)
                tz_old = strdup(env);

            setenv("TZ", timezone, 1);
            tzset();
        }

        if (timezone && strcmp(timezone, ":UTC") == 0) {
            gmtime_r(&t, &tm);
        }
        else if (strncmp(timeformat, PURC_TFORMAT_PREFIX_UTC,
                    sizeof(PURC_TFORMAT_PREFIX_UTC) - 1) == 0) {
            gmtime_r(&t, &tm);
            timeformat += sizeof(PURC_TFORMAT_PREFIX_UTC) - 1;
        }
        else {
            localtime_r(&t, &tm);
        }

        strftime(buf, sizeof(buf), timeformat, &tm);

        if (tz_old) {
            setenv("TZ", tz_old, 1);
            tzset();
            free(tz_old);
        }

        return purc_variant_make_string(buf, false);
    }

    return purc_variant_make_boolean(false);
}

static bool fmttime_vrtcmp(purc_variant_t result, purc_variant_t expected)
{
    const char *t1, *t2;

    t1 = purc_variant_get_string_const(result);
    t2 = purc_variant_get_string_const(expected);

    if (t1 && t2) {
        purc_log_info("result: %s <-> expected: %s\n", t1, t2);
        return strcmp(t1, t2) == 0;
    }

    return false;
}

TEST(dvobjs, fmttime)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$DATETIME.fmttime",
            fmttime, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$DATETIME.fmttime()",
            fmttime, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$DATETIME.fmttime(false)",
            fmttime, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$DATETIME.fmttime('bad', false)",
            fmttime, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$DATETIME.fmttime('bad', 0, false)",
            fmttime, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$DATETIME.fmttime('bad', 3600, 'Bad/TimeZone')",
            fmttime, NULL, PURC_ERROR_INVALID_VALUE },
        { "iso8601",
            "$DATETIME.fmttime('%Y-%m-%dT%H:%M:%S%z')",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601",
            "$DATETIME.fmttime('%Y-%m-%dT%H:%M:%S%z', null)",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-timezone",
            "$DATETIME.fmttime('%Y-%m-%dT%H:%M:%S%z', null, 'America/New_York')",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-epoch",
            "$DATETIME.fmttime('%Y-%m-%dT%H:%M:%S%z', 0)",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-epoch-timezone",
            "$DATETIME.fmttime('%Y-%m-%dT%H:%M:%S%z', 0, 'America/New_York')",
            fmttime, fmttime_vrtcmp, 0 },
        { "{UTC}It is %H:%M now in UTC",
            "$DATETIME.fmttime('{UTC}It is %H:%M now in UTC')",
            fmttime, fmttime_vrtcmp, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobjs", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t dvobj = purc_dvobj_datetime_new();
    ASSERT_NE(dvobj, nullptr);
    ASSERT_EQ(purc_variant_is_object(dvobj), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        result = eval(test_cases[i].ejson, dvobj);

        /* FIXME: purc_variant_ejson_parse_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(dvobj, test_cases[i].name);

            if (purc_variant_get_type(result) != purc_variant_get_type(expected)) {
                purc_log_error("result type: %s, error message: %s\n",
                        purc_variant_typename(purc_variant_get_type(result)),
                        purc_get_error_message(purc_get_last_error()));
            }

            if (test_cases[i].vrtcmp) {
                if (test_cases[i].vrtcmp(result, expected) != true) {
                    PURC_VARIANT_SAFE_CLEAR(result);
                    result = eval(test_cases[i].ejson, dvobj);
                }
                ASSERT_EQ(test_cases[i].vrtcmp(result, expected), true);
            }
            else {
                ASSERT_EQ(purc_variant_is_equal_to(result, expected), true);
            }

            if (test_cases[i].errcode) {
                ASSERT_EQ(purc_get_last_error(), test_cases[i].errcode);
            }

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }

    purc_variant_unref(dvobj);
    purc_cleanup();
}

static bool result_is_object(purc_variant_t result, purc_variant_t expected)
{
    (void)expected;

    return purc_variant_is_object(result);
}

TEST(dvobjs, broken_down_time)
{
    static const struct ejson_result test_cases[] = {
        { "bad",
            "$DATETIME.utctime(false)",
            fmttime, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$DATETIME.localtime(false)",
            fmttime, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$DATETIME.localtime(null, false)",
            fmttime, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "bad",
            "$DATETIME.mktime",
            fmttime, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$DATETIME.mktime()",
            fmttime, NULL, PURC_ERROR_ARGUMENT_MISSED },
        { "bad",
            "$DATETIME.mktime(false)",
            fmttime, NULL, PURC_ERROR_WRONG_DATA_TYPE },
        { "object",
            "$DATETIME.localtime",
            fmttime, result_is_object, 0 },
        { "object",
            "$DATETIME.localtime(null)",
            fmttime, result_is_object, 0 },
        { "object",
            "$DATETIME.localtime(null, 'America/New_York')",
            fmttime, result_is_object, 0 },
        { "object",
            "$DATETIME.localtime(0, 'America/New_York')",
            fmttime, result_is_object, 0 },
        { "iso8601",
            "$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S%z', null)",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601",
            "$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.localtime)",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601",
            "$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.localtime())",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-timezone",
            "$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.localtime(null, 'America/New_York'))",
            fmttime, fmttime_vrtcmp, 0 },
        { "{UTC}It is %H:%M now in UTC",
            "$DATETIME.fmtbdtime('{UTC}It is %H:%M now in UTC', $DATETIME.utctime())",
            fmttime, fmttime_vrtcmp, 0 },
        { "{UTC}It is %H:%M now in UTC",
            "$DATETIME.fmtbdtime('{UTC}It is %H:%M now in UTC', $DATETIME.utctime(null))",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-epoch-utc",
            "$DATETIME.fmtbdtime('{UTC}%Y-%m-%dT%H:%M:%S%z', $DATETIME.utctime(0))",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-before-epoch-utc",
            "$DATETIME.fmtbdtime('{UTC}%Y-%m-%dT%H:%M:%S%z', $DATETIME.utctime(-3600))",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601",
            "$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S%z', null)",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-epoch",
            "$DATETIME.fmttime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.mktime($DATETIME.utctime(0)))",
            fmttime, fmttime_vrtcmp, 0 },
        { "{UTC}It is %H:%M now in UTC",
            "$DATETIME.fmttime('{UTC}It is %H:%M now in UTC', $DATETIME.mktime($DATETIME.utctime(null)))",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-epoch",
            "$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.localtime(0))",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-before-epoch",
            "$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.localtime(-3600))",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-epoch-timezone",
            "$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.localtime(0, 'America/New_York'))",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-before-epoch-timezone",
            "$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.localtime(-3600, 'America/New_York'))",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601",
            "$DATETIME.fmttime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.mktime($DATETIME.localtime(null)))",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-timezone",
            "$DATETIME.fmttime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.mktime($DATETIME.localtime(null, 'America/New_York')), 'America/New_York')",
            fmttime, fmttime_vrtcmp, 0 },
        { "iso8601-epoch-timezone",
            "$DATETIME.fmttime('%Y-%m-%dT%H:%M:%S%z', $DATETIME.mktime($DATETIME.localtime(0, 'America/New_York')), 'America/New_York')",
            fmttime, fmttime_vrtcmp, 0 },
    };

    int ret = purc_init_ex(PURC_MODULE_EJSON, "cn.fmsfot.hvml.test",
            "dvobjs", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t dvobj = purc_dvobj_datetime_new();
    ASSERT_NE(dvobj, nullptr);
    ASSERT_EQ(purc_variant_is_object(dvobj), true);

    for (size_t i = 0; i < PCA_TABLESIZE(test_cases); i++) {
        purc_variant_t result, expected;

        purc_log_info("evalute: %s\n", test_cases[i].ejson);

        result = eval(test_cases[i].ejson, dvobj);

        /* FIXME: purc_variant_ejson_parse_tree_evalute should not return NULL
           when evaluating silently */
        ASSERT_NE(result, nullptr);

        if (test_cases[i].expected) {
            expected = test_cases[i].expected(dvobj, test_cases[i].name);

            if (purc_variant_get_type(result) != purc_variant_get_type(expected)) {
                purc_log_error("result type: %s, error message: %s\n",
                        purc_variant_typename(purc_variant_get_type(result)),
                        purc_get_error_message(purc_get_last_error()));
            }

            if (test_cases[i].vrtcmp) {
                if (test_cases[i].vrtcmp(result, expected) != true) {
                    PURC_VARIANT_SAFE_CLEAR(result);
                    result = eval(test_cases[i].ejson, dvobj);
                }
                ASSERT_EQ(test_cases[i].vrtcmp(result, expected), true);
            }
            else {
                ASSERT_EQ(purc_variant_is_equal_to(result, expected), true);
            }

            if (test_cases[i].errcode) {
                ASSERT_EQ(purc_get_last_error(), test_cases[i].errcode);
            }

            purc_variant_unref(expected);
        }
        else {
            ASSERT_EQ(purc_variant_get_type(result), PURC_VARIANT_TYPE_NULL);
        }

        purc_variant_unref(result);
    }

    purc_variant_unref(dvobj);
    purc_cleanup();
}

