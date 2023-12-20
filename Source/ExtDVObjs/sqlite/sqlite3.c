/*
 * @file sqlite3.c
 * @author Xue Shuming
 * @date 2023/12/20
 * @brief The implementation of dynamic variant object $SQLITE.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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

//#undef NDEBUG

#include <sqlite3.h>

#include "config.h"
#include "private/map.h"
#include "private/dvobjs.h"
#include "private/instance.h"
#include "private/atom-buckets.h"
#include "private/debug.h"
#include "purc-variant.h"
#include "purc-errors.h"

#define SQLITE_DVOBJ_VERNAME        "0.1.0"
#define SQLITE_DVOBJ_VERCODE        0

#define MAX_SYMBOL_LEN              64

#define SQLITE_DEFAULT_TIMEOUT      5

#define STR(x)                      #x
#define STR2(x)                     STR(x)
#define SQLITE_DVOBJ_VERCODE_STR    STR2(SQLITE_DVOBJ_VERCODE)

#define SQLITE_KEY_IMPL             "impl"
#define SQLITE_KEY_INFO             "info"

#define SQLITE_KEY_CONNECT          "connect"
#define SQLITE_KEY_CURSOR           "cursor"
#define SQLITE_KEY_COMMIT           "commit"
#define SQLITE_KEY_ROLLBACK         "rollback"
#define SQLITE_KEY_CLOSE            "close"
#define SQLITE_KEY_EXECUTE          "execute"
#define SQLITE_KEY_EXECUTEMANY      "executemany"
#define SQLITE_KEY_EXECUTE          "execute"
#define SQLITE_KEY_EXECUTEMANY      "executemany"
#define SQLITE_KEY_FETCHONE         "fetchone"
#define SQLITE_KEY_FETCHMANY        "fetchmany"
#define SQLITE_KEY_FETCHALL         "fetchall"
#define SQLITE_KEY_ROWCOUNT         "rowcount"
#define SQLITE_KEY_LASTROWID        "lastrowid"
#define SQLITE_KEY_DESCRIPTION      "description"
#define SQLITE_KEY_CONNECTION       "connection"
#define SQLITE_KEY_HANDLE           "__handle_sqlite__"

#define SQLITE_INFO_VERSION         "version"
#define SQLITE_INFO_PLATFORM        "platform"
#define SQLITE_INFO_COPYRIGHT       "copyright"
#define SQLITE_INFO_COMPILER        "compiler"
#define SQLITE_INFO_BUILD_INFO      "build-info"

#define _KW_DELIMITERS              " \t\n\v\f\r"

#if defined(__linux) || defined(__linux__) || defined(linux)
#define SQLITE_PLATFORM             "Linux"
#elif defined(__APPLE__)↵
#define SQLITE_PLATFORM             "Darwin"
#else
#define SQLITE_PLATFORM             "Unknown"
#endif

struct dvobj_sqlite_info {
    purc_variant_t          root;               // the root variant, i.e., $SQLITE itself
    struct pcvar_listener   *listener;          // the listener
};

struct dvobj_sqlite_connection {
    purc_variant_t              root;               // the root variant, itself
    sqlite3                     *db;
    char                        *db_name;
    struct pcvar_listener       *listener;          // the listener
};

struct dvobj_sqlite_cursor {
    purc_variant_t              root;               // the root variant, itself
    sqlite3                     *db;
    char                        *db_name;
    struct pcvar_listener       *listener;          // the listener
};

/* $SQLiteConnect begin */
static inline struct dvobj_sqlite_connection *
get_connection_from_root(purc_variant_t root)
{
    purc_variant_t v;

    v = purc_variant_object_get_by_ckey(root, SQLITE_KEY_HANDLE);
    assert(v && purc_variant_is_native(v));

    return (struct dvobj_sqlite_connection *)purc_variant_native_get_entity(v);
}

