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
#define _GNU_SOURCE
#include "config.h"
#include "helper.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/atom-buckets.h"
#include "private/dvobjs.h"
#include "private/interpreter.h"

#include "purc-variant.h"
#include "purc-dvobjs.h"
#include "purc-version.h"

#include <locale.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/socket.h>

#define MSG_SOURCE_SYSTEM         PURC_PREDEF_VARNAME_SYS

#define MSG_TYPE_CHANGE           "change"
#define MSG_SUB_TYPE_TIME         "time"
#define MSG_SUB_TYPE_ENV          "env"
#define MSG_SUB_TYPE_CWD          "cwd"

enum {
#define _KW_HVML_SPEC_VERSION   "HVML_SPEC_VERSION"
    K_KW_HVML_SPEC_VERSION,
#define _KW_HVML_SPEC_RELEASE   "HVML_SPEC_RELEASE"
    K_KW_HVML_SPEC_RELEASE,
#define _KW_HVML_PREDEF_VARS_SPEC_VERSION   "HVML_PREDEF_VARS_SPEC_VERSION"
    K_KW_HVML_PREDEF_VARS_SPEC_VERSION,
#define _KW_HVML_PREDEF_VARS_SPEC_RELEASE   "HVML_PREDEF_VARS_SPEC_RELEASE"
    K_KW_HVML_PREDEF_VARS_SPEC_RELEASE,
#define _KW_HVML_INTRPR_NAME    "HVML_INTRPR_NAME"
    K_KW_HVML_INTRPR_NAME,
#define _KW_HVML_INTRPR_VERSION "HVML_INTRPR_VERSION"
    K_KW_HVML_INTRPR_VERSION,
#define _KW_HVML_INTRPR_RELEASE "HVML_INTRPR_RELEASE"
    K_KW_HVML_INTRPR_RELEASE,
#define _KW_all                 "all"
    K_KW_all,
#define _KW_none                "none"
    K_KW_none,
#define _KW_default             "default"
    K_KW_default,
#define _KW_kernel_name         "kernel-name"
    K_KW_kernel_name,
#define _KW_kernel_release      "kernel-release"
    K_KW_kernel_release,
#define _KW_kernel_version      "kernel-version"
    K_KW_kernel_version,
#define _KW_nodename            "nodename"
    K_KW_nodename,
#define _KW_machine             "machine"
    K_KW_machine,
#define _KW_processor           "processor"
    K_KW_processor,
#define _KW_hardware_platform   "hardware-platform"
    K_KW_hardware_platform,
#define _KW_operating_system    "operating-system"
    K_KW_operating_system,
#define _KW_ctype               "ctype"
    K_KW_ctype,
#define _KW_numeric             "numeric"
    K_KW_numeric,
#define _KW_time                "time"
    K_KW_time,
#define _KW_collate             "collate"
    K_KW_collate,
#define _KW_monetary            "monetary"
    K_KW_monetary,
#define _KW_messages            "messages"
    K_KW_messages,
#define _KW_paper               "paper"
    K_KW_paper,
#define _KW_name                "name"
    K_KW_name,
#define _KW_address             "address"
    K_KW_address,
#define _KW_telephone           "telephone"
    K_KW_telephone,
#define _KW_measurement         "measurement"
    K_KW_measurement,
#define _KW_identification      "identification"
    K_KW_identification,
#define _KW_cloexec             "cloexec"
    K_KW_cloexec,
#define _KW_nonblock            "nonblock"
    K_KW_nonblock,
#define _KW_append              "append"
    K_KW_append,
#define _KW_type                "type"
    K_KW_type,
#define _KW_nread               "nread"
    K_KW_nread,
#define _KW_nwrite              "nwrite"
    K_KW_nwrite,
#define _KW_recv_timeout        "recv-timeout"
    K_KW_recv_timeout,
#define _KW_send_timeout        "send-timeout"
    K_KW_send_timeout,
#define _KW_recv_buffer         "recv-buffer"
    K_KW_recv_buffer,
#define _KW_send_buffer         "send-buffer"
    K_KW_send_buffer,
};

static struct keyword_to_atom {
    const char *keyword;
    purc_atom_t atom;
} keywords2atoms [] = {
    { _KW_HVML_SPEC_VERSION, 0 },      // "HVML_SPEC_VERSION"
    { _KW_HVML_SPEC_RELEASE, 0 },      // "HVML_SPEC_RELEASE"
    { _KW_HVML_PREDEF_VARS_SPEC_VERSION, 0 }, // "HVML_PREDEF_VARS_SPEC_VERSION"
    { _KW_HVML_PREDEF_VARS_SPEC_RELEASE, 0 }, // "HVML_PREDEF_VARS_SPEC_RELEASE"
    { _KW_HVML_INTRPR_NAME, 0 },       // "HVML_INTRPR_NAME"
    { _KW_HVML_INTRPR_VERSION, 0 },    // "HVML_INTRPR_VERSION"
    { _KW_HVML_INTRPR_RELEASE, 0 },    // "HVML_INTRPR_RELEASE"
    { _KW_all, 0 },                    // "all"
    { _KW_none, 0 },                   // "none"
    { _KW_default, 0 },                // "default"
    { _KW_kernel_name, 0 },            // "kernel-name"
    { _KW_kernel_release, 0 },         // "kernel-release"
    { _KW_kernel_version, 0 },         // "kernel-version"
    { _KW_nodename, 0 },               // "nodename"
    { _KW_machine, 0 },                // "machine"
    { _KW_processor, 0 },              // "processor"
    { _KW_hardware_platform, 0 },      // "hardware-platform"
    { _KW_operating_system, 0 },       // "operating-system"
    { _KW_ctype, 0 },                  // "ctype"
    { _KW_numeric, 0 },                // "numeric"
    { _KW_time, 0 },                   // "time"
    { _KW_collate, 0 },                // "collate"
    { _KW_monetary, 0 },               // "monetary"
    { _KW_messages, 0 },               // "messages"
    { _KW_paper, 0 },                  // "paper"
    { _KW_name, 0 },                   // "name"
    { _KW_address, 0 },                // "address"
    { _KW_telephone, 0 },              // "telephone"
    { _KW_measurement, 0 },            // "measurement"
    { _KW_identification, 0 },         // "identification"
    { _KW_cloexec, 0 },                // "cloexec"
    { _KW_nonblock, 0 },               // "nonblock"
    { _KW_append, 0 },                 // "append"
    { _KW_type, 0 },                   // "type"
    { _KW_nread, 0 },                  // "nread"
    { _KW_nwrite, 0 },                 // "nwrite"
    { _KW_recv_timeout, 0 },           // "recv-timeout"
    { _KW_send_timeout, 0 },           // "send-timeout"
    { _KW_recv_buffer, 0 },            // "recv-buffer"
    { _KW_send_buffer, 0 },            // "send-buffer"
};

