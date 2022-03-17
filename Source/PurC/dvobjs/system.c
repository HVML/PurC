/*
 * @file system.c
 * @author Geng Yue, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of SYSTEM dynamic variant object.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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
#include "config.h"
#include "helper.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/atom-buckets.h"
#include "private/dvobjs.h"

#include "purc-variant.h"
#include "purc-dvobjs.h"
#include "purc-version.h"

#include <locale.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#include <sys/utsname.h>
#include <sys/time.h>

#define LEN_INI_PRINT_BUF   128
#define LEN_MAX_PRINT_BUF   1024
#define LEN_MAX_KEYWORD     64

enum {
#define SYSTEM_KEYWORD_HVML_SPEC_VERSION    "HVML_SPEC_VERSION"
    K_SYSTEM_KEYWORD_HVML_SPEC_VERSION,
#define SYSTEM_KEYWORD_HVML_SPEC_RELEASE    "HVML_SPEC_RELEASE"
    K_SYSTEM_KEYWORD_HVML_SPEC_RELEASE,
#define SYSTEM_KEYWORD_HVML_PREDEF_VARS_SPEC_VERSION    "HVML_PREDEF_VARS_SPEC_VERSION"
    K_SYSTEM_KEYWORD_HVML_PREDEF_VARS_SPEC_VERSION,
#define SYSTEM_KEYWORD_HVML_PREDEF_VARS_SPEC_RELEASE    "HVML_PREDEF_VARS_SPEC_RELEASE"
    K_SYSTEM_KEYWORD_HVML_PREDEF_VARS_SPEC_RELEASE,
#define SYSTEM_KEYWORD_HVML_INTRPR_NAME     "HVML_INTRPR_NAME"
    K_SYSTEM_KEYWORD_HVML_INTRPR_NAME,
#define SYSTEM_KEYWORD_HVML_INTRPR_VERSION  "HVML_INTRPR_VERSION"
    K_SYSTEM_KEYWORD_HVML_INTRPR_VERSION,
#define SYSTEM_KEYWORD_HVML_INTRPR_RELEASE  "HVML_INTRPR_RELEASE"
    K_SYSTEM_KEYWORD_HVML_INTRPR_RELEASE,
#define SYSTEM_KEYWORD_all                  "all"
    K_SYSTEM_KEYWORD_all,
#define SYSTEM_KEYWORD_default              "default"
    K_SYSTEM_KEYWORD_default,
#define SYSTEM_KEYWORD_kernel_name          "kernel-name"
    K_SYSTEM_KEYWORD_kernel_name,
#define SYSTEM_KEYWORD_kernel_release       "kernel-release"
    K_SYSTEM_KEYWORD_kernel_release,
#define SYSTEM_KEYWORD_kernel_version       "kernel-version"
    K_SYSTEM_KEYWORD_kernel_version,
#define SYSTEM_KEYWORD_nodename             "nodename"
    K_SYSTEM_KEYWORD_nodename,
#define SYSTEM_KEYWORD_machine              "machine"
    K_SYSTEM_KEYWORD_machine,
#define SYSTEM_KEYWORD_processor            "processor"
    K_SYSTEM_KEYWORD_processor,
#define SYSTEM_KEYWORD_hardware_platform    "hardware-platform"
    K_SYSTEM_KEYWORD_hardware_platform,
#define SYSTEM_KEYWORD_operating_system     "operating-system"
    K_SYSTEM_KEYWORD_operating_system,
#define SYSTEM_KEYWORD_ctype                "ctype"
    K_SYSTEM_KEYWORD_ctype,
#define SYSTEM_KEYWORD_numeric              "numeric"
    K_SYSTEM_KEYWORD_numeric,
#define SYSTEM_KEYWORD_time                 "time"
    K_SYSTEM_KEYWORD_time,
#define SYSTEM_KEYWORD_collate              "collate"
    K_SYSTEM_KEYWORD_collate,
#define SYSTEM_KEYWORD_monetary             "monetary"
    K_SYSTEM_KEYWORD_monetary,
#define SYSTEM_KEYWORD_messsages            "messsages"
    K_SYSTEM_KEYWORD_messsages,
#define SYSTEM_KEYWORD_paper                "paper"
    K_SYSTEM_KEYWORD_paper,
#define SYSTEM_KEYWORD_name                 "name"
    K_SYSTEM_KEYWORD_name,
#define SYSTEM_KEYWORD_address              "address"
    K_SYSTEM_KEYWORD_address,
#define SYSTEM_KEYWORD_telephone            "telephone"
    K_SYSTEM_KEYWORD_telephone,
#define SYSTEM_KEYWORD_measurement          "measurement"
    K_SYSTEM_KEYWORD_measurement,
#define SYSTEM_KEYWORD_identification       "identification"
    K_SYSTEM_KEYWORD_identification,
};

static struct keyword_to_atom {
    const char *keyword;
    purc_atom_t atom;
} keywords2atoms [] = {
    { SYSTEM_KEYWORD_HVML_SPEC_VERSION, 0 },      // "HVML_SPEC_VERSION"
    { SYSTEM_KEYWORD_HVML_SPEC_RELEASE, 0 },      // "HVML_SPEC_RELEASE"
    { SYSTEM_KEYWORD_HVML_PREDEF_VARS_SPEC_VERSION, 0 }, // "HVML_PREDEF_VARS_SPEC_VERSION"
    { SYSTEM_KEYWORD_HVML_PREDEF_VARS_SPEC_RELEASE, 0 }, // "HVML_PREDEF_VARS_SPEC_RELEASE"
    { SYSTEM_KEYWORD_HVML_INTRPR_NAME, 0 },       // "HVML_INTRPR_NAME"
    { SYSTEM_KEYWORD_HVML_INTRPR_VERSION, 0 },    // "HVML_INTRPR_VERSION"
    { SYSTEM_KEYWORD_HVML_INTRPR_RELEASE, 0 },    // "HVML_INTRPR_RELEASE"
    { SYSTEM_KEYWORD_all, 0 },                    // "all"
    { SYSTEM_KEYWORD_default, 0 },                // "default"
    { SYSTEM_KEYWORD_kernel_name, 0 },            // "kernel-name"
    { SYSTEM_KEYWORD_kernel_release, 0 },         // "kernel-release"
    { SYSTEM_KEYWORD_kernel_version, 0 },         // "kernel-version"
    { SYSTEM_KEYWORD_nodename, 0 },               // "nodename"
    { SYSTEM_KEYWORD_machine, 0 },                // "machine"
    { SYSTEM_KEYWORD_processor, 0 },              // "processor"
    { SYSTEM_KEYWORD_hardware_platform, 0 },      // "hardware-platform"
    { SYSTEM_KEYWORD_operating_system, 0 },       // "operating-system"
    { SYSTEM_KEYWORD_ctype, 0 },                  // "ctype"
    { SYSTEM_KEYWORD_numeric, 0 },                // "numeric"
    { SYSTEM_KEYWORD_time, 0 },                   // "time"
    { SYSTEM_KEYWORD_collate, 0 },                // "collate"
    { SYSTEM_KEYWORD_monetary, 0 },               // "monetary"
    { SYSTEM_KEYWORD_messsages, 0 },              // "messsages"
    { SYSTEM_KEYWORD_paper, 0 },                  // "paper"
    { SYSTEM_KEYWORD_name, 0 },                   // "name"
    { SYSTEM_KEYWORD_address, 0 },                // "address"
    { SYSTEM_KEYWORD_telephone, 0 },              // "telephone"
    { SYSTEM_KEYWORD_measurement, 0 },            // "measurement"
    { SYSTEM_KEYWORD_identification, 0 },         // "identification"
};

static purc_variant_t
const_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    const char *name;
    purc_atom_t atom;

    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if ((name = purc_variant_get_string_const(argv[0])) == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if ((atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, name)) == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;
    if (atom == keywords2atoms[K_SYSTEM_KEYWORD_HVML_SPEC_VERSION].atom)
        retv = purc_variant_make_string_static(HVML_SPEC_VERSION, false);
    else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_HVML_SPEC_RELEASE].atom)
        retv = purc_variant_make_string_static(HVML_SPEC_RELEASE, false);
    else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_HVML_PREDEF_VARS_SPEC_VERSION].atom)
        retv = purc_variant_make_string_static(HVML_PREDEF_VARS_SPEC_VERSION, false);
    else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_HVML_PREDEF_VARS_SPEC_RELEASE].atom)
        retv = purc_variant_make_string_static(HVML_PREDEF_VARS_SPEC_RELEASE, false);
    else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_HVML_INTRPR_NAME].atom)
        retv = purc_variant_make_string_static(HVML_INTRPR_NAME, false);
    else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_HVML_INTRPR_VERSION].atom)
        retv = purc_variant_make_string_static(HVML_INTRPR_VERSION, false);
    else if (keywords2atoms[K_SYSTEM_KEYWORD_HVML_INTRPR_RELEASE].atom)
        retv = purc_variant_make_string_static(HVML_INTRPR_RELEASE, false);
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    return retv;

failed:
    if (silently)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

#if OS(HYBRIDOS)
#define _OS_NAME    "HybridOS"
#elif OS(AIX)
#define _OS_NAME    "AIX"
#elif OS(IOS_FAMILY)
#define _OS_NAME    "iOS Family"
#elif OS(IOS)
#define _OS_NAME    "iOS"
#elif OS(TVOS)
#define _OS_NAME    "tvOS"
#elif OS(WATCHOS)
#define _OS_NAME    "watchOS"
#elif OS(MAC_OS_X)
#define _OS_NAME    "macOS"
#elif OS(DARWIN)
#define _OS_NAME    "Darwin"
#elif OS(FREEBSD)
#define _OS_NAME    "FreeBSD"
#elif OS(FUCHSIA)
#define _OS_NAME    "Fuchsia"
#elif OS(HURD)
#define _OS_NAME    "GNU/Hurd"
#elif OS(LINUX)
#define _OS_NAME    "GNU/Linux"
#elif OS(NETBSD)
#define _OS_NAME    "NetBSD"
#elif OS(OPENBSD)
#define _OS_NAME    "OpenBSD"
#elif OS(WINDOWS)
#define _OS_NAME    "Windows"
#elif OS(UNIX)
#define _OS_NAME    "UNIX"
#else
#define _OS_NAME    "UnknowOS"
#endif

static purc_variant_t
uname_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct utsname name;
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    if (uname(&name) < 0) {
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    // create an empty object
    retv = purc_variant_make_object (0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    val = purc_variant_make_string(name.sysname, true);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYWORD_kernel_name, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string(name.nodename, true);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYWORD_nodename, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string(name.release, true);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYWORD_kernel_release, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string(name.version, true);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYWORD_kernel_version, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string(name.machine, true);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYWORD_machine, val))
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYWORD_processor, val))
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYWORD_hardware_platform, val))
        goto fatal;
    purc_variant_unref(val);

    /* FIXME: How to get the name of operating system? */
    val = purc_variant_make_string_static(_OS_NAME, false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYWORD_operating_system, val))
        goto fatal;
    purc_variant_unref(val);

    return retv;