static struct dvobj_sqlite_connection *
create_connection(struct dvobj_sqlite_info *sqlite_info, const char *db_name)
{
    (void) sqlite_info;
    int rc;
    sqlite3 *db = NULL;
    struct dvobj_sqlite_connection *connection = NULL;
    rc = sqlite3_open_v2(db_name, &db,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
                         , NULL);
    if (rc == SQLITE_OK) {
        (void)sqlite3_busy_timeout(db, (int)(SQLITE_DEFAULT_TIMEOUT*1000));
    }

    if (db == NULL && rc == SQLITE_NOMEM) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    connection = calloc(1, sizeof(*connection));
    if (!connection) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    connection->db = db;

    return connection;

failed:
    if (db) {
        sqlite3_close(db);
    }
    return NULL;
}

static bool on_sqlite_connection_being_released(purc_variant_t src, pcvar_op_t op,
        void *ctxt, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (op == PCVAR_OPERATION_RELEASING) {
        struct dvobj_sqlite_connection *connection = ctxt;
        purc_variant_revoke_listener(src, connection->listener);
        if (connection->db) {
            sqlite3_close(connection->db);
        }
        free(connection);
    }

    return true;
}

static inline int
conn_exec_stmt(struct dvobj_sqlite_connection *conn, const char *sql)
{
    int rc;
    int len = (int)strlen(sql) + 1;
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(conn->db, sql, len, &stmt, NULL);
    if (rc == SQLITE_OK) {
        (void)sqlite3_step(stmt);
        rc = sqlite3_finalize(stmt);
    }

    if (rc != SQLITE_OK) {
        return -1;
    }
    return 0;
}


static purc_variant_t cursor_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    return PURC_VARIANT_INVALID;
}

static purc_variant_t commit_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    struct dvobj_sqlite_connection *conn = get_connection_from_root(root);
    bool ret = false;
    if (conn_exec_stmt(conn, "COMMIT") < 0) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "sqlite error message is %s", sqlite3_errmsg(conn->db));
        goto out;
    }

    if (conn_exec_stmt(conn, "BEGIN") < 0) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "sqlite error message is %s", sqlite3_errmsg(conn->db));
        goto out;
    }

    ret = true;
out:
    return purc_variant_make_boolean(ret);
}

static purc_variant_t rollback_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    struct dvobj_sqlite_connection *conn = get_connection_from_root(root);
    bool ret = false;
    if (conn_exec_stmt(conn, "ROLLBACK") < 0) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "sqlite error message is %s", sqlite3_errmsg(conn->db));
        goto out;
    }

    if (conn_exec_stmt(conn, "BEGIN") < 0) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "sqlite error message is %s", sqlite3_errmsg(conn->db));
        goto out;
    }

    ret = true;
out:
    return purc_variant_make_boolean(ret);
}

static purc_variant_t close_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;

    struct dvobj_sqlite_connection *conn = get_connection_from_root(root);
    int rc = sqlite3_close(conn->db);
    bool ret = false;
    if (rc == SQLITE_OK) {
        ret = true;
        conn->db = NULL;
    }

    return purc_variant_make_boolean(ret);
}

static purc_variant_t execute_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    return PURC_VARIANT_INVALID;
}

static purc_variant_t executemany_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    return PURC_VARIANT_INVALID;
}
/* $SQLiteConnect end */

/* $SQLITE begin */
static inline struct dvobj_sqlite_info *
get_sqlite_info_from_root(purc_variant_t root)
{
    purc_variant_t v;

    v = purc_variant_object_get_by_ckey(root, SQLITE_KEY_HANDLE);
    assert(v && purc_variant_is_native(v));

    return (struct dvobj_sqlite_info *)purc_variant_native_get_entity(v);
}

static purc_variant_t make_impl_object(void)
{
    static const char *kvs[] = {
        "vendor",
        "HVML Community",
        "author",
        "Nine Xue",
        "verName",
        SQLITE_DVOBJ_VERNAME,
        "verCode",
        SQLITE_DVOBJ_VERCODE_STR,
        "license",
        "LGPLv3+",
        "url",
        "https://hvml.fmsoft.cn",
        "repo",
        "https://github.com/HVML",
    };

    purc_variant_t retv, val = PURC_VARIANT_INVALID;
    retv = purc_variant_make_object_0();
    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    assert(PCA_TABLESIZE(kvs) % 2 == 0);
    for (size_t i = 0; i < PCA_TABLESIZE(kvs); i += 2) {
        val = purc_variant_make_string_static(kvs[i+1], false);
        if (val == PURC_VARIANT_INVALID) {
            goto fatal;
        }
        if (!purc_variant_object_set_by_static_ckey(retv, kvs[i], val)) {
            goto fatal;
        }
        purc_variant_unref(val);
    }

    return retv;

fatal:
    if (val) {
        purc_variant_unref(val);
    }
    if (retv) {
        purc_variant_unref(retv);
    }

    return PURC_VARIANT_INVALID;
}