static int
broadcast_event(purc_variant_t source, const char *type, const char *sub_type,
        purc_variant_t data)
{
    UNUSED_PARAM(source);
    struct pcinst* inst = pcinst_current();
    if (!inst->intr_heap) {
        return 0;
    }
    purc_variant_t source_uri = purc_variant_make_string(
            inst->endpoint_name, false);
    purc_variant_t observed = purc_variant_make_string_static(
            MSG_SOURCE_SYSTEM, false);

    int ret = pcinst_broadcast_event(PCRDR_MSG_EVENT_REDUCE_OPT_OVERLAY,
            source_uri, observed, type, sub_type, data);

    purc_variant_unref(source_uri);
    purc_variant_unref(observed);
    return ret;
}

static purc_variant_t
const_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
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
    if (atom == keywords2atoms[K_KW_HVML_SPEC_VERSION].atom)
        retv = purc_variant_make_string_static(HVML_SPEC_VERSION, false);
    else if (atom == keywords2atoms[K_KW_HVML_SPEC_RELEASE].atom)
        retv = purc_variant_make_string_static(HVML_SPEC_RELEASE, false);
    else if (atom == keywords2atoms[K_KW_HVML_PREDEF_VARS_SPEC_VERSION].atom)
        retv = purc_variant_make_string_static(HVML_PREDEF_VARS_SPEC_VERSION, false);
    else if (atom == keywords2atoms[K_KW_HVML_PREDEF_VARS_SPEC_RELEASE].atom)
        retv = purc_variant_make_string_static(HVML_PREDEF_VARS_SPEC_RELEASE, false);
    else if (atom == keywords2atoms[K_KW_HVML_INTRPR_NAME].atom)
        retv = purc_variant_make_string_static(HVML_INTRPR_NAME, false);
    else if (atom == keywords2atoms[K_KW_HVML_INTRPR_VERSION].atom)
        retv = purc_variant_make_string_static(HVML_INTRPR_VERSION, false);
    else if (keywords2atoms[K_KW_HVML_INTRPR_RELEASE].atom)
        retv = purc_variant_make_string_static(HVML_INTRPR_RELEASE, false);
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    return retv;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
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
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    bool silently = call_flags & PCVRT_CALL_FLAG_SILENTLY;
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
                _KW_kernel_name, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string(name.nodename, true);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                _KW_nodename, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string(name.release, true);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                _KW_kernel_release, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string(name.version, true);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                _KW_kernel_version, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string(name.machine, true);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                _KW_machine, val))
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                _KW_processor, val))
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                _KW_hardware_platform, val))
        goto fatal;
    purc_variant_unref(val);

    /* FIXME: How to get the name of operating system? */
    val = purc_variant_make_string_static(_OS_NAME, false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                _KW_operating_system, val))
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
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    struct utsname name;
    const char *parts;
    size_t parts_len;
    purc_atom_t atom = 0;

    if (nr_args > 0) {
        parts = purc_variant_get_string_const_ex(argv[0], &parts_len);
        if (parts == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        parts = pcutils_trim_spaces(parts, &parts_len);
        if (parts_len == 0) {
            parts = _KW_default;
            parts_len = sizeof(_KW_default) - 1;
            atom = keywords2atoms[K_KW_default].atom;
        }
    }
    else {
        parts = _KW_default;
        parts_len = sizeof(_KW_default) - 1;
        atom = keywords2atoms[K_KW_default].atom;
    }

    if (uname(&name) < 0) {
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    purc_rwstream_t rwstream;
    rwstream = purc_rwstream_new_buffer(LEN_INI_PRINT_BUF, LEN_MAX_PRINT_BUF);
    if (rwstream == NULL)
        goto fatal;

    if (atom == 0) {
        char *tmp = strndup(parts, parts_len);
        atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
        free(tmp);
    }

    size_t nr_wrotten = 0;
    if (atom == keywords2atoms[K_KW_all].atom) {
        size_t len_part = 0;

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

#if OS(LINUX)
        // TODO: processor for macOS
        purc_rwstream_write(rwstream, name.machine, len_part);
        purc_rwstream_write(rwstream, " ", 1);
        nr_wrotten += len_part + 1;

        // TODO: hardware-platform for macOS
        purc_rwstream_write(rwstream, name.machine, len_part);
        purc_rwstream_write(rwstream, " ", 1);
        nr_wrotten += len_part + 1;

        // operating-system for macOS
        len_part = sizeof(_OS_NAME) - 1;
        purc_rwstream_write(rwstream, _OS_NAME, len_part);
        nr_wrotten += len_part;
#endif
    }
    else if (atom == keywords2atoms[K_KW_default].atom) {
        // kernel-name
        nr_wrotten = strlen(name.sysname);
        purc_rwstream_write(rwstream, name.sysname, nr_wrotten);
    }
    else {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                _KW_DELIMITERS, &length);
        do {
            size_t len_part = 0;

            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = keywords2atoms[K_KW_kernel_name].atom;
            }
            else {
#if 0
                /* TODO: use strndupa if it is available */
                char *tmp = strndup(part, length);
                atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
                free(tmp);
#else
                char tmp[length + 1];
                strncpy(tmp, part, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
#endif
            }

            if (atom == keywords2atoms[K_KW_kernel_name].atom) {
                // kernel-name
                len_part = strlen(name.sysname);
                purc_rwstream_write(rwstream, name.sysname, len_part);
                nr_wrotten += len_part;
            }
            else if (atom == keywords2atoms[K_KW_nodename].atom) {
                // nodename
                len_part = strlen(name.nodename);
                purc_rwstream_write(rwstream, name.nodename, len_part);
                nr_wrotten += len_part;
            }
            else if (atom == keywords2atoms[K_KW_kernel_release].atom) {
                // kernel-release
                len_part = strlen(name.release);
                purc_rwstream_write(rwstream, name.release, len_part);
                nr_wrotten += len_part;
            }
            else if (atom == keywords2atoms[K_KW_kernel_version].atom) {
                // kernel-version
                len_part = strlen(name.version);
                purc_rwstream_write(rwstream, name.version, len_part);
                nr_wrotten += len_part;
            }
            else if (atom == keywords2atoms[K_KW_machine].atom) {
                // machine
                len_part = strlen(name.machine);
                purc_rwstream_write(rwstream, name.machine, len_part);
                nr_wrotten += len_part;
            }
            else if (atom == keywords2atoms[K_KW_processor].atom) {
                // processor
                len_part = strlen(name.machine);
                purc_rwstream_write(rwstream, name.machine, len_part);
                nr_wrotten += len_part;
            }
            else if (atom == keywords2atoms[K_KW_hardware_platform].atom) {
                // hardware-platform
                len_part = strlen(name.machine);
                purc_rwstream_write(rwstream, name.machine, len_part);
                nr_wrotten += len_part;
            }
            else if (atom == keywords2atoms[K_KW_operating_system].atom) {
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

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    _KW_DELIMITERS, &length);
        } while (part);
    }

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
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_string_static("", false);

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
time_getter(purc_variant_t root,size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    time_t t_time;
    t_time = time(NULL);
    return purc_variant_make_longint((int64_t)t_time);
}

static purc_variant_t
time_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    // NOTE: initialize with {} to prevent `uninitialized ...` error
    // from being reported by valgrind
    struct timeval timeval = {};

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!dvobjs_cast_to_timeval(&timeval, argv[0])) {
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

    // broadcast "change:time" event
    broadcast_event(root, MSG_TYPE_CHANGE, MSG_SUB_TYPE_TIME,
            PURC_VARIANT_INVALID);
    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

#define _KN_sec   "sec"
#define _KN_usec  "usec"

static purc_variant_t
time_us_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    bool silently = call_flags & PCVRT_CALL_FLAG_SILENTLY;
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    int rettype = -1;
    if (nr_args == 0) {
        rettype = PURC_K_KW_longdouble;
    }
    else {
        const char *option;
        size_t option_len;

        option = purc_variant_get_string_const_ex(argv[0], &option_len);
        if (option == NULL) {
            pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        }
        else {
            option = pcutils_trim_spaces(option, &option_len);
            if (option_len == 0) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            }
            else {
                rettype = pcdvobjs_global_keyword_id(option, option_len);
            }
        }
    }

    if (rettype == -1) {
        // bad keyword, do not change error code.
    }
    else if (rettype == PURC_K_KW_longdouble) {
        silently = true;
    }
    else if (rettype == PURC_K_KW_object) {
        // create an empty object
        retv = purc_variant_make_object(0,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        if (retv == PURC_VARIANT_INVALID) {
            goto fatal;
        }

        val = purc_variant_make_longint((int64_t)tv.tv_sec);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(retv,
                    _KN_sec, val))
            goto fatal;
        purc_variant_unref(val);

        val = purc_variant_make_longint((int64_t)tv.tv_usec);
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(retv,
                    _KN_usec, val))
            goto fatal;
        purc_variant_unref(val);
        return retv;
    }
    else {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
    }

    if (silently) {
        long double time_ld = (long double)tv.tv_sec;
        time_ld += tv.tv_usec/1000000.0L;

        return purc_variant_make_longdouble(time_ld);
    }