fatal:
    silently = false;

failed:
    if (val)
        purc_variant_unref (val);
    if (retv)
        purc_variant_unref (retv);

    if (silently)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

#define _KW_DELIMITERS  " \t\n\v\f\r"

static purc_variant_t
uname_prt_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);

    struct utsname name;
    const char *parts = NULL;

    if (nr_args > 0) {
        parts = purc_variant_get_string_const(argv[0]);
        if (parts == NULL) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (!pcutils_is_meaningful_string(parts))
            parts = SYSTEM_KEYWORD_default;
    }
    else {
        parts = SYSTEM_KEYWORD_default;
    }

    if (uname(&name) < 0) {
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    purc_rwstream_t rwstream;
    rwstream = purc_rwstream_new_buffer(LEN_INI_PRINT_BUF, LEN_MAX_PRINT_BUF);
    if (rwstream == NULL)
        goto fatal;

    size_t nr_wrotten = 0;
    size_t length = 0;
    const char *head = pcutils_get_next_token(parts, _KW_DELIMITERS, &length);
    do {
        purc_atom_t atom;
        size_t len_part = 0;

        if (length == 0 || length > LEN_MAX_KEYWORD) {
            atom = keywords2atoms[K_SYSTEM_KEYWORD_kernel_name].atom;
        }
        else {
            /* TODO: use strndupa if it is available */
            char *part = strndup(head, length);
            atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, part);
            free(part);
        }

        if (atom == keywords2atoms[K_SYSTEM_KEYWORD_all].atom) {
            nr_wrotten = 0;
            purc_rwstream_seek(rwstream, 0, SEEK_SET);

            // kernel-name
            len_part = strlen(name.sysname);
            purc_rwstream_write(rwstream, name.sysname, len_part);
            purc_rwstream_write(rwstream, " ", 1);
            nr_wrotten += len_part + 1;

            // nodename
            len_part = strlen(name.nodename);
            purc_rwstream_write(rwstream, name.nodename, len_part);
            purc_rwstream_write(rwstream, " ", 1);
            nr_wrotten += len_part + 1;

            // kernel-release
            len_part = strlen(name.release);
            purc_rwstream_write(rwstream, name.release, len_part);
            purc_rwstream_write(rwstream, " ", 1);
            nr_wrotten += len_part + 1;

            // kernel-version
            len_part = strlen(name.version);
            purc_rwstream_write(rwstream, name.version, len_part);
            purc_rwstream_write(rwstream, " ", 1);
            nr_wrotten += len_part + 1;

            // machine
            len_part = strlen(name.machine);
            purc_rwstream_write(rwstream, name.machine, len_part);
            purc_rwstream_write(rwstream, " ", 1);
            nr_wrotten += len_part + 1;

            // TODO: processor
            purc_rwstream_write(rwstream, name.machine, len_part);
            purc_rwstream_write(rwstream, " ", 1);
            nr_wrotten += len_part + 1;

            // TODO: hardware-platform
            purc_rwstream_write(rwstream, name.machine, len_part);
            purc_rwstream_write(rwstream, " ", 1);
            nr_wrotten += len_part + 1;

            // operating-system
            len_part = sizeof(_OS_NAME) - 1;
            purc_rwstream_write(rwstream, _OS_NAME, len_part);
            nr_wrotten += len_part;
            break;
        }
        else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_default].atom) {
            purc_rwstream_seek(rwstream, 0, SEEK_SET);

            // kernel-name
            nr_wrotten = strlen(name.sysname);
            purc_rwstream_write(rwstream, name.sysname, nr_wrotten);
            break;
        }
        else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_kernel_name].atom) {
            // kernel-name
            len_part = strlen(name.sysname);
            purc_rwstream_write(rwstream, name.sysname, len_part);
            nr_wrotten += len_part;
        }
        else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_nodename].atom) {
            // nodename
            len_part = strlen(name.nodename);
            purc_rwstream_write(rwstream, name.nodename, len_part);
            nr_wrotten += len_part;
        }
        else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_kernel_release].atom) {
            // kernel-release
            len_part = strlen(name.release);
            purc_rwstream_write(rwstream, name.release, len_part);
            nr_wrotten += len_part;
        }
        else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_kernel_version].atom) {
            // kernel-version
            len_part = strlen(name.version);
            purc_rwstream_write(rwstream, name.version, len_part);
            nr_wrotten += len_part;
        }
        else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_machine].atom) {
            // machine
            len_part = strlen(name.machine);
            purc_rwstream_write(rwstream, name.machine, len_part);
            nr_wrotten += len_part;
        }
        else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_processor].atom) {
            // processor
            len_part = strlen(name.machine);
            purc_rwstream_write(rwstream, name.machine, len_part);
            nr_wrotten += len_part;
        }
        else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_hardware_platform].atom) {
            // hardware-platform
            len_part = strlen(name.machine);
            purc_rwstream_write(rwstream, name.machine, len_part);
            nr_wrotten += len_part;
        }
        else if (atom == keywords2atoms[K_SYSTEM_KEYWORD_operating_system].atom) {
            // operating-system
            len_part = sizeof(_OS_NAME) - 1;
            purc_rwstream_write(rwstream, _OS_NAME, len_part);
            nr_wrotten += len_part;
        }
        else {
            // invalid part name
            len_part = 0;
        }

        if (len_part > 0) {
            purc_rwstream_write(rwstream, " ", 1);
            nr_wrotten++;
        }

        head = pcutils_get_next_token(head + length, _KW_DELIMITERS, &length);
    } while (head);

    purc_rwstream_write(rwstream, "\0", 1);

    size_t sz_buffer = 0;
    size_t sz_content = 0;
    char *content = NULL;
    content = purc_rwstream_get_mem_buffer_ex(rwstream,
            &sz_content, &sz_buffer, true);
    purc_rwstream_destroy(rwstream);

    if (nr_wrotten == 0) {
        free(content);
        return purc_variant_make_string_static("", false);
    }
    else if (content[nr_wrotten - 1] == ' ') {
        content[nr_wrotten - 1] = '\0';
    }

    return purc_variant_make_string_reuse_buff(content, sz_buffer, false);