static const char *sqlite3_get_version(void)
{
    return SQLITE_VERSION;
}

static const char *sqlite3_get_platform(void)
{
    return SQLITE_PLATFORM;
}

static const char *sqlite3_get_copyright(void)
{
    return "unknown";
}

static const char *sqlite3_get_compiler(void)
{
    return "unknown";
}

static const char *sqlite3_get_buildinfo(void)
{
    return SQLITE_SOURCE_ID;
}

static purc_variant_t make_info_object(void)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val  = PURC_VARIANT_INVALID;

    retv = purc_variant_make_object_0();
    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    val = purc_variant_make_string_static(sqlite3_get_version(), false);
    if (val == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    if (!purc_variant_object_set_by_static_ckey(retv,
                SQLITE_INFO_VERSION, val)) {
        goto fatal;
    }
    purc_variant_unref(val);

    val = purc_variant_make_string_static(sqlite3_get_platform(), false);
    if (val == PURC_VARIANT_INVALID) {
        goto fatal;
    }
    if (!purc_variant_object_set_by_static_ckey(retv,
                SQLITE_INFO_PLATFORM, val)) {
        goto fatal;
    }
    purc_variant_unref(val);

    val = purc_variant_make_string_static(sqlite3_get_copyright(), false);
    if (val == PURC_VARIANT_INVALID) {
        goto fatal;
    }
    if (!purc_variant_object_set_by_static_ckey(retv,
                SQLITE_INFO_COPYRIGHT, val)) {
        goto fatal;
    }
    purc_variant_unref(val);

    val = purc_variant_make_string_static(sqlite3_get_compiler(), false);
    if (val == PURC_VARIANT_INVALID) {
        goto fatal;
    }
    if (!purc_variant_object_set_by_static_ckey(retv,
                SQLITE_INFO_COMPILER, val)) {
        goto fatal;
    }
    purc_variant_unref(val);

    val = purc_variant_make_string_static(sqlite3_get_buildinfo(), false);
    if (val == PURC_VARIANT_INVALID) {
        goto fatal;
    }
    if (!purc_variant_object_set_by_static_ckey(retv,
                SQLITE_INFO_BUILD_INFO, val)) {
        goto fatal;
    }
    purc_variant_unref(val);

    return retv;

fatal:
    if (val) {
        purc_variant_unref(val);
    }
    if (retv) {
        purc_variant_unref(retv);
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t connect_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;

    static struct purc_dvobj_method methods[] = {
        { SQLITE_KEY_CURSOR,                cursor_getter,          NULL },
        { SQLITE_KEY_COMMIT,                commit_getter,          NULL },
        { SQLITE_KEY_ROLLBACK,              rollback_getter,        NULL },
        { SQLITE_KEY_CLOSE,                 close_getter,           NULL },
        { SQLITE_KEY_EXECUTE,               execute_getter,         NULL },
        { SQLITE_KEY_EXECUTEMANY,           executemany_getter,     NULL },
    };

    const char *db_name;
    size_t db_name_len;
    purc_variant_t connect;
    purc_variant_t val;
    struct dvobj_sqlite_info *sqlite_info;
    struct dvobj_sqlite_connection *sqlite_connection;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    connect = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
    if (connect == PURC_VARIANT_INVALID) {
        goto failed;
    }

    sqlite_info = get_sqlite_info_from_root(root);
    db_name = purc_variant_get_string_const(argv[0]);
    db_name = pcutils_trim_spaces(db_name, &db_name_len);
    if (db_name_len == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    sqlite_connection = create_connection(sqlite_info, db_name);
    if (!sqlite_connection) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    sqlite_connection->root = connect;;

    if ((val = purc_variant_make_native((void *)sqlite_info, NULL)) == NULL) {
        goto failed;
    }

    if (!purc_variant_object_set_by_static_ckey(connect, SQLITE_KEY_HANDLE,
                val)) {
        goto failed;
    }
    purc_variant_unref(val);

    sqlite_info->listener = purc_variant_register_post_listener(connect,
            PCVAR_OPERATION_RELEASING, on_sqlite_connection_being_released,
            sqlite_info);

    return connect;
failed:
    if (connect) {
        purc_variant_unref(connect);
        connect = NULL;
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_undefined();
    }

    return PURC_VARIANT_INVALID;
}

static bool on_sqlite_being_released(purc_variant_t src, pcvar_op_t op,
        void *ctxt, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (op == PCVAR_OPERATION_RELEASING) {
        struct dvobj_sqlite_info *sqlite_info = ctxt;
        purc_variant_revoke_listener(src, sqlite_info->listener);
        free(sqlite_info);
    }

    return true;
}

static purc_variant_t create_sqlite(void)
{
    static struct purc_dvobj_method methods[] = {
        { SQLITE_KEY_CONNECT,           connect_getter,         NULL },
    };

    purc_variant_t sqlite = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    sqlite = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
    if (sqlite == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    struct dvobj_sqlite_info *sqlite_info = NULL;
    if (!sqlite) {
        goto fatal;
    }

    sqlite_info = calloc(1, sizeof(*sqlite_info));
    if (sqlite_info == NULL) {
        goto failed_info;
    }

    sqlite_info->root = sqlite;

    /* $SQLITE.impl */
    if ((val = make_impl_object()) == PURC_VARIANT_INVALID) {
        goto fatal;
    }
    if (!purc_variant_object_set_by_static_ckey(sqlite, SQLITE_KEY_IMPL, val)) {
        goto fatal;
    }
    purc_variant_unref(val);

    /* $SQLITE.info */
    if ((val = make_info_object()) == PURC_VARIANT_INVALID) {
        goto fatal;
    }
    if (!purc_variant_object_set_by_static_ckey(sqlite, SQLITE_KEY_INFO, val)) {
        goto fatal;
    }
    purc_variant_unref(val);

    if ((val = purc_variant_make_native((void *)sqlite_info, NULL)) == NULL) {
        goto fatal;
    }

    if (!purc_variant_object_set_by_static_ckey(sqlite, SQLITE_KEY_HANDLE, val)) {
        goto fatal;
    }
    purc_variant_unref(val);

    sqlite_info->listener = purc_variant_register_post_listener(sqlite,
            PCVAR_OPERATION_RELEASING, on_sqlite_being_released, sqlite_info);

    return sqlite;

failed_info:
    if (sqlite_info) {
        free(sqlite_info);
    }

fatal:
    if (val) {
        purc_variant_unref(val);
    }

    if (sqlite) {
        purc_variant_unref(sqlite);
    }

    pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
    return PURC_VARIANT_INVALID;
}
/* $SQLITE end */

static struct dvobj_info {
    const char *name;
    const char *desc;
    purc_variant_t (*create_func)(void);
} dvobjs[] = {
    {
        "SQLITE",                                       // name
        "Implementaion of $SQLITE based on sqlite3",    // description
        create_sqlite                                   // create function
    },
};

purc_variant_t __purcex_load_dynamic_variant(const char *name, int *ver_code)
{
    size_t i = 0;
    for (i = 0; i < PCA_TABLESIZE(dvobjs); i++) {
        if (strcasecmp(name, dvobjs[i].name) == 0)
            break;
    }

    if (i == PCA_TABLESIZE(dvobjs)) {
        return PURC_VARIANT_INVALID;
    }
    else {
        *ver_code = SQLITE_DVOBJ_VERCODE;
        return dvobjs[i].create_func();
    }
}

size_t __purcex_get_number_of_dynamic_variants(void)
{
    return PCA_TABLESIZE(dvobjs);
}

const char *__purcex_get_dynamic_variant_name(size_t idx)
{
    if (idx >= PCA_TABLESIZE(dvobjs)) {
        return NULL;
    }
    else {
        return dvobjs[idx].name;
    }
}

const char *__purcex_get_dynamic_variant_desc(size_t idx)
{
    if (idx >= PCA_TABLESIZE(dvobjs)) {
        return NULL;
    }
    else {
        return dvobjs[idx].desc;
    }
}