fatal:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
time_us_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(call_flags);

    int64_t l_sec, l_usec;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (purc_variant_get_type(argv[0]) == PURC_VARIANT_TYPE_OBJECT) {
        purc_variant_t v1 = purc_variant_object_get_by_ckey(argv[0],
                _KN_sec);
        purc_variant_t v2 = purc_variant_object_get_by_ckey(argv[0],
                _KN_usec);

        if (v1 == PURC_VARIANT_INVALID || v2 == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        if (!purc_variant_cast_to_longint(v1, &l_sec, false) ||
                !purc_variant_cast_to_longint(v2, &l_usec, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }
    else {
        long double time_d, sec_d, usec_d;
        if (!purc_variant_cast_to_longdouble(argv[0], &time_d, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (isinf(time_d) || isnan(time_d) || time_d < 0.0L) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        usec_d = modfl(time_d, &sec_d);
        l_sec = (int64_t)sec_d;
        l_usec = (int64_t)(usec_d * 1000000.0);
    }

    if (l_usec < 0 || l_usec > 999999) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    struct timeval timeval;
    timeval.tv_sec = (time_t)l_sec;
    timeval.tv_usec = (suseconds_t)l_usec;
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

    // broadcast "change:time" event
    broadcast_event(root, MSG_TYPE_CHANGE, MSG_SUB_TYPE_TIME,
            PURC_VARIANT_INVALID);
    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
sleep_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int arg_type = purc_variant_get_type(argv[0]);
    long double ld_rem;

    pcintr_coroutine_t crtn;
    if (call_flags & PCVRT_CALL_FLAG_AGAIN) {
        crtn = pcintr_get_coroutine();
        PC_ASSERT(call_flags & PCVRT_CALL_FLAG_TIMEOUT);
        PC_ASSERT(crtn);

        // TODO: get the remaining time from crtn and return it
        ld_rem = 0;

        if (arg_type == PURC_VARIANT_TYPE_LONGINT) {
            return purc_variant_make_ulongint((int64_t)ld_rem);
        }
        else if (arg_type == PURC_VARIANT_TYPE_ULONGINT) {
            return purc_variant_make_ulongint((uint64_t)ld_rem);
        }
        else if (arg_type == PURC_VARIANT_TYPE_ULONGINT) {
            return purc_variant_make_number((double)ld_rem);
        }

        return purc_variant_make_longdouble(ld_rem);
    }

    uint64_t ul_sec = 0;
    long     l_nsec = 0;

    if (arg_type == PURC_VARIANT_TYPE_LONGINT) {
        int64_t tmp;
        purc_variant_cast_to_longint(argv[0], &tmp, false);
        if (tmp < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        ul_sec = (uint64_t)tmp;
    }
    else if (arg_type == PURC_VARIANT_TYPE_ULONGINT) {
        purc_variant_cast_to_ulongint(argv[0], &ul_sec, false);
    }
    else if (arg_type == PURC_VARIANT_TYPE_NUMBER) {
        double time_d, sec_d, nsec_d;
        purc_variant_cast_to_number(argv[0], &time_d, false);

        if (isinf(time_d) || isnan(time_d) || time_d < 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        nsec_d = modf(time_d, &sec_d);
        ul_sec = (uint64_t)sec_d;
        l_nsec = (long)(nsec_d * 1000000000.0);
    }
    else if (arg_type == PURC_VARIANT_TYPE_LONGDOUBLE) {
        long double time_d, sec_d, nsec_d;
        purc_variant_cast_to_longdouble(argv[0], &time_d, false);

        if (isinf(time_d) || isnan(time_d) || time_d < 0.0L) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        nsec_d = modfl(time_d, &sec_d);
        ul_sec = (uint64_t)sec_d;
        l_nsec = (long)(nsec_d * 1000000000.0);
    }
    else {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (l_nsec < 0 || l_nsec > 999999999) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    struct timespec req, rem;
    req.tv_sec = (time_t)ul_sec;
    req.tv_nsec = (long)l_nsec;

    crtn = pcintr_get_coroutine();
    if (crtn == NULL) {
        if (nanosleep(&req, &rem) == 0) {
            ld_rem = 0;
        }
        else {
            if (errno == EINTR) {
                ld_rem = rem.tv_sec + rem.tv_nsec / 1000000000.0L;
            }
            else if (errno == EINVAL) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }
            else {
                purc_set_error(PURC_ERROR_SYS_FAULT);
                goto fatal;
            }
        }

        if (arg_type == PURC_VARIANT_TYPE_LONGINT) {
            return purc_variant_make_ulongint((int64_t)ld_rem);
        }
        else if (arg_type == PURC_VARIANT_TYPE_ULONGINT) {
            return purc_variant_make_ulongint((uint64_t)ld_rem);
        }
        else if (arg_type == PURC_VARIANT_TYPE_ULONGINT) {
            return purc_variant_make_number((double)ld_rem);
        }

        return purc_variant_make_longdouble(ld_rem);
    }
    else {
        pcintr_stop_coroutine(crtn, &req);
        purc_set_error(PURC_ERROR_AGAIN);
        return PURC_VARIANT_INVALID;
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
locale_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *category = NULL;
    size_t length = 0;
    purc_atom_t atom = 0;

    if (nr_args == 0) {
        category = _KW_messages;
        length = sizeof(_KW_messages) - 1;
        atom = keywords2atoms[K_KW_messages].atom;
    }
    else {
        category = purc_variant_get_string_const_ex(argv[0], &length);
        if (category == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        category = pcutils_trim_spaces(category, &length);
        if (length == 0) {
            category = _KW_messages;
            length = sizeof(_KW_messages) - 1;
            atom = keywords2atoms[K_KW_messages].atom;
        }
        else if (length > MAX_LEN_KEYWORD) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
        else {
            char *tmp = strndup(category, length);
            atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
            free(tmp);
        }
    }

    char *locale = NULL;
    if (atom == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }
    else if (atom == keywords2atoms[K_KW_ctype].atom) {
        locale = setlocale(LC_CTYPE, NULL);
    }
    else if (atom == keywords2atoms[K_KW_numeric].atom) {
        locale = setlocale(LC_NUMERIC, NULL);
    }
    else if (atom == keywords2atoms[K_KW_time].atom) {
        locale = setlocale(LC_TIME, NULL);
    }
    else if (atom == keywords2atoms[K_KW_collate].atom) {
        locale = setlocale(LC_COLLATE, NULL);
    }
    else if (atom == keywords2atoms[K_KW_monetary].atom) {
        locale = setlocale(LC_MONETARY, NULL);
    }
    else if (atom == keywords2atoms[K_KW_messages].atom) {
        locale = setlocale(LC_MESSAGES, NULL);
    }
#ifdef LC_PAPER
    else if (atom == keywords2atoms[K_KW_paper].atom) {
        locale = setlocale(LC_PAPER, NULL);
    }
#endif /* LC_PAPER */
#ifdef LC_NAME
    else if (atom == keywords2atoms[K_KW_name].atom) {
        locale = setlocale(LC_NAME, NULL);
    }
#endif /* LC_NAME */
#ifdef LC_ADDRESS
    else if (atom == keywords2atoms[K_KW_address].atom) {
        locale = setlocale(LC_ADDRESS, NULL);
    }
#endif /* LC_ADDRESS */
#ifdef LC_TELEPHONE
    else if (atom == keywords2atoms[K_KW_telephone].atom) {
        locale = setlocale(LC_TELEPHONE, NULL);
    }
#endif /* LC_TELEPHONE */
#ifdef LC_MEASUREMENT
    else if (atom == keywords2atoms[K_KW_measurement].atom) {
        locale = setlocale(LC_MEASUREMENT, NULL);
    }
#endif /* LC_MEASUREMENT */
#ifdef LC_IDENTIFICATION
    else if (atom == keywords2atoms[K_KW_identification].atom) {
        locale = setlocale(LC_IDENTIFICATION, NULL);
    }
#endif /* LC_IDENTIFICATION */
    else {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto failed;
    }

    if (locale) {
        char *end = strchr(locale, '.');
        if (end)
            length = end - locale;
        else
            length = strlen(locale);
        return purc_variant_make_string_ex(locale, length, false);
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
locale_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *categories;
    const char *locale;
    size_t categories_len = 0, locale_len = 0;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    categories = purc_variant_get_string_const_ex(argv[0], &categories_len);
    if (categories == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    categories = pcutils_trim_spaces(categories, &categories_len);
    if (categories_len == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    locale = purc_variant_get_string_const_ex(argv[1], &locale_len);
    if (locale == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    locale = pcutils_trim_spaces(locale, &locale_len);
    if (locale_len == 0 || locale_len > MAX_LEN_KEYWORD) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    /* check locale more and concatenate .UTF-8 .utf8 postfix */
    char normalized[16];
    {
        if (purc_islower(locale[0]) && purc_islower(locale[1]) &&
                locale[2] == '_' &&
                purc_isupper(locale[3]) && purc_isupper(locale[4])) {
            strncpy(normalized, locale, 5);
            normalized[5] = '\0';
            strcat(normalized, ".UTF-8");
            locale = normalized;
        }
        else {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    purc_atom_t atom;
    {
        char *tmp = strndup(categories, categories_len);
        atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
        free(tmp);
    }

    if (atom == keywords2atoms[K_KW_all].atom) {
        if (setlocale(LC_ALL, locale) == NULL) {
            purc_set_error(PURC_ERROR_BAD_STDC_CALL);
            goto failed;
        }
    }
    else {
        const char *category;
        size_t length;

        category = pcutils_get_next_token_len(categories, categories_len,
                _KW_DELIMITERS, &length);
        while (category) {
            char *tmp = strndup(category, length);
            atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
            free(tmp);

            char *retv = NULL;
            if (atom == keywords2atoms[K_KW_ctype].atom) {
                retv = setlocale(LC_CTYPE, locale);
            }
            else if (atom == keywords2atoms[K_KW_numeric].atom) {
                retv = setlocale(LC_NUMERIC, locale);
            }
            else if (atom == keywords2atoms[K_KW_time].atom) {
                retv = setlocale(LC_TIME, locale);
            }
            else if (atom == keywords2atoms[K_KW_collate].atom) {
                retv = setlocale(LC_COLLATE, locale);
            }
            else if (atom == keywords2atoms[K_KW_monetary].atom) {
                retv = setlocale(LC_MONETARY, locale);
            }
            else if (atom == keywords2atoms[K_KW_messages].atom) {
                retv = setlocale(LC_MESSAGES, locale);
            }
#ifdef LC_PAPER
            else if (atom == keywords2atoms[K_KW_paper].atom) {
                retv = setlocale(LC_PAPER, locale);
            }
#endif /* LC_PAPER */
#ifdef LC_NAME
            else if (atom == keywords2atoms[K_KW_name].atom) {
                retv = setlocale(LC_NAME, locale);
            }
#endif /* LC_NAME */
#ifdef LC_ADDRESS
            else if (atom == keywords2atoms[K_KW_address].atom) {
                retv = setlocale(LC_ADDRESS, locale);
            }
#endif /* LC_ADDRESS */
#ifdef LC_TELEPHONE
            else if (atom == keywords2atoms[K_KW_telephone].atom) {
                retv = setlocale(LC_TELEPHONE, locale);
            }
#endif /* LC_TELEPHONE */
#ifdef LC_MEASUREMENT
            else if (atom == keywords2atoms[K_KW_measurement].atom) {
                retv = setlocale(LC_MEASUREMENT, locale);
            }
#endif /* LC_MEASUREMENT */
#ifdef LC_IDENTIFICATION
            else if (atom == keywords2atoms[K_KW_identification].atom) {
                retv = setlocale(LC_IDENTIFICATION, locale);
            }
#endif /* LC_IDENTIFICATION */
            else {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            if (retv == NULL) {
                purc_set_error(PURC_ERROR_BAD_STDC_CALL);
                goto failed;
            }

            if (categories_len <= length)
                break;

            categories_len -= length;
            category = pcutils_get_next_token_len(category + length,
                    categories_len, _KW_DELIMITERS, &length);
        }
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

bool
pcdvobjs_get_current_timezone(char *buff, size_t sz_buff)
{
    const char *timezone;
    char path[PATH_MAX + 1];

    const char* env_tz = getenv("TZ");
    if (env_tz) {
        if (env_tz[0] == ':')
            timezone = env_tz + 1;
        if (!pcdvobjs_is_valid_timezone(timezone)) {
            timezone = "posixrules";
        }
    }
    else {
        ssize_t nr_bytes;
        nr_bytes = readlink(PURC_SYS_TZ_FILE, path, sizeof(path) - 1);
        if (nr_bytes > 0)
            path[nr_bytes] = '\0';
        if ((nr_bytes > 0) &&
                (strstr(path, PURC_SYS_TZ_DIR) != NULL)) {
            timezone = strstr(path, PURC_SYS_TZ_DIR);
            timezone += sizeof(PURC_SYS_TZ_DIR) - 1;
        }
        else {
            /* XXX: Should we return "UTC" as the default one? */
            purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
            purc_log_error("Cannot determine timezone.\n");
            goto failed;
        }
    }

    if (strlen(timezone) >= sz_buff) {
        purc_set_error(PURC_ERROR_TOO_SMALL_BUFF);
        goto failed;
    }

    strcpy(buff, timezone);
    return true;

failed:
    return false;
}

static purc_variant_t
timezone_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    char timezone[MAX_LEN_TIMEZONE];
    if (pcdvobjs_get_current_timezone(timezone, sizeof(timezone))) {
        return purc_variant_make_string(timezone, false);
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

bool pcdvobjs_is_valid_timezone(const char *timezone)
{
    assert(timezone);

    char path[PATH_MAX + 1];
    if (strlen(timezone) < PATH_MAX - sizeof(PURC_SYS_TZ_DIR)) {
        strcpy(path, PURC_SYS_TZ_DIR);
        strcat(path, timezone);
        if (access(path, F_OK)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }

        if (access(path, R_OK)) {
            purc_set_error(PURC_ERROR_ACCESS_DENIED);
            goto failed;
        }
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    return true;

failed:
    return false;
}

static purc_variant_t
timezone_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *timezone;
    if ((timezone = purc_variant_get_string_const(argv[0])) == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!pcdvobjs_is_valid_timezone(timezone))
        goto failed;

    char path[PATH_MAX + 1];
    if (nr_args > 1) {
        int global = -1;
        const char *option;
        size_t option_len;

        option = purc_variant_get_string_const_ex(argv[1], &option_len);
        if (option == NULL) {
            pcinst_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        }
        else {
            option = pcutils_trim_spaces(option, &option_len);
            if (option_len == 0) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
            }
            else {
                switch (pcdvobjs_global_keyword_id(option, option_len)) {
                case PURC_K_KW_local:
                    global = 0;
                    break;
                case PURC_K_KW_global:
                    global = 1;
                    break;
                default:
                    // keep global being -1
                    pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                    break;
                }
            }
        }

        if (global == -1) {
            if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
                global = 0;
            }
            else {
                goto failed;
            }
        }

        /* try to change timezone permanently */
        if (global) {

            if (unlink(PURC_SYS_TZ_FILE) == 0) {
                if (symlink(PURC_SYS_TZ_FILE, path)) {
                    purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
                    goto failed;
                }
            }
            else {
                purc_set_error(PURC_ERROR_ACCESS_DENIED);
                goto failed;
            }
        }
    }

    strcpy(path, ":");
    strcat(path, timezone);
    setenv("TZ", path, 1);
    tzset();

    // broadcast "change:env" event
    broadcast_event(root, MSG_TYPE_CHANGE, MSG_SUB_TYPE_ENV,
            PURC_VARIANT_INVALID);
    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}


static purc_variant_t
cwd_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
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
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
cwd_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
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

    // broadcast "change:cwd" event
    broadcast_event(root, MSG_TYPE_CHANGE, MSG_SUB_TYPE_CWD,
            PURC_VARIANT_INVALID);
    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
env_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
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
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}


static purc_variant_t
env_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
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
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
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

    // broadcast "change:env" event
    broadcast_event(root, MSG_TYPE_CHANGE, MSG_SUB_TYPE_ENV,
            PURC_VARIANT_INVALID);
    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

#define MAX_LEN_STATE_BUF   256

#if HAVE(RANDOM_R)
struct local_random_data {
    char                state_buf[MAX_LEN_STATE_BUF];
    size_t              state_len;
    struct random_data  data;
};

static void cb_free_local_random_data(void *key, void *local_data)
{
    if (key)
        free_key_string(key);
    free(local_data);
}
#else
static char random_state[MAX_LEN_STATE_BUF];
#endif

int32_t pcdvobjs_get_random(void)
{
#if HAVE(RANDOM_R)
    struct local_random_data *rd = NULL;
    purc_get_local_data(PURC_LDNAME_RANDOM_DATA, (uintptr_t *)&rd, NULL);

    int32_t result;
    if (rd)
        random_r(&rd->data, &result);
    else
        result = (int32_t)random();
#else
    long int result;
    result = random();
#endif

    return (int32_t)result;
}

static purc_variant_t
random_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int32_t result = pcdvobjs_get_random();

    if (nr_args == 0) {
        return purc_variant_make_longint((int64_t)result);
    }

    switch (purc_variant_get_type(argv[0])) {
    case PURC_VARIANT_TYPE_NUMBER:
    {
        double max, number;
        purc_variant_cast_to_number(argv[0], &max, false);
        number = max * result / (double)(RAND_MAX);
        return purc_variant_make_number(number);
    }

    case PURC_VARIANT_TYPE_LONGINT:
    {
        int64_t max, number;
        purc_variant_cast_to_longint(argv[0], &max, false);
        number = max * result / RAND_MAX;
        return purc_variant_make_longint(number);
    }

    case PURC_VARIANT_TYPE_ULONGINT:
    {
        uint64_t max, number;
        purc_variant_cast_to_ulongint(argv[0], &max, false);
        number = max * result / RAND_MAX;
        return purc_variant_make_ulongint(number);
    }

    case PURC_VARIANT_TYPE_LONGDOUBLE:
    {
        long double max, number;
        purc_variant_cast_to_longdouble(argv[0], &max, false);
        number = max * result / (long double)(RAND_MAX);
        return purc_variant_make_longdouble(number);
    }

    default:
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        break;
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
random_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    uint64_t seed;
    uint64_t complexity = 8;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_cast_to_ulongint(argv[0], &seed, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (seed > UINT32_MAX) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args > 1) {
        if (!purc_variant_cast_to_ulongint(argv[1], &complexity, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (complexity < 8 || complexity > 256) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

#if HAVE(RANDOM_R)
    struct local_random_data *rd = NULL;
    purc_get_local_data(PURC_LDNAME_RANDOM_DATA, (uintptr_t *)&rd, NULL);
    assert(rd);

    rd->data.state = NULL;
    initstate_r((unsigned int)seed, rd->state_buf, (size_t)complexity,
            &rd->data);
#else
    initstate((unsigned int)seed, random_state, (size_t)complexity);
#endif

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

#if OS(LINUX)

#include <sys/random.h>

static purc_variant_t
random_sequence_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
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
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

#else   /* OS(LINUX) */

static purc_variant_t
random_sequence_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}
#endif  /* !OS(LINUX) */

static purc_variant_t
remove_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
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

    if (remove(path)) {
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static
int parse_pipe_flags(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int64_t flags = 0;

    if (option == PURC_VARIANT_INVALID) {
        atom = keywords2atoms[K_KW_default].atom;
    }
    else {
        parts = purc_variant_get_string_const_ex(option, &parts_len);
        if (parts == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            flags = -1;
            goto done;
        }

        parts = pcutils_trim_spaces(parts, &parts_len);
        if (parts_len == 0) {
            atom = keywords2atoms[K_KW_default].atom;
        }
    }

    if (atom == 0) {
        char tmp[parts_len + 1];
        strncpy(tmp, parts, parts_len);
        tmp[parts_len]= '\0';
        atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
    }

    if (atom == keywords2atoms[K_KW_none].atom) {
        flags = 0;
    }
    else if (atom == keywords2atoms[K_KW_default].atom) {
        flags = O_CLOEXEC;
    }
    else {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                _KW_DELIMITERS, &length);
        do {
            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = 0;
            }
            else {
                char tmp[length + 1];
                strncpy(tmp, part, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
            }

            if (atom == keywords2atoms[K_KW_nonblock].atom) {
                flags |= O_NONBLOCK;
            }
            else if (atom == keywords2atoms[K_KW_cloexec].atom) {
                flags |= O_CLOEXEC;
            }
            else {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                flags = -1;
                break;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    _KW_DELIMITERS, &length);
        } while (part);
    }

done:
    return flags;
}

static purc_variant_t
pipe_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int flags = parse_pipe_flags(nr_args > 0 ? argv[0] : PURC_VARIANT_INVALID);
    if (flags == -1) {
        goto failed;
    }

    int fds[2];

#if OS(LINUX)
    if (pipe2(fds, flags) == -1) {
         purc_set_error(purc_error_from_errno(errno));
         goto failed;
    }
#else
    if (pipe(fds) == -1) {
         purc_set_error(purc_error_from_errno(errno));
         goto failed;
    }

    for (int i = 0; i < 2; i++) {
        if (flags & O_CLOEXEC && fcntl(fds[i], F_SETFD, FD_CLOEXEC) == -1) {
            purc_set_error(purc_error_from_errno(errno));
            goto failed;
        }

        if (flags & O_NONBLOCK && fcntl(fds[i], F_SETFL, O_NONBLOCK) == -1) {
            purc_set_error(purc_error_from_errno(errno));
            goto failed;
        }
    }
#endif

    purc_variant_t fd1 = purc_variant_make_longint(fds[0]);
    purc_variant_t fd2 = purc_variant_make_longint(fds[0]);
    if (!fd1 || !fd2) {
        if (fd1)
            purc_variant_unref(fd1);
        if (fd2)
            purc_variant_unref(fd2);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t kv[2] = { fd1, fd2 };
    purc_variant_t retv = purc_variant_make_tuple(2, kv);
    purc_variant_unref(fd1);
    purc_variant_unref(fd2);
    return retv;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static
int parse_fdflags_getter_flags(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int64_t flags = 0;

    if (option == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        flags = -1;
    }
    else {
        parts = purc_variant_get_string_const_ex(option, &parts_len);
        if (parts == NULL) {
            flags = -1;
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto done;
        }

        parts = pcutils_trim_spaces(parts, &parts_len);
        if (parts_len == 0) {
            flags = -1;
            purc_set_error(PURC_ERROR_INVALID_VALUE);
        }
        else {
            char tmp[parts_len + 1];
            strncpy(tmp, parts, parts_len);
            tmp[parts_len]= '\0';
            atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);

            if (atom == keywords2atoms[K_KW_nonblock].atom) {
                flags = O_NONBLOCK;
            }
            else if (atom == keywords2atoms[K_KW_cloexec].atom) {
                flags = O_CLOEXEC;
            }
            else if (atom == keywords2atoms[K_KW_append].atom) {
                flags = O_APPEND;
            }
            else {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                flags = -1;
            }
        }
    }

done:
    return flags;
}

static purc_variant_t
fdflags_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    int64_t tmp_l;
    if (!purc_variant_cast_to_longint(argv[0], &tmp_l, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    if (tmp_l < 0 || tmp_l > INT_MAX) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    int flags = parse_fdflags_getter_flags(argv[1]);
    if (flags == -1) {
        goto error;
    }

    int fd = (int)tmp_l;
    bool result = false;
    if (flags & O_CLOEXEC) {
        int ret = fcntl(fd, F_GETFD);
        if (ret == -1) {
            purc_set_error(purc_error_from_errno(errno));
            goto error;
        }

        result = !!(ret & FD_CLOEXEC);
    }
    else if (flags & O_APPEND) {
        int ret = fcntl(fd, F_GETFL);
        if (ret == -1) {
            purc_set_error(purc_error_from_errno(errno));
            goto error;
        }
        result = !!(ret & O_APPEND);
    }
    else if (flags & O_NONBLOCK) {
        int ret = fcntl(fd, F_GETFL);
        if (ret == -1) {
            purc_set_error(purc_error_from_errno(errno));
            goto error;
        }
        result = !!(ret & O_NONBLOCK);
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    return purc_variant_make_boolean(result);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static
int parse_fdflags_setter_flags(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int64_t flags = 0;

    if (option == PURC_VARIANT_INVALID) {
        flags = -1;
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
    }
    else {
        parts = purc_variant_get_string_const_ex(option, &parts_len);
        if (parts == NULL) {
            flags = -1;
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        }
        else {
            parts = pcutils_trim_spaces(parts, &parts_len);
            if (parts_len == 0) {
                flags = -1;
                purc_set_error(PURC_ERROR_INVALID_VALUE);
            }
        }
    }

    if (flags == -1)
        goto done;

    size_t length = 0;
    const char *part = pcutils_get_next_token_len(parts, parts_len,
            _KW_DELIMITERS, &length);
    do {
        if (length == 0 || length > MAX_LEN_KEYWORD) {
            atom = 0;
        }
        else {
            char tmp[length + 1];
            strncpy(tmp, part, length);
            tmp[length]= '\0';
            atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
        }

        if (atom == keywords2atoms[K_KW_nonblock].atom) {
            flags |= O_NONBLOCK;
        }
        else if (atom == keywords2atoms[K_KW_append].atom) {
            flags |= O_APPEND;
        }
        else if (atom == keywords2atoms[K_KW_cloexec].atom) {
            flags |= O_CLOEXEC;
        }
        else {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            flags = -1;
            break;
        }

        if (parts_len <= length)
            break;

        parts_len -= length;
        part = pcutils_get_next_token_len(part + length, parts_len,
                _KW_DELIMITERS, &length);
    } while (part);

done:
    return flags;
}

static purc_variant_t
fdflags_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    int64_t tmp_l;
    if (!purc_variant_cast_to_longint(argv[0], &tmp_l, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    if (tmp_l < 0 || tmp_l > INT_MAX) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    int flags = parse_fdflags_setter_flags(argv[1]);
    if (flags == -1) {
        goto error;
    }

    int fd = (int)tmp_l;
    if ((flags & O_CLOEXEC) && (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if (flags & O_APPEND || flags & O_NONBLOCK) {
        if (fcntl(fd, F_SETFL,
                    (flags & O_APPEND) ? O_APPEND : 0 |
                    (flags & O_NONBLOCK) ? O_NONBLOCK : 0) == -1) {
            purc_set_error(purc_error_from_errno(errno));
            goto error;
        }
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    return purc_variant_make_boolean(true);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static
purc_atom_t parse_socket_option(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;

    parts = purc_variant_get_string_const_ex(option, &parts_len);
    if (parts == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
    }
    else {
        parts = pcutils_trim_spaces(parts, &parts_len);
        if (parts_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
        }
        else {
            char tmp[parts_len + 1];
            strncpy(tmp, parts, parts_len);
            tmp[parts_len]= '\0';
            atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
        }
    }

    return atom;
}

static purc_variant_t
sockopt_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    int64_t tmp_l;
    if (!purc_variant_cast_to_longint(argv[0], &tmp_l, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    if (tmp_l < 0 || tmp_l > INT_MAX) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    purc_atom_t option = parse_socket_option(argv[1]);
    if (option == 0) {
        goto error;
    }

    int fd = (int)tmp_l;
    int optname = 0;

    struct timeval timeval;
    int intval;
    void *optval = &intval;
    socklen_t optlen = sizeof(intval);

    if (option == keywords2atoms[K_KW_type].atom) {
        optname = SO_TYPE;
    }
#if OS(Mac)
    else if (option == keywords2atoms[K_KW_nread].atom) {
        optname = SO_NREAD;
    }
    else if (option == keywords2atoms[K_KW_nwrite].atom) {
        optname = SO_NWRITE;
    }
#endif
    else if (option == keywords2atoms[K_KW_recv_timeout].atom) {
        optval = &timeval;
        optlen = sizeof(timeval);
        optname = SO_RCVTIMEO;
    }
    else if (option == keywords2atoms[K_KW_send_timeout].atom) {
        optval = &timeval;
        optlen = sizeof(timeval);
        optname = SO_SNDTIMEO;
    }
    else if (option == keywords2atoms[K_KW_recv_buffer].atom) {
        optname = SO_RCVBUF;
    }
    else if (option == keywords2atoms[K_KW_send_buffer].atom) {
        optname = SO_SNDBUF;
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    if (getsockopt(fd, SOL_SOCKET, optname, optval, &optlen) == -1) {
        printf("Failed getsockopt(): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    purc_variant_t retv;
    if (option == keywords2atoms[K_KW_type].atom) {
        PC_ASSERT(optlen == sizeof(intval));

        if (intval == SOCK_STREAM) {
            retv = purc_variant_make_string_static("stream", false);
        }
        else if (intval == SOCK_DGRAM) {
            retv = purc_variant_make_string_static("dgram", false);
        }
        else {
            retv = purc_variant_make_string_static("unknown", false);
        }
    }
    else if (option == keywords2atoms[K_KW_recv_timeout].atom ||
            option == keywords2atoms[K_KW_send_timeout].atom) {
        PC_ASSERT(optlen == sizeof(timeval));

        double tmp = (double)timeval.tv_sec;
        tmp += timeval.tv_usec/1000000.0L;
        retv = purc_variant_make_number(tmp);
    }

    return retv;

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
sockopt_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 3) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    int64_t tmp_l;
    if (!purc_variant_cast_to_longint(argv[0], &tmp_l, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    if (tmp_l < 0 || tmp_l > INT_MAX) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    int fd = (int)tmp_l;

    purc_atom_t option = parse_socket_option(argv[1]);
    if (option == 0) {
        goto error;
    }

    struct timeval timeval = {};
    int intval = 0;
    const void *optval = &intval;
    socklen_t optlen = sizeof(intval);

    int optname = 0;
    if (option == keywords2atoms[K_KW_recv_timeout].atom) {
        if (!dvobjs_cast_to_timeval(&timeval, argv[2])) {
            goto error;
        }

        optval = &timeval;
        optlen = sizeof(timeval);
        optname = SO_RCVTIMEO;
    }
    else if (option == keywords2atoms[K_KW_send_timeout].atom) {
        if (!dvobjs_cast_to_timeval(&timeval, argv[2])) {
            goto error;
        }

        optval = &timeval;
        optlen = sizeof(timeval);
        optname = SO_SNDTIMEO;
    }
    else if (option == keywords2atoms[K_KW_recv_buffer].atom) {
        int64_t tmp_l;
        if (!purc_variant_cast_to_longint(argv[0], &tmp_l, false)) {
            goto error;
        }

        intval = (int)tmp_l;
        optname = SO_RCVBUF;
    }
    else if (option == keywords2atoms[K_KW_send_buffer].atom) {
        int64_t tmp_l;
        if (!purc_variant_cast_to_longint(argv[0], &tmp_l, false)) {
            goto error;
        }

        intval = (int)tmp_l;
        optname = SO_SNDBUF;
    }
    else {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    if (setsockopt(fd, SOL_SOCKET, optname, optval, optlen) == -1) {
        printf("Failed setsockopt(): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    return purc_variant_make_boolean(true);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
close_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    int64_t tmp_l;
    if (!purc_variant_cast_to_longint(argv[0], &tmp_l, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    if (tmp_l < 0 || tmp_l > INT_MAX) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    int fd = (int)tmp_l;
    if (close(fd) == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    return purc_variant_make_boolean(true);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_system_new (void)
{
    static const struct purc_dvobj_method methods[] = {
        { "const",      const_getter,       NULL },
        { "uname",      uname_getter,       NULL },
        { "uname_prt",  uname_prt_getter,   NULL },
        { "time",       time_getter,        time_setter },
        { "time_us",    time_us_getter,     time_us_setter },
        { "sleep",      sleep_getter,       NULL },
        { "locale",     locale_getter,      locale_setter },
        { "timezone",   timezone_getter,    timezone_setter },
        { "cwd",        cwd_getter,         cwd_setter },
        { "env",        env_getter,         env_setter },
        { "random",     random_getter,      random_setter },
        { "random_sequence", random_sequence_getter, NULL },
        { "remove",     remove_getter,      NULL },
        { "pipe",       pipe_getter,        NULL },
        { "fdflags",    fdflags_getter,     fdflags_setter },
        { "sockopt",    sockopt_getter,     sockopt_setter },
        { "close",      close_getter,       NULL },
    };

    if (keywords2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
            keywords2atoms[i].atom =
                purc_atom_from_static_string_ex(ATOM_BUCKET_DVOBJ,
                    keywords2atoms[i].keyword);
        }

    }

#if HAVE(RANDOM_R)
    /* allocate data for state of the random generator */
    struct local_random_data *rd;
    rd = calloc(1, sizeof(*rd));
    if (rd) {
        if (!purc_set_local_data(PURC_LDNAME_RANDOM_DATA,
                    (uintptr_t)rd, cb_free_local_random_data)) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        initstate_r(time(NULL), rd->state_buf, 8, &rd->data);
    }
    else {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }
#else
    initstate(time(NULL), random_state, 8);
#endif

    return purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
}