failed:
    if (silently)
        return purc_variant_make_string_static("", false);

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
time_getter(purc_variant_t root,size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    time_t t_time;
    t_time = time(NULL);
    return purc_variant_make_ulongint((uint64_t)t_time);
}

static purc_variant_t
time_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    struct timeval timeval;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    switch(purc_variant_get_type(argv[0])) {
        case PURC_VARIANT_TYPE_NUMBER:
        {
            double time_d, sec_d, usec_d;

            purc_variant_cast_to_number(argv[0], &time_d, false);
            if (isfinite(time_d) || isnan(time_d) || time_d < 0.0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            usec_d = modf(time_d, &sec_d);
            timeval.tv_sec = (time_t)sec_d;
            timeval.tv_usec = (suseconds_t)(usec_d * 1000000.0);
            break;
        }

        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        {
            long double time_d, sec_d, usec_d;
            purc_variant_cast_to_long_double(argv[0], &time_d, false);

            if (isfinite(time_d) || isnan(time_d) || time_d < 0.0L) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            usec_d = modfl(time_d, &sec_d);
            timeval.tv_sec = (time_t)sec_d;
            timeval.tv_usec = (suseconds_t)(usec_d * 1000000.0);
            break;
        }

        default:
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
    }

    if (settimeofday(&timeval, NULL)) {
        if (errno == EINVAL) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
        }
        else if (errno == EPERM) {
            purc_set_error(PURC_ERROR_ACCESS_DENIED);
        }
        else {
            purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        }

        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (silently)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

#define SYSTEM_KEYNAME_sec   "sec"
#define SYSTEM_KEYNAME_usec  "usec"

static purc_variant_t
time_us_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    // create an empty object
    purc_variant_t retv = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);

    purc_variant_t val = purc_variant_make_ulongint((uint64_t)tv.tv_sec);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYNAME_sec, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_ulongint((uint64_t)tv.tv_usec);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                SYSTEM_KEYNAME_usec, val))
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

static purc_variant_t
time_us_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(silently);

    uint64_t ul_sec, ul_usec;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_cast_to_ulongint(argv[0], &ul_sec, false) ||
            !purc_variant_cast_to_ulongint(argv[1], &ul_usec, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (ul_usec > 999999) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    struct timeval timeval;
    timeval.tv_sec = (time_t)ul_sec;
    timeval.tv_usec = (suseconds_t)ul_usec;
    if (settimeofday(&timeval, NULL)) {
        if (errno == EINVAL) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
        }
        else if (errno == EPERM) {
            purc_set_error(PURC_ERROR_ACCESS_DENIED);
        }
        else {
            purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        }

        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (silently)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
locale_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    char *locale = NULL;
    size_t length = 0;
    purc_variant_t retv = PURC_VARIANT_INVALID;

    if ((nr_args != 0) && (argv == NULL)) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if ((nr_args == 1) && ((argv[0] == PURC_VARIANT_INVALID) ||
                (!purc_variant_is_string (argv[0])))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (nr_args) {
        const char *option = purc_variant_get_string_const (argv[0]);
        const char *head = pcutils_get_next_token (option, " ", &length);

        while (head) {
            switch (*head) {
                case 'c':
                case 'C':
                    if (strncasecmp (head, LOCALE_CTYPE, length) == 0) {
                        locale = setlocale (LC_CTYPE, "");
                    }
                    else if (strncasecmp (head, LOCALE_COLLATE,
                                length) == 0) {
                        locale = setlocale (LC_COLLATE, "");
                    }
                    else
                        goto bad_category;
                    break;

                case 'n':
                case 'N':
                    if (strncasecmp (head, LOCALE_NUMERIC, length) == 0) {
                        locale = setlocale (LC_NUMERIC, "");
                    }
#ifdef LC_NAME
                    else if (strncasecmp (head, LOCALE_NAME, length) == 0) {
                        locale = setlocale (LC_NAME, "");
                    }
#endif /* LC_NAME */
                    else
                        goto bad_category;
                    break;

                case 't':
                case 'T':
                    if (strncasecmp (head, LOCALE_TIME, length) == 0) {
                        locale = setlocale (LC_TIME, "");
                    }
#ifdef LC_TELEPHONE
                    else if (strncasecmp (head, LOCALE_TELEPHONE,
                                length) == 0) {
                        locale = setlocale (LC_TELEPHONE, "");
                    }
#endif /* LC_TELEPHONE */
                    else
                        goto bad_category;
                    break;

                case 'm':
                case 'M':
                    if (strncasecmp (head, LOCALE_MONETARY, length) == 0) {
                        locale = setlocale (LC_MONETARY, "");
                    }
                    else if (strncasecmp (head, LOCALE_MESSAGE,
                                length) == 0) {
                        locale = setlocale (LC_MESSAGES, "");
                    }
#ifdef LC_MEASUREMENT
                    else if (strncasecmp (head, LOCALE_MEASUREMENT,
                                length) == 0) {
                        locale = setlocale (LC_MEASUREMENT, "");
                    }
#endif /* LC_MEASUREMENT */
                    else
                        goto bad_category;

                    break;

#ifdef LC_PAPER
                case 'p':
                case 'P':
                    if (strncasecmp (head, LOCALE_PAPER, length) == 0) {
                        locale = setlocale (LC_PAPER, "");
                    }
                    else
                        goto bad_category;
                    break;
#endif /* LC_PAPER */

#ifdef LC_ADDRESS
                case 'a':
                case 'A':
                    if (strncasecmp (head, LOCALE_ADDRESS, length) == 0) {
                        locale = setlocale (LC_ADDRESS, "");
                    }
                    else
                        goto bad_category;
                    break;
#endif /* LC_ADDRESS */

#ifdef LC_IDENTIFICATION
                case 'i':
                case 'I':
                    if (strncasecmp (head, LOCALE_IDENTIFICATION,
                                length) == 0) {
                        locale = setlocale (LC_IDENTIFICATION, "");
                    }
                    else
                        goto bad_category;
                    break;
#endif /* LC_IDENTIFICATION */

                default:
                    goto bad_category;
                    break;
            }

            head = pcutils_get_next_token (head + length, " ", &length);
        }
    }
    else
        locale = setlocale (LC_MESSAGES, "");

    if (locale) {
        char *end = strchr (locale, '.');
        size_t length = 0;
        if (end)
            length = end - locale;
        else
            length = strlen (locale);
        retv = purc_variant_make_string_ex (locale, length, true);
    }

    if (retv == PURC_VARIANT_INVALID) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    return retv;

bad_category:
    purc_set_error (PURC_ERROR_INVALID_VALUE);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
locale_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    size_t length = 0;
    purc_variant_t retv = PURC_VARIANT_INVALID;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] == PURC_VARIANT_INVALID) ||
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    const char *option = purc_variant_get_string_const (argv[0]);
    const char *head = pcutils_get_next_token (option, " ", &length);

    while (head) {
        switch (*head) {
            case 'a':
            case 'A':
                if (strncasecmp (head, LOCALE_ALL, length) == 0) {
                    if (setlocale (LC_ALL,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
                }
                else if (strncasecmp (head, LOCALE_ADDRESS, length) == 0) {
#ifdef LC_ADDRESS
                    if (setlocale (LC_ADDRESS,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
#else
                    purc_set_error (PURC_ERROR_NOT_SUPPORTED);
                    retv = purc_variant_make_boolean (false);
#endif
                }
                break;

            case 'c':
            case 'C':
                if (strncasecmp (head, LOCALE_CTYPE, length) == 0) {
                    if (setlocale (LC_CTYPE,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
                }
                else if (strncasecmp (head, LOCALE_COLLATE, length) == 0) {
                    if (setlocale (LC_COLLATE,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
                }
                break;

            case 'n':
            case 'N':
                if (strncasecmp (head, LOCALE_NUMERIC, length) == 0) {
                    if (setlocale (LC_NUMERIC,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
                }
                else if (strncasecmp (head, LOCALE_NAME, length) == 0) {
#ifdef LC_NAME
                    if (setlocale (LC_NAME,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
#else
                    purc_set_error (PURC_ERROR_NOT_SUPPORTED);
                    retv = purc_variant_make_boolean (false);
#endif
                }
                break;

            case 't':
            case 'T':
                if (strncasecmp (head, LOCALE_TIME, length) == 0) {
                    if (setlocale (LC_TIME,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
                }
                else if (strncasecmp (head, LOCALE_TELEPHONE, length) == 0) {
#ifdef LC_TELEPHONE
                    if (setlocale (LC_TELEPHONE,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
#else
                    purc_set_error (PURC_ERROR_NOT_SUPPORTED);
                    retv = purc_variant_make_boolean (false);
#endif
                }
                break;

            case 'm':
            case 'M':
                if (strncasecmp (head, LOCALE_MONETARY, length) == 0) {
                    if (setlocale (LC_MONETARY,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
                }
                else if (strncasecmp (head, LOCALE_MESSAGE, length) == 0) {
                    if (setlocale (LC_MESSAGES,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
                }
                else if (strncasecmp (head, LOCALE_MEASUREMENT,
                            length) == 0) {
#ifdef LC_MEASUREMENT
                    if (setlocale (LC_MEASUREMENT,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
#else
                    purc_set_error (PURC_ERROR_NOT_SUPPORTED);
                    retv = purc_variant_make_boolean (false);
#endif
                }
                break;

            case 'p':
            case 'P':
                if (strncasecmp (head, LOCALE_PAPER, length) == 0) {
#ifdef LC_PAPER
                    if (setlocale (LC_PAPER,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
#else
                    purc_set_error (PURC_ERROR_NOT_SUPPORTED);
                    retv = purc_variant_make_boolean (false);
#endif
                }
                break;

            case 'i':
            case 'I':
                if (strncasecmp (head, LOCALE_IDENTIFICATION, length) == 0) {
#ifdef LC_IDENTIFICATION
                    if (setlocale (LC_IDENTIFICATION,
                                purc_variant_get_string_const (argv[1])))
                        retv = purc_variant_make_boolean (true);
                    else
                        retv = purc_variant_make_boolean (false);
#else
                    purc_set_error (PURC_ERROR_NOT_SUPPORTED);
                    retv = purc_variant_make_boolean (false);
#endif
                }
                break;
        }

        head = pcutils_get_next_token (head + length, " ", &length);
    }

    return retv;
}

static purc_variant_t
timezone_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    time_t t_time;
    t_time = time (NULL);
    return purc_variant_make_ulongint((uint64_t)t_time);
}

static purc_variant_t
timezone_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(silently);

    struct timeval timeval;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    switch(purc_variant_get_type(argv[0])) {
        case PURC_VARIANT_TYPE_NUMBER:
        {
            double time_d, sec_d, usec_d;

            purc_variant_cast_to_number(argv[0], &time_d, false);
            if (isfinite(time_d) || isnan(time_d) || time_d < 0.0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            usec_d = modf(time_d, &sec_d);
            timeval.tv_sec = (time_t)sec_d;
            timeval.tv_usec = (suseconds_t)(usec_d * 1000000.0);
            break;
        }

        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        {
            long double time_d, sec_d, usec_d;
            purc_variant_cast_to_long_double(argv[0], &time_d, false);

            if (isfinite(time_d) || isnan(time_d) || time_d < 0.0L) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            usec_d = modfl(time_d, &sec_d);
            timeval.tv_sec = (time_t)sec_d;
            timeval.tv_usec = (suseconds_t)(usec_d * 1000000.0);
            break;
        }

        default:
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
    }

    if (settimeofday(&timeval, NULL)) {
        if (errno == EINVAL) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
        }
        else if (errno == EPERM) {
            purc_set_error(PURC_ERROR_ACCESS_DENIED);
        }
        else {
            purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        }

        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (silently)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}


static purc_variant_t
cwd_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    static const size_t sz_alloc = PATH_MAX * 2 + 1;

    char buf[PATH_MAX + 1];
    char *cwd;
    if (getcwd(buf, sizeof(buf)) == NULL) {
        cwd = malloc(sz_alloc);
        if (cwd == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        if (getcwd(cwd, sz_alloc) == NULL) {
            free(cwd);
            purc_set_error(PURC_ERROR_TOO_LARGE_ENTITY);
            goto failed;
        }

    }
    else {
        cwd = buf;
    }

    if (cwd == buf) {
        return purc_variant_make_string(cwd, true);
    }
    else {
        return purc_variant_make_string_reuse_buff(cwd, sz_alloc, true);
    }

failed:
    if (silently)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
cwd_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    const char *path;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if ((path = purc_variant_get_string_const(argv[0])) == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (chdir(path)) {
        int errcode;
        switch (errno) {
        case ENOTDIR:
            errcode = PURC_ERROR_NOT_DESIRED_ENTITY;
            break;

        case EACCES:
            errcode = PURC_ERROR_ACCESS_DENIED;
            break;

        case ENOENT:
            errcode = PURC_ERROR_NOT_EXISTS;
            break;

        case ELOOP:
            errcode = PURC_ERROR_TOO_MANY;
            break;

        case ENAMETOOLONG:
            errcode = PURC_ERROR_TOO_LARGE_ENTITY;
            break;

        case ENOMEM:
            errcode = PURC_ERROR_OUT_OF_MEMORY;
            break;

        default:
            errcode = PURC_ERROR_BAD_SYSTEM_CALL;
            break;
        }

        purc_set_error(errcode);
        goto failed;
    }

    // TODO: broadcast "change:cwd" event
    return purc_variant_make_boolean(true);

failed:
    if (silently)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
env_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *name = purc_variant_get_string_const(argv[0]);
    if (name == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    char *result = getenv(name);
    if (result)
        return purc_variant_make_string(result, false);

    purc_set_error(PURC_ERROR_NOT_EXISTS);

failed:
    if (silently)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}


static purc_variant_t
env_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *name = purc_variant_get_string_const(argv[0]);
    if (name == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    int ret;
    if (purc_variant_is_undefined(argv[1])) {
        ret = unsetenv(name);
    }
    else {
        const char *value = purc_variant_get_string_const(argv[1]);

        if (value == NULL) {
            purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        ret = setenv(name, value, 1);
    }

    if (ret) {
        int errcode;
        switch (errno) {
        case EINVAL:
            errcode = PURC_ERROR_INVALID_VALUE;
            break;

        case ENOMEM:
            errcode = PURC_ERROR_OUT_OF_MEMORY;
            break;

        default:
            errcode = PURC_ERROR_BAD_SYSTEM_CALL;
            break;
        }

        purc_set_error(errcode);
        goto failed;
    }

    // TODO: broadcast "change:env" event
    return purc_variant_make_boolean(true);

failed:
    if (silently)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

#if OS(LINUX)

#include <sys/random.h>

static purc_variant_t
random_sequence_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);

    char buf[256];

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    uint64_t length;

    if (!purc_variant_cast_to_ulongint(argv[0], &length, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (length == 0 || length > 256) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    ssize_t ret = getrandom(buf, sizeof(buf), GRND_NONBLOCK);
    if (ret < 0) {
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
    }

    return purc_variant_make_byte_sequence(buf, ret);

failed:
    if (silently) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

#else   /* OS(LINUX) */

static purc_variant_t
random_sequence_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (silently) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}
#endif  /* !OS(LINUX) */

purc_variant_t purc_dvobj_system_new (void)
{
    static const struct purc_dvobj_method methods[] = {
        { "const",      const_getter,     NULL },
        { "uname",      uname_getter,     NULL },
        { "uname_prt",  uname_prt_getter, NULL },
        { "time",       time_getter,      time_setter },
        { "time_us",    time_us_getter,   time_us_setter },
        { "locale",     locale_getter,    locale_setter },
        { "timezone",   timezone_getter,  timezone_setter },
        { "cwd",        cwd_getter,       cwd_setter },
        { "env",        env_getter,       env_setter },
        { "random_sequence", random_sequence_getter, NULL }
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

#if 0

#define FORMAT_ISO8601  1
#define FORMAT_RFC822   2

static purc_variant_t
get_time_format (int type, double epoch, const char *timezone)
{
    time_t t_time;
    struct tm *t_tm = NULL;
    char local_time[32] = {0};
    char *tz_now = getenv ("TZ");
    purc_variant_t retv = NULL;
    char str_format[32] = {0};

    if (type == FORMAT_ISO8601)
        sprintf (str_format, "%%FT%%T%%z");
    else if (type == FORMAT_RFC822)
        sprintf (str_format, "%%a, %%d %%b %%y %%T %%z");
    else
        sprintf (str_format, "%%FT%%T%%z");

    if (epoch == 0) {
        if (timezone == NULL) {
            t_time = time (NULL);
            t_tm = localtime(&t_time);
            if (t_tm == NULL) {
                purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if(strftime(local_time, 32, str_format, t_tm) == 0) {
                purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            // create a string variant
            retv = purc_variant_make_string (local_time, false);
            if(retv == PURC_VARIANT_INVALID) {
                purc_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }
        }
        else {
            setenv ("TZ", timezone, 0);
            t_time = time (NULL);
            t_tm = localtime(&t_time);
            if (t_tm == NULL) {
                purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if(strftime(local_time, 32, str_format, t_tm) == 0) {
                purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if (tz_now)
                setenv ("TZ", tz_now, 1);
            else
                unsetenv ("TZ");

            // create a string variant
            retv = purc_variant_make_string (local_time, false);
            if(retv == PURC_VARIANT_INVALID) {
                purc_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }
        }
    }
    else {
        if (timezone == NULL) {
            t_time = epoch;
            t_tm = localtime(&t_time);
            if (t_tm == NULL) {
                purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if(strftime(local_time, 32, str_format, t_tm) == 0) {
                purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            // create a string variant
            retv = purc_variant_make_string (local_time, false);
            if(retv == PURC_VARIANT_INVALID) {
                purc_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }
        }
        else {
            setenv ("TZ", timezone, 0);

            t_time = epoch;
            t_tm = localtime(&t_time);

            if (t_tm == NULL) {
                purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if(strftime(local_time, 32, str_format, t_tm) == 0) {
                purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                return PURC_VARIANT_INVALID;
            }

            if (tz_now)
                setenv ("TZ", tz_now, 1);
            else
                unsetenv ("TZ");

            // create a string variant
            retv = purc_variant_make_string (local_time, false);
            if(retv == PURC_VARIANT_INVALID) {
                purc_set_error (PURC_ERROR_INVALID_VALUE);
                return PURC_VARIANT_INVALID;
            }
        }
    }

    return retv;
}


static purc_variant_t
time_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    double epoch = 0.0;
    const char *name = NULL;
    const char *timezone = NULL;
    time_t t_time;
    struct tm *t_tm = NULL;

    if (nr_args == 0) {
        t_time = time (NULL);
        return purc_variant_make_ulongint ((uint64_t) t_time);
    }

    if ((nr_args >= 1) && (argv[0] == PURC_VARIANT_INVALID ||
                    (!purc_variant_is_string (argv[0])))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    else
        name = purc_variant_get_string_const (argv[0]);

    if ((nr_args >= 2) && (argv[1] == PURC_VARIANT_INVALID ||
            (!((purc_variant_is_ulongint (argv[1]))   ||
               (purc_variant_is_longdouble (argv[1])) ||
               (purc_variant_is_longint (argv[1]))    ||
               (purc_variant_is_number (argv[1])))))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    else if (nr_args >= 2)
        purc_variant_cast_to_number (argv[1], &epoch, false);

    if ((nr_args >= 3) && (argv[2] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string (argv[2])))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    else if (nr_args >= 3)
        timezone = purc_variant_get_string_const (argv[2]);

    if (strcasecmp (name, "tm") == 0) {
        t_time = time (NULL);
        t_tm = localtime(&t_time);

        retv = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                PURC_VARIANT_INVALID);
        if(retv == PURC_VARIANT_INVALID) {
            purc_set_error (PURC_ERROR_INVALID_VALUE);
            return PURC_VARIANT_INVALID;
        }

        val = purc_variant_make_number (t_tm->tm_sec);
        purc_variant_object_set_by_static_ckey (retv, "sec", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_min);
        purc_variant_object_set_by_static_ckey (retv, "min", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_hour);
        purc_variant_object_set_by_static_ckey (retv, "hour", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_mday);
        purc_variant_object_set_by_static_ckey (retv, "mday", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_mon);
        purc_variant_object_set_by_static_ckey (retv, "mon", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_year);
        purc_variant_object_set_by_static_ckey (retv, "year", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_wday);
        purc_variant_object_set_by_static_ckey (retv, "wday", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_yday);
        purc_variant_object_set_by_static_ckey (retv, "yday", val);
        purc_variant_unref (val);

        val = purc_variant_make_number (t_tm->tm_isdst);
        purc_variant_object_set_by_static_ckey (retv, "isdst", val);
        purc_variant_unref (val);
    }
    else if (strcasecmp (name, "iso8601") == 0) {
        retv = get_time_format (FORMAT_ISO8601, epoch, timezone);
    }
    else if (strcasecmp (name, "rfc822") == 0) {
        retv = get_time_format (FORMAT_RFC822, epoch, timezone);
    }
    else {
        /* replace 
           %Y: year
           %m: month
           %d: day
           %H: hour
           %M: minute
           %S: second
         */
        purc_rwstream_t rwstream = purc_rwstream_new_buffer (32, STREAM_SIZE);
        char buffer[16];
        int start = 0;
        int i = 0;
        char *tz_now = getenv ("TZ");

        if (epoch == 0) {
            if (timezone == NULL) {
                t_time = time (NULL);
                t_tm = localtime(&t_time);
                if (t_tm == NULL) {
                    purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                    return PURC_VARIANT_INVALID;
                }
            }
            else {
                setenv ("TZ", timezone, 0);
                t_time = time (NULL);
                t_tm = localtime(&t_time);
                if (t_tm == NULL) {
                    purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                    return PURC_VARIANT_INVALID;
                }
                if (tz_now)
                    setenv ("TZ", tz_now, 1);
                else
                    unsetenv ("TZ");
            }
        }
        else {
            if (timezone == NULL) {
                t_time = epoch;
                t_tm = localtime(&t_time);
                if (t_tm == NULL) {
                    purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                    return PURC_VARIANT_INVALID;
                }
            }
            else {
                setenv ("TZ", timezone, 0);

                t_time = epoch;
                t_tm = localtime(&t_time);

                if (t_tm == NULL) {
                    purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
                    return PURC_VARIANT_INVALID;
                }
                if (tz_now)
                    setenv ("TZ", tz_now, 1);
                else
                    unsetenv ("TZ");

            }
        }

        while (*(name + i) != 0x00) {
            if (*(name + i) == '%') {
                switch (*(name + i + 1)) {
                    case 0x00:
                        break;
                    case '%':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        purc_rwstream_write (rwstream, "%", 1);
                        i++;
                        start = i + 1;
                        break;
                    case 'Y':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_year + 1900);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'm':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_mon + 1);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'd':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_mday);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'H':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_hour);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'M':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_min);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                    case 'S':
                        purc_rwstream_write (rwstream, name + start, i - start);
                        sprintf (buffer, "%d", t_tm->tm_sec);
                        purc_rwstream_write (rwstream, buffer, strlen (buffer));
                        i++;
                        start = i + 1;
                        break;
                }
            }
            i++;
        }

        if (i != start) {
            purc_rwstream_write (rwstream, name + start, strlen (name + start));
            purc_rwstream_write (rwstream, "\0", 1);
        }

        size_t rw_size = 0;
        size_t content_size = 0;
        char *rw_string = purc_rwstream_get_mem_buffer_ex (rwstream,
                                            &content_size, &rw_size, true);

        if ((rw_size == 0) || (rw_string == NULL))
            retv = PURC_VARIANT_INVALID;
        else {
            retv = purc_variant_make_string_reuse_buff (rw_string,
                                                        rw_size, false);
            if(retv == PURC_VARIANT_INVALID) {
                purc_set_error (PURC_ERROR_INVALID_VALUE);
                retv = PURC_VARIANT_INVALID;
            }
        }

        purc_rwstream_destroy (rwstream);

    }

    return retv;
}

static purc_variant_t
random_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    double random = 0.0;
    double number = 0.0;

    if (nr_args == 0) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] == PURC_VARIANT_INVALID) ||
            (!purc_variant_cast_to_number (argv[0], &number, false))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (fabs (number) < 1.0E-10) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    random = number * rand() / (double)(RAND_MAX);

    return purc_variant_make_number (random);
}

#endif

