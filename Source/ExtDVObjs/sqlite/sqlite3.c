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
#define SQLITE_INFO_BUILD_INFO      "build-info"

#define _KW_DELIMITERS              " \t\n\v\f\r"

#define SQLITE_UTC                  "1970-01-01 00:00:00.000"
#define SQLITE_JULIAN               2440587.5

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
    bool                            closed;
    bool                            locked;
    bool                            is_dml;
    long                            rowcount;
    int64_t                         lastrowid;
    purc_variant_t                  root;               // the root variant, itself
    purc_variant_t                  description;        // description attr
    struct dvobj_sqlite_connection  *conn;
    struct pcvar_listener           *listener;          // the listener
    sqlite3_stmt                    *st;
};

static bool is_conn_closed(struct dvobj_sqlite_connection *conn)
{
    return !conn->db;
}

static bool is_cursor_closed(struct dvobj_sqlite_cursor *cursor)
{
    return cursor->closed;
}

static bool is_cursor_locked(struct dvobj_sqlite_cursor *cursor)
{
    return cursor->locked;
}

/* $SQLiteCursor begin */
static bool check_cursor(struct dvobj_sqlite_cursor *cursor)
{
    bool ret = false;
    if (is_conn_closed(cursor->conn)) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "can not operate on a closed database");
        goto out;
    }

    if (is_cursor_closed(cursor)) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "can not operate on a closed cursor");
        goto out;
    }

    if (is_cursor_locked(cursor)) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "Recursive use of cursors not allowed.");
        goto out;
    }

    ret = true;

out:
    return ret;
}

static inline const char *
lstrip_sql(const char *sql)
{
    // This loop is borrowed from the SQLite source code.
    for (const char *pos = sql; *pos; pos++) {
        switch (*pos) {
            case ' ':
            case '\t':
            case '\f':
            case '\n':
            case '\r':
                // Skip whitespace.
                break;
            case '-':
                // Skip line comments.
                if (pos[1] == '-') {
                    pos += 2;
                    while (pos[0] && pos[0] != '\n') {
                        pos++;
                    }
                    if (pos[0] == '\0') {
                        return NULL;
                    }
                    continue;
                }
                return pos;
            case '/':
                // Skip C style comments.
                if (pos[1] == '*') {
                    pos += 2;
                    while (pos[0] && (pos[0] != '*' || pos[1] != '/')) {
                        pos++;
                    }
                    if (pos[0] == '\0') {
                        return NULL;
                    }
                    pos++;
                    continue;
                }
                return pos;
            default:
                return pos;
        }
    }

    return NULL;
}

static inline int
cursor_create_st(struct dvobj_sqlite_cursor *cursor, const char *sql)
{
    struct dvobj_sqlite_connection *conn = cursor->conn;
    sqlite3 *db = conn->db;
    size_t size = strlen(sql);
    int max_length = sqlite3_limit(db, SQLITE_LIMIT_SQL_LENGTH, -1);
    if (size > (size_t)max_length) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "Query string is too large.");
        return -1;
    }

    sqlite3_stmt *stmt = NULL;
    const char *tail;
    int rc;
    rc = sqlite3_prepare_v2(db, sql, (int)size + 1, &stmt, &tail);

    if (rc != SQLITE_OK) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "sqlite error message is %s", sqlite3_errmsg(conn->db));
        return -1;
    }

    if (lstrip_sql(tail) != NULL) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                        "You can only execute one statement at a time.");
        goto error;
    }

    bool is_dml = false;
    const char *p = lstrip_sql(sql);
    if (p != NULL) {
        is_dml = (strncasecmp(p, "insert", 6) == 0)
                  || (strncasecmp(p, "update", 6) == 0)
                  || (strncasecmp(p, "delete", 6) == 0)
                  || (strncasecmp(p, "replace", 7) == 0);
    }

    cursor->st = stmt;
    cursor->is_dml = is_dml;

    return 0;

error:
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    return -1;
}

static int
bind_param(struct dvobj_sqlite_cursor *cursor, int pos,
        purc_variant_t parameter)
{
    int rc = SQLITE_OK;
    const char *string;
    size_t nr_string;
    enum purc_variant_type paramtype;

    if (purc_variant_is_null(parameter)) {
        rc = sqlite3_bind_null(cursor->st, pos);
        goto out;
    }

    switch (paramtype) {
        case PURC_VARIANT_TYPE_LONGINT: {
            int64_t value;
            if (!purc_variant_cast_to_longint(parameter, &value, false)) {
                rc = -1;
            }
            else {
                rc = sqlite3_bind_int64(cursor->st, pos, (sqlite3_int64)value);
            }
            break;
        }
        case PURC_VARIANT_TYPE_NUMBER: {
            double value;
            if (!purc_variant_cast_to_number(parameter, &value, false)) {
                rc = -1;
            }
            else {
                rc = sqlite3_bind_double(cursor->st, pos, value);
            }
            break;
        }
        case PURC_VARIANT_TYPE_STRING:
            string = purc_variant_get_string_const_ex(parameter, &nr_string);
            if (string == NULL) {
                rc = -1;
            }

            if (nr_string > INT_MAX) {
                purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                                "string longer than INT_MAX bytes");
                goto out;
            }
            rc = sqlite3_bind_text(cursor->st, pos, string, (int)nr_string,
                    SQLITE_TRANSIENT);
            break;
        case PURC_VARIANT_TYPE_BSEQUENCE: {
            size_t nr_bytes;
            const unsigned char *bytes = purc_variant_get_bytes_const(
                    parameter, &nr_bytes);
            if (!bytes || nr_bytes == 0) {
                purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                                "invalid BLOB data");
                goto out;
            }

            if (nr_bytes > INT_MAX) {
                purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                                "BLOB longer than INT_MAX bytes");
                goto out;
            }
            rc = sqlite3_bind_blob(cursor->st, pos, bytes, nr_bytes, SQLITE_TRANSIENT);
            break;
        }
        default:
            purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                    "Error binding parameter %d: type '%s' is not supported",
                    pos, purc_variant_typename(paramtype));
            rc = -1;
    }

out:
    return rc;
}

static int
bind_parameters(struct dvobj_sqlite_cursor *cursor, purc_variant_t parameters)
{
    int i;
    int rc = -1;
    int num_params_needed;
    ssize_t num_params;
    purc_variant_t current_param;

    num_params_needed = sqlite3_bind_parameter_count(cursor->st);
    num_params = purc_variant_array_get_size(parameters);

    if (num_params != num_params_needed) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "Incorrect number of bindings supplied. The current "
                "statement uses %d, and there are %zd supplied.",
                num_params_needed, num_params);
        goto out;
    }

    for (i = 0; i < num_params; i++) {
        const char *name = sqlite3_bind_parameter_name(cursor->st, i+1);
        if (!name) {
            purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                    "sqlite error message is %s", sqlite3_errmsg(cursor->conn->db));
            goto out;
        }

        current_param = purc_variant_array_get(parameters, i);

        rc = bind_param(cursor, i + 1, current_param);

        if (rc != SQLITE_OK) {
            purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                    "sqlite error message is %s", sqlite3_errmsg(cursor->conn->db));
            goto out;
        }
    }

out:
    return rc;
}

static inline int
cursor_exec_query(struct dvobj_sqlite_cursor *cursor, bool multiple,
        const char *sql, purc_variant_t param)
{
    (void) cursor;
    (void) multiple;
    (void) sql;
    (void) param;
    int rc;
    int ret = -1;
    purc_variant_t param_array = PURC_VARIANT_INVALID;
    if (!check_cursor(cursor)) {
        goto failed;
    }

    /* Prevent recursive use of cursors. */
    cursor->locked = 1;

    if (multiple) {
        if (param) {
            param_array = purc_variant_ref(param);
        }
        else {
            param_array = purc_variant_make_array(0, PURC_VARIANT_INVALID);
        }
    }
    else {
        size_t nr_param = param ? 1 : 0;
        param_array = purc_variant_make_array(nr_param, param);
    }

    /* reset description */
    if (cursor->description) {
        purc_variant_unref(cursor->description);
        cursor->description = PURC_VARIANT_INVALID;
    }

    /* reset stmt : FIXME */
    if (cursor->st) {
        sqlite3_reset(cursor->st);
        sqlite3_finalize(cursor->st);
        cursor->st = NULL;
    }

    rc = cursor_create_st(cursor, sql);
    if (rc != 0) {
        goto failed;
    }

    if (multiple && sqlite3_stmt_readonly(cursor->st)) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                        "executemany() can only execute DML statements.");
        goto failed;
    }

    sqlite3_reset(cursor->st);
    /* reset rowcount */
    cursor->rowcount = cursor->is_dml ? 0L : -1L;

    assert(!sqlite3_stmt_busy(cursor->st));

    ssize_t nr_param_array = purc_variant_array_get_size(param_array);
    for (ssize_t i = 0; i < nr_param_array; i++) {
        purc_variant_t val = purc_variant_array_get(param_array, i);
        if (!purc_variant_is_array(val)) {
            purc_set_error_with_info(PURC_ERROR_WRONG_DATA_TYPE,
                            "execute/executemany param is not array.");
            goto failed;
        }

        /* bind param */
        ret = bind_parameters(cursor, val);
        if (ret != 0) {
            goto failed;
        }

        rc = sqlite3_step(cursor->st);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                    "sqlite error message is %s",
                    sqlite3_errmsg(cursor->conn->db));
            goto failed;
        }

        int numcols = sqlite3_column_count(cursor->st);
        if (cursor->description == PURC_VARIANT_INVALID && numcols > 0) {
            cursor->description = purc_variant_make_tuple(numcols, NULL);
            if (!cursor->description) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto failed;
            }
            for (i = 0; i < numcols; i++) {
                const char *colname;
                colname = sqlite3_column_name(cursor->st, i);
                if (colname == NULL) {
                    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                    goto failed;
                }
                purc_variant_t val = purc_variant_make_string(colname, true);
                if (!val) {
                    goto failed;
                }
                purc_variant_tuple_set(cursor->description, i, val);
                purc_variant_unref(val);
            }
        }

        if (rc == SQLITE_DONE) {
            if (cursor->is_dml) {
                cursor->rowcount += (long)sqlite3_changes(cursor->conn->db);
            }
            sqlite3_reset(cursor->st);
        }
    }

    if (!multiple) {
        cursor->lastrowid = sqlite3_last_insert_rowid(cursor->conn->db);
    }

failed:
    cursor->locked = 0;

    if (param_array) {
        purc_variant_unref(param_array);
    }
    return ret;
}

enum affinity_type_enum {
    SQLITE_AFFINITY_TYPE_FIRST = 0,

    SQLITE_AFFINITY_TYPE_BIGINT = SQLITE_AFFINITY_TYPE_FIRST,
#define SQLITE_AFFINITY_TYPE_NAME_BIGINT                    "bigint"
    SQLITE_AFFINITY_TYPE_BINARY,
#define SQLITE_AFFINITY_TYPE_NAME_BINARY                    "binary"
    SQLITE_AFFINITY_TYPE_BIT,
#define SQLITE_AFFINITY_TYPE_NAME_BIT                       "bit"
    SQLITE_AFFINITY_TYPE_BLOB,
#define SQLITE_AFFINITY_TYPE_NAME_BLOB                      "blob"
    SQLITE_AFFINITY_TYPE_BOOLEAN,
#define SQLITE_AFFINITY_TYPE_NAME_BOOLEAN                   "boolean"
    SQLITE_AFFINITY_TYPE_CHARACTER,
#define SQLITE_AFFINITY_TYPE_NAME_CHARACTER                 "character"
    SQLITE_AFFINITY_TYPE_CLOB,
#define SQLITE_AFFINITY_TYPE_NAME_CLOB                      "clob"
    SQLITE_AFFINITY_TYPE_DATE,
#define SQLITE_AFFINITY_TYPE_NAME_DATE                      "date"
    SQLITE_AFFINITY_TYPE_DATETIME,
#define SQLITE_AFFINITY_TYPE_NAME_DATETIME                  "datetime"
    SQLITE_AFFINITY_TYPE_DECIMAL,
#define SQLITE_AFFINITY_TYPE_NAME_DECIMAL                   "decimal"
    SQLITE_AFFINITY_TYPE_DOUBLE,
#define SQLITE_AFFINITY_TYPE_NAME_DOUBLE                    "double"
    SQLITE_AFFINITY_TYPE_DOUBLE_PRECISION,
#define SQLITE_AFFINITY_TYPE_NAME_DOUBLE_PRECISION          "double precision"
    SQLITE_AFFINITY_TYPE_FLOAT,
#define SQLITE_AFFINITY_TYPE_NAME_FLOAT                     "float"
    SQLITE_AFFINITY_TYPE_INT,
#define SQLITE_AFFINITY_TYPE_NAME_INT                       "int"
    SQLITE_AFFINITY_TYPE_INT2,
#define SQLITE_AFFINITY_TYPE_NAME_INT2                      "int2"
    SQLITE_AFFINITY_TYPE_INT4,
#define SQLITE_AFFINITY_TYPE_NAME_INT4                      "int4"
    SQLITE_AFFINITY_TYPE_INT8,
#define SQLITE_AFFINITY_TYPE_NAME_INT8                      "int8"
    SQLITE_AFFINITY_TYPE_INTEGER,
#define SQLITE_AFFINITY_TYPE_NAME_INTEGER                   "integer"
    SQLITE_AFFINITY_TYPE_MEDIUMINT,
#define SQLITE_AFFINITY_TYPE_NAME_MEDIUMINT                 "mediumint"
    SQLITE_AFFINITY_TYPE_NATIVE_CHARACTER,
#define SQLITE_AFFINITY_TYPE_NAME_NATIVE_CHARACTER          "native character"
    SQLITE_AFFINITY_TYPE_NCHAR,
#define SQLITE_AFFINITY_TYPE_NAME_NCHAR                     "nchar"
    SQLITE_AFFINITY_TYPE_NUMERIC,
#define SQLITE_AFFINITY_TYPE_NAME_NUMERIC                   "numeric"
    SQLITE_AFFINITY_TYPE_NVARCHAR,
#define SQLITE_AFFINITY_TYPE_NAME_NVARCHAR                  "nvarchar"
    SQLITE_AFFINITY_TYPE_REAL,
#define SQLITE_AFFINITY_TYPE_NAME_REAL                      "real"
    SQLITE_AFFINITY_TYPE_SMALLINT,
#define SQLITE_AFFINITY_TYPE_NAME_SMALLINT                  "smallint"
    SQLITE_AFFINITY_TYPE_TEXT,
#define SQLITE_AFFINITY_TYPE_NAME_TEXT                      "text"
    SQLITE_AFFINITY_TYPE_TINYINT,
#define SQLITE_AFFINITY_TYPE_NAME_TINYINT                   "tinyint"
    SQLITE_AFFINITY_TYPE_UNSIGNED_BIG_INT,
#define SQLITE_AFFINITY_TYPE_NAME_UNSIGNED_BIG_INT          "unsigned big int"
    SQLITE_AFFINITY_TYPE_VARCHAR,
#define SQLITE_AFFINITY_TYPE_NAME_VARCHAR                   "varchar"
    SQLITE_AFFINITY_TYPE_VARYING_CHARACTER,
#define SQLITE_AFFINITY_TYPE_NAME_VARYING_CHARACTER          "varying character"

    /* XXX: change this when you append a new type */
    SQLITE_AFFINITY_TYPE_LAST = SQLITE_AFFINITY_TYPE_VARYING_CHARACTER,
};

#define SQLITE_NR_AFFINITY_TYPES \
    (SQLITE_AFFINITY_TYPE_LAST - SQLITE_AFFINITY_TYPE_FIRST + 1)

static struct affinity_type {
    const char *type_name;
    enum affinity_type_enum affinity;
    purc_variant_type type;
} affinitys[] = {
    {
        SQLITE_AFFINITY_TYPE_NAME_BIGINT,
        SQLITE_AFFINITY_TYPE_BIGINT,
        PURC_VARIANT_TYPE_LONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_BINARY,
        SQLITE_AFFINITY_TYPE_BINARY,
        PURC_VARIANT_TYPE_BSEQUENCE
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_BIT,
        SQLITE_AFFINITY_TYPE_BIT,
        PURC_VARIANT_TYPE_BOOLEAN
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_BLOB,
        SQLITE_AFFINITY_TYPE_BLOB,
        PURC_VARIANT_TYPE_BSEQUENCE
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_BOOLEAN,
        SQLITE_AFFINITY_TYPE_BOOLEAN,
        PURC_VARIANT_TYPE_BOOLEAN
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_CHARACTER,
        SQLITE_AFFINITY_TYPE_CHARACTER,
        PURC_VARIANT_TYPE_STRING
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_CLOB,
        SQLITE_AFFINITY_TYPE_CLOB,
        PURC_VARIANT_TYPE_STRING
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_DATE,
        SQLITE_AFFINITY_TYPE_DATE,
        PURC_VARIANT_TYPE_STRING
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_DATETIME,
        SQLITE_AFFINITY_TYPE_DATETIME,
        PURC_VARIANT_TYPE_STRING
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_DECIMAL,
        SQLITE_AFFINITY_TYPE_DECIMAL,
        PURC_VARIANT_TYPE_NUMBER
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_DOUBLE,
        SQLITE_AFFINITY_TYPE_DOUBLE,
        PURC_VARIANT_TYPE_NUMBER
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_DOUBLE_PRECISION,
        SQLITE_AFFINITY_TYPE_DOUBLE_PRECISION,
        PURC_VARIANT_TYPE_NUMBER
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_FLOAT,
        SQLITE_AFFINITY_TYPE_FLOAT,
        PURC_VARIANT_TYPE_NUMBER
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_INT,
        SQLITE_AFFINITY_TYPE_INT,
        PURC_VARIANT_TYPE_LONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_INT2,
        SQLITE_AFFINITY_TYPE_INT2,
        PURC_VARIANT_TYPE_LONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_INT4,
        SQLITE_AFFINITY_TYPE_INT4,
        PURC_VARIANT_TYPE_LONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_INT8,
        SQLITE_AFFINITY_TYPE_INT8,
        PURC_VARIANT_TYPE_LONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_INTEGER,
        SQLITE_AFFINITY_TYPE_INTEGER,
        PURC_VARIANT_TYPE_LONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_MEDIUMINT,
        SQLITE_AFFINITY_TYPE_MEDIUMINT,
        PURC_VARIANT_TYPE_LONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_NATIVE_CHARACTER,
        SQLITE_AFFINITY_TYPE_NATIVE_CHARACTER,
        PURC_VARIANT_TYPE_STRING
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_NCHAR,
        SQLITE_AFFINITY_TYPE_NCHAR,
        PURC_VARIANT_TYPE_STRING
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_NUMERIC,
        SQLITE_AFFINITY_TYPE_NUMERIC,
        PURC_VARIANT_TYPE_NUMBER
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_NVARCHAR,
        SQLITE_AFFINITY_TYPE_NVARCHAR,
        PURC_VARIANT_TYPE_STRING
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_REAL,
        SQLITE_AFFINITY_TYPE_REAL,
        PURC_VARIANT_TYPE_NUMBER
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_SMALLINT,
        SQLITE_AFFINITY_TYPE_SMALLINT,
        PURC_VARIANT_TYPE_LONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_TEXT,
        SQLITE_AFFINITY_TYPE_TEXT,
        PURC_VARIANT_TYPE_STRING
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_TINYINT,
        SQLITE_AFFINITY_TYPE_TINYINT,
        PURC_VARIANT_TYPE_LONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_UNSIGNED_BIG_INT,
        SQLITE_AFFINITY_TYPE_UNSIGNED_BIG_INT,
        PURC_VARIANT_TYPE_ULONGINT
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_VARCHAR,
        SQLITE_AFFINITY_TYPE_VARCHAR,
        PURC_VARIANT_TYPE_STRING
    },
    {
        SQLITE_AFFINITY_TYPE_NAME_VARYING_CHARACTER,
        SQLITE_AFFINITY_TYPE_VARYING_CHARACTER,
        PURC_VARIANT_TYPE_STRING
    },
};

/* Make sure the number of handlers matches the number of operations */
#define _COMPILE_TIME_ASSERT(name, x)               \
     typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(tps,
      sizeof(affinitys)/sizeof(affinitys[0]) == SQLITE_NR_AFFINITY_TYPES);
#undef _COMPILE_TIME_ASSERT

static struct affinity_type *find_affinity_type(const char* type_name)
{
    static ssize_t max = sizeof(affinitys)/sizeof(affinitys[0]) - 1;

    ssize_t low = 0, high = max, mid;
    while (low <= high) {
        int cmp;

        mid = (low + high) / 2;
        cmp = strcasecmp(type_name, affinitys[mid].type_name);
        if (cmp == 0) {
            goto found;
        }
        else {
            if (cmp < 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
    }

    return NULL;

found:
    return &affinitys[mid];
}

static inline bool is_date(struct affinity_type *type)
{
    if (type && (type->affinity == SQLITE_AFFINITY_TYPE_DATE ||
                type->affinity == SQLITE_AFFINITY_TYPE_DATETIME)) {
        return true;
    }
    return false;
}

static purc_variant_t sqlite_value_to_variant(sqlite3 *db, sqlite3_stmt *st,
        int pos)
{
    purc_variant_t val;
    int col_type = sqlite3_column_type(st, pos);
    switch (col_type) {
    case SQLITE_NULL: {
        val = purc_variant_make_null();
        break;
    }

    case SQLITE_INTEGER: {
        val = purc_variant_make_longint(sqlite3_column_int64(st, pos));
        break;
    }

    case SQLITE_FLOAT: {
        val = purc_variant_make_number(sqlite3_column_double(st, pos));
        break;
    }

    case SQLITE3_TEXT: {
        const char *text = (const char*)sqlite3_column_text(st, pos);
        if (text == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto fatal;
        }
        val = purc_variant_make_string(text, true);
        break;
    }

    case SQLITE_BLOB: {
        const void *blob = sqlite3_column_blob(st, pos);
        if (blob == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto fatal;
        }

        int nr_blob = sqlite3_column_bytes(st, pos);
        val = purc_variant_make_byte_sequence(blob, nr_blob);
        break;
    }

    default: {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
              "invalid sqllite3 column type %d", col_type);
        break;
    }
    }

fatal:
    return val;
}

static purc_variant_t sqlite_null_cast_to_variant(sqlite3 *db,
        sqlite3_stmt *st, int pos, struct affinity_type *dest_type)
{
    (void) db;
    (void) st;
    (void) pos;
    purc_variant_t val = PURC_VARIANT_INVALID;

    purc_variant_type type = PURC_VARIANT_TYPE_NULL;
    if (dest_type) {
        type = dest_type->type;
    }

    switch (type) {
    case PURC_VARIANT_TYPE_NULL:
        val = purc_variant_make_null();
        break;

    case PURC_VARIANT_TYPE_LONGINT:
        val = purc_variant_make_longint(0);
        break;

    case PURC_VARIANT_TYPE_ULONGINT:
        val = purc_variant_make_ulongint(0);
        break;

    case PURC_VARIANT_TYPE_NUMBER:
        val = purc_variant_make_number(0.0f);
        break;

    case PURC_VARIANT_TYPE_BOOLEAN:
        val = purc_variant_make_boolean(false);
        break;

    case PURC_VARIANT_TYPE_STRING:
        if (is_date(dest_type)) {
            val = purc_variant_make_string(SQLITE_UTC, false);
            break;
        }
        val = purc_variant_make_string(NULL, false);
        break;

    case PURC_VARIANT_TYPE_BSEQUENCE:
        val = purc_variant_make_byte_sequence_empty();
        break;

    default: {
        /* never reatch here */
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
              "invalid affinity type %s", dest_type->type_name);
        assert(0);
        break;
    }
    }

    return val;
}

static purc_variant_t sqlite_integer_cast_to_variant(sqlite3 *db,
        sqlite3_stmt *st, int pos, struct affinity_type *dest_type)
{
    purc_variant_t val = PURC_VARIANT_INVALID;

    int64_t v = sqlite3_column_int64(st, pos);
    purc_variant_type type = PURC_VARIANT_TYPE_LONGINT;
    if (dest_type) {
        type = dest_type->type;
    }

    switch (type) {
    case PURC_VARIANT_TYPE_NULL:
        val = purc_variant_make_null();
        break;

    case PURC_VARIANT_TYPE_LONGINT:
        val = purc_variant_make_longint(v);
        break;

    case PURC_VARIANT_TYPE_ULONGINT:
        val = purc_variant_make_ulongint(v);
        break;

    case PURC_VARIANT_TYPE_NUMBER:
        val = purc_variant_make_number(v);
        break;

    case PURC_VARIANT_TYPE_BOOLEAN:
        val = purc_variant_make_boolean(v != 0);
        break;

    case PURC_VARIANT_TYPE_STRING: {
        if (is_date(dest_type)) {
            struct timeval tv;
            tv.tv_sec = (time_t)v;
            tv.tv_usec = 0;
            struct tm tm;
            gmtime_r(&tv.tv_sec, &tm);
            char buf[32];
            strftime(buf, 32, "%F %T", &tm);
            val = purc_variant_make_string(buf, false);
            break;
        }

        const char *text = (const char*)sqlite3_column_text(st, pos);
        if (text == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }
        val = purc_variant_make_string(text, true);
        break;
    }

    case PURC_VARIANT_TYPE_BSEQUENCE: {
        const void *blob = sqlite3_column_blob(st, pos);
        if (blob == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }

        int nr_blob = sqlite3_column_bytes(st, pos);
        val = purc_variant_make_byte_sequence(blob, nr_blob);
        break;
    }

    default: {
        /* never reatch here */
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
              "invalid affinity type %s", dest_type->type_name);
        assert(0);
    }

    }

out:
    return val;
}

static purc_variant_t sqlite_float_cast_to_variant(sqlite3 *db,
        sqlite3_stmt *st, int pos, struct affinity_type *dest_type)
{
    purc_variant_t val = PURC_VARIANT_INVALID;

    double v = sqlite3_column_double(st, pos);
    purc_variant_type type = PURC_VARIANT_TYPE_NUMBER;
    if (dest_type) {
        type = dest_type->type;
    }

    switch (type) {
    case PURC_VARIANT_TYPE_NULL:
        val = purc_variant_make_null();
        break;

    case PURC_VARIANT_TYPE_LONGINT:
        val = purc_variant_make_longint(v);
        break;

    case PURC_VARIANT_TYPE_ULONGINT:
        val = purc_variant_make_ulongint(v);
        break;

    case PURC_VARIANT_TYPE_NUMBER:
        val = purc_variant_make_number(v);
        break;

    case PURC_VARIANT_TYPE_BOOLEAN:
        val = purc_variant_make_boolean(v != 0);
        break;

    case PURC_VARIANT_TYPE_STRING: {
        if (is_date(dest_type)) {
            if (v > SQLITE_JULIAN) {
                int64_t unix_timestamp = (v - SQLITE_JULIAN) * 86400;

                struct timeval tv;
                tv.tv_sec = (time_t)unix_timestamp;
                tv.tv_usec = 0;
                struct tm tm;
                gmtime_r(&tv.tv_sec, &tm);
                char buf[32];
                strftime(buf, 32, "%F %T", &tm);
                val = purc_variant_make_string(buf, false);
                break;
            }
            else {
                purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                        "invalid julay value for date conversion '%f' less than '%f'",
                        v, SQLITE_JULIAN);
                break;
            }
        }

        const char *text = (const char*)sqlite3_column_text(st, pos);
        if (text == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }
        val = purc_variant_make_string(text, true);
        break;
    }

    case PURC_VARIANT_TYPE_BSEQUENCE: {
        const void *blob = sqlite3_column_blob(st, pos);
        if (blob == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }

        int nr_blob = sqlite3_column_bytes(st, pos);
        val = purc_variant_make_byte_sequence(blob, nr_blob);
        break;
    }

    default: {
        /* never reatch here */
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
              "invalid affinity type %s", dest_type->type_name);
        assert(0);
    }

    }

out:
    return val;
}

static purc_variant_t sqlite_text_cast_to_variant(sqlite3 *db,
        sqlite3_stmt *st, int pos, struct affinity_type *dest_type)
{
    purc_variant_t val = PURC_VARIANT_INVALID;

    const char *text = (const char*)sqlite3_column_text(st, pos);
    if (text == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    purc_variant_type type = PURC_VARIANT_TYPE_STRING;
    if (dest_type) {
        type = dest_type->type;
    }

    switch (type) {
    case PURC_VARIANT_TYPE_NULL:
        val = purc_variant_make_null();
        break;

    case PURC_VARIANT_TYPE_LONGINT:
        val = purc_variant_make_longint(sqlite3_column_int64(st, pos));
        break;

    case PURC_VARIANT_TYPE_ULONGINT:
        val = purc_variant_make_longint(sqlite3_column_int64(st, pos));
        break;

    case PURC_VARIANT_TYPE_NUMBER:
        val = purc_variant_make_number(sqlite3_column_double(st, pos));
        break;

    case PURC_VARIANT_TYPE_BOOLEAN: {
        size_t nr_text = strlen(text);
        val = purc_variant_make_boolean(nr_text > 0);
        break;
    }

    case PURC_VARIANT_TYPE_STRING: {
        val = purc_variant_make_string(text, true);
        break;
    }

    case PURC_VARIANT_TYPE_BSEQUENCE: {
        size_t nr_text = strlen(text);
        val = purc_variant_make_byte_sequence(text, nr_text);
        break;
    }

    default: {
        /* never reatch here */
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
              "invalid affinity type %s", dest_type->type_name);
        assert(0);
    }

    }

out:
    return val;
}

static purc_variant_t sqlite_blob_cast_to_variant(sqlite3 *db,
        sqlite3_stmt *st, int pos, struct affinity_type *dest_type)
{
    purc_variant_t val = PURC_VARIANT_INVALID;

    const void *blob = sqlite3_column_blob(st, pos);
    if (blob == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    int nr_blob = sqlite3_column_bytes(st, pos);
    purc_variant_type type = PURC_VARIANT_TYPE_BSEQUENCE;
    if (dest_type) {
        type = dest_type->type;
    }

    switch (type) {
    case PURC_VARIANT_TYPE_NULL:
        val = purc_variant_make_null();
        break;

    case PURC_VARIANT_TYPE_LONGINT:
        val = purc_variant_make_longint(sqlite3_column_int64(st, pos));
        break;

    case PURC_VARIANT_TYPE_ULONGINT:
        val = purc_variant_make_longint(sqlite3_column_int64(st, pos));
        break;

    case PURC_VARIANT_TYPE_NUMBER:
        val = purc_variant_make_number(sqlite3_column_double(st, pos));
        break;

    case PURC_VARIANT_TYPE_BOOLEAN:
        val = purc_variant_make_boolean(nr_blob > 0);
        break;

    case PURC_VARIANT_TYPE_STRING: {
        const char *text = (const char*)sqlite3_column_text(st, pos);
        if (text == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }
        val = purc_variant_make_string(text, true);
        break;
    }

    case PURC_VARIANT_TYPE_BSEQUENCE: {
        val = purc_variant_make_byte_sequence(blob, nr_blob);
        break;
    }

    default: {
        /* never reatch here */
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
              "invalid affinity type %s", dest_type->type_name);
        assert(0);
    }

    }

out:
    return val;
}

static purc_variant_t sqlite_value_to_variant_with_type(sqlite3 *db,
        sqlite3_stmt *st, int pos, const char *type_name)
{
    (void) db;
    (void) st;
    (void) pos;
    (void) type_name;

    purc_variant_t val = PURC_VARIANT_INVALID;
    struct affinity_type *dest_type = find_affinity_type(type_name);

    int col_type = sqlite3_column_type(st, pos);
    switch (col_type) {
    case SQLITE_NULL: {
        val = sqlite_null_cast_to_variant(db, st, pos, dest_type);
        break;
    }

    case SQLITE_INTEGER: {
        val = sqlite_integer_cast_to_variant(db, st, pos, dest_type);
        break;
    }

    case SQLITE_FLOAT: {
        val = sqlite_float_cast_to_variant(db, st, pos, dest_type);
        break;
    }

    case SQLITE3_TEXT: {
        val = sqlite_text_cast_to_variant(db, st, pos, dest_type);
        break;
    }

    case SQLITE_BLOB: {
        val = sqlite_blob_cast_to_variant(db, st, pos, dest_type);
        break;
    }

    default: {
        /* never reatch here */
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
              "invalid sqllite3 column type %d", col_type);
        break;
    }
    }

    return val;
}

static purc_variant_t cursor_fetch_one_row_as_tuple(
        struct dvobj_sqlite_cursor *cursor, int nr_cols)
{
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t row = purc_variant_make_tuple(nr_cols, NULL);
    if (!row) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    sqlite3 *db = cursor->conn->db;
    sqlite3_stmt *st = cursor->st;
    for (int i = 0; i < nr_cols; i++) {
        val = sqlite_value_to_variant(db, st, i);
        if (!val) {
            goto fatal;
        }
        purc_variant_tuple_set(row, i, val);
    }

    return row;

fatal:
    if (val) {
        purc_variant_unref(val);
    }
    if (row) {
        purc_variant_unref(row);
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t build_column_name(struct dvobj_sqlite_cursor *cursor,
        int pos, const char *name, purc_variant_t name_mapping)
{
    (void) cursor;
    (void) pos;
    (void) name;
    (void) name_mapping;
    purc_variant_t val = PURC_VARIANT_INVALID;
    if (!name_mapping) {
        val = purc_variant_make_string(name, true);
        goto out;
    }

    purc_variant_t v = purc_variant_object_get_by_ckey(name_mapping, name);
    if (!v) {
        val = purc_variant_make_string(name, true);
        goto out;
    }

    if (!purc_variant_is_string(v)){
        purc_set_error_with_info(PURC_ERROR_WRONG_DATA_TYPE,
                "wrong data type for name_mapping '%s' type '%s'",
                name, purc_variant_typename(purc_variant_get_type(v)));
        goto out;
    }

    size_t length;
    const char *str = purc_variant_get_string_const_ex(v, &length);
    if (length > 0) {
        val = purc_variant_ref(v);
    }
    else {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "invalid value for  name_mapping '%s' value '%s'",
                name, str);
        goto out;
    }

out:
    return val;
}

static purc_variant_t build_column_value(struct dvobj_sqlite_cursor *cursor,
        int pos, const char *name, purc_variant_t type_conversion)
{
    (void) cursor;
    (void) pos;
    (void) name;
    (void) type_conversion;
    purc_variant_t val = PURC_VARIANT_INVALID;
    if (!type_conversion) {
        val = sqlite_value_to_variant(cursor->conn->db, cursor->st, pos);
        goto out;
    }

    purc_variant_t v = purc_variant_object_get_by_ckey(type_conversion, name);
    if (!v) {
        val = sqlite_value_to_variant(cursor->conn->db, cursor->st, pos);
        goto out;
    }

    if (!purc_variant_is_string(v)) {
        purc_set_error_with_info(PURC_ERROR_WRONG_DATA_TYPE,
                "wrong data type for type conversion '%s' type '%s'",
                name, purc_variant_typename(purc_variant_get_type(v)));
        goto out;
    }

    size_t nr_dest_type;
    const char *dest_type = purc_variant_get_string_const_ex(v, &nr_dest_type);
    if (nr_dest_type == 0) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "invalid value for  name_mapping '%s' value '%s'",
                name, dest_type);
        goto out;
    }

    val = sqlite_value_to_variant_with_type(cursor->conn->db, cursor->st, pos,
            dest_type);

out:
    return val;
}

static purc_variant_t cursor_fetch_one_row_as_object(
        struct dvobj_sqlite_cursor *cursor, int nr_cols,
        purc_variant_t name_mapping, purc_variant_t type_conversion)
{
    purc_variant_t key = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t row = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (!row) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto fatal;
    }

    sqlite3_stmt *st = cursor->st;
    for (int i = 0; i < nr_cols; i++) {
        const char *col_name = sqlite3_column_name(st, i);
        if (col_name == NULL) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto fatal;
        }
        key = build_column_name(cursor, i, col_name, name_mapping);
        if (!key) {
            goto fatal;
        }

        val = build_column_value(cursor, i, col_name, type_conversion);
        if (!val) {
            goto fatal;
        }

        if (!purc_variant_object_set(row, key, val)) {
            goto fatal;
        }
    }

    return row;

fatal:
    if (val) {
        purc_variant_unref(val);
    }
    if (row) {
        purc_variant_unref(row);
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t cursor_fetch_one_row(struct dvobj_sqlite_cursor *cursor,
        purc_variant_type result_type, purc_variant_t name_mapping,
        purc_variant_t type_conversion)
{
    purc_variant_t row = PURC_VARIANT_INVALID;
    int nr_cols = sqlite3_data_count(cursor->st);
    if (nr_cols <= 0) {
        row = purc_variant_make_null();
        goto out;
    }

    if (result_type == PURC_VARIANT_TYPE_TUPLE) {
        row = cursor_fetch_one_row_as_tuple(cursor, nr_cols);
    }
    else {
        row = cursor_fetch_one_row_as_object(cursor, nr_cols,
                name_mapping, type_conversion);
    }

out:
    return row;
}

static purc_variant_t cursor_iterator_next(struct dvobj_sqlite_cursor *cursor,
        purc_variant_type result_type, purc_variant_t name_mapping,
        purc_variant_t type_conversion)
{
    purc_variant_t row = PURC_VARIANT_INVALID;;

    if (!check_cursor(cursor)) {
        goto failed;
    }

    sqlite3_stmt *stmt = cursor->st;
    assert(stmt != NULL);
    assert(sqlite3_data_count(stmt) != 0);

    /* Prevent recursive use of cursors. */
    cursor->locked = 1;
    row = cursor_fetch_one_row(cursor, result_type, name_mapping,
            type_conversion);
    cursor->locked = 0;

    if (!row || purc_variant_is_null(row)) {
        goto failed;
    }

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        if (cursor->is_dml) {
            cursor->rowcount = (long)sqlite3_changes(cursor->conn->db);
        }
        sqlite3_reset(cursor->st);
        sqlite3_finalize(cursor->st);
        cursor->st = NULL;
    }
    else if (rc != SQLITE_ROW) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "sqlite error message is %s", sqlite3_errmsg(cursor->conn->db));
        sqlite3_reset(cursor->st);
        sqlite3_finalize(cursor->st);
        cursor->st = NULL;
        purc_variant_unref(row);
        row = PURC_VARIANT_INVALID;
    }

failed:
    return row;
}

static inline struct dvobj_sqlite_cursor *
get_cursor_from_root(purc_variant_t root)
{
    purc_variant_t v;

    v = purc_variant_object_get_by_ckey(root, SQLITE_KEY_HANDLE);
    assert(v && purc_variant_is_native(v));

    return (struct dvobj_sqlite_cursor *)purc_variant_native_get_entity(v);
}

static struct dvobj_sqlite_cursor *
create_cursor(struct dvobj_sqlite_connection *sqlite_conn)
{
    struct dvobj_sqlite_cursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    cursor->conn = sqlite_conn;
    cursor->rowcount = -1;
    cursor->lastrowid = -1;

    return cursor;
failed:
    return NULL;
}

static void destroy_cursor(struct dvobj_sqlite_cursor *cursor)
{
    if (cursor->listener) {
        purc_variant_revoke_listener(cursor->root, cursor->listener);
    }
    free(cursor);
}

static bool on_sqlite_cursor_being_released(purc_variant_t src, pcvar_op_t op,
        void *ctxt, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(src);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (op == PCVAR_OPERATION_RELEASING) {
        struct dvobj_sqlite_cursor *cursor = ctxt;
        destroy_cursor(cursor);
    }

    return true;
}

static purc_variant_t cursor_execute_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    bool ret = false;
    struct dvobj_sqlite_cursor *cursor = get_cursor_from_root(root);
    if (!check_cursor(cursor)) {
        goto failed;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    size_t nr_sql;
    const char *sql = purc_variant_get_string_const(argv[0]);
    sql = pcutils_trim_spaces(sql, &nr_sql);
    if (nr_sql == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    int rc = cursor_exec_query(cursor, false, sql,
            nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID);
    if (rc == 0) {
        ret = true;
    }

failed:
    return purc_variant_make_boolean(ret);
}

static purc_variant_t cursor_executemany_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    bool ret = false;
    struct dvobj_sqlite_cursor *cursor = get_cursor_from_root(root);
    if (!check_cursor(cursor)) {
        goto failed;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (!purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    size_t nr_sql;
    const char *sql = purc_variant_get_string_const(argv[0]);
    sql = pcutils_trim_spaces(sql, &nr_sql);
    if (nr_sql == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    int rc = cursor_exec_query(cursor, true, sql,
            nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID);
    if (rc == 0) {
        ret = true;
    }

failed:
    return purc_variant_make_boolean(ret);
}

static int parse_fetch_params(size_t nr_args, purc_variant_t *argv,
        purc_variant_type *result_type, purc_variant_t *name_mapping,
        purc_variant_t *type_conversion)
{
    purc_variant_t val;
    if (nr_args > 0) {
        val = argv[0];
        if (!purc_variant_is_string(val)) {
            purc_set_error_with_info(PURC_ERROR_WRONG_DATA_TYPE,
                    "invalid result type '%s'",
                    purc_variant_typename(purc_variant_get_type(val)));
            goto failed;
        }
        const char *type = purc_variant_get_string_const(val);
        if (strcasecmp(type, "tuple") == 0) {
            *result_type = PURC_VARIANT_TYPE_TUPLE;
        }
        else if (strcasecmp(type, "object") == 0) {
            *result_type = PURC_VARIANT_TYPE_OBJECT;
        }
        else {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "invalid result type '%s'", type);
            goto failed;
        }
    }

    if (nr_args > 1) {
        purc_variant_t val = argv[1];
        if (purc_variant_is_null(val)) {
            *name_mapping = PURC_VARIANT_INVALID;
        }
        else if (purc_variant_is_object(val)) {
            *name_mapping = val;
        }
        else {
            purc_set_error_with_info(PURC_ERROR_WRONG_DATA_TYPE,
                    "invalid name mapping type '%s'",
                    purc_variant_typename(purc_variant_get_type(val)));
            goto failed;
        }
    }

    if (nr_args > 2) {
        purc_variant_t val = argv[2];
        if (purc_variant_is_null(val)) {
            *type_conversion = PURC_VARIANT_INVALID;
        }
        else if (purc_variant_is_object(val)) {
            *type_conversion = val;
        }
        else {
            purc_set_error_with_info(PURC_ERROR_WRONG_DATA_TYPE,
                    "invalid type conversion type '%s'", 
                    purc_variant_typename(purc_variant_get_type(val)));
            goto failed;
        }
    }
    return 0;

failed:
    return -1;
}

static purc_variant_t cursor_fetchone_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;

    purc_variant_type result_type = PURC_VARIANT_TYPE_TUPLE;
    purc_variant_t name_mapping = PURC_VARIANT_INVALID;
    purc_variant_t type_conversion = PURC_VARIANT_INVALID;
    purc_variant_t val;

    struct dvobj_sqlite_cursor *cursor = get_cursor_from_root(root);
    if (!check_cursor(cursor)) {
        goto failed;
    }

    int rc = parse_fetch_params(nr_args, argv, &result_type, &name_mapping,
            &type_conversion);
    if (rc != 0) {
        goto failed;
    }

    val = cursor_iterator_next(cursor, result_type, name_mapping,
            type_conversion);

    return val;

failed:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t cursor_fetchmany_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;

    purc_variant_type result_type = PURC_VARIANT_TYPE_TUPLE;
    purc_variant_t name_mapping = PURC_VARIANT_INVALID;
    purc_variant_t type_conversion = PURC_VARIANT_INVALID;
    purc_variant_t val;
    uint64_t size = 0;

    struct dvobj_sqlite_cursor *cursor = get_cursor_from_root(root);
    if (!check_cursor(cursor)) {
        goto failed;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    val = argv[0];
    if (purc_variant_is_longint(val) || purc_variant_is_ulongint(val)) {
        if (!purc_variant_cast_to_ulongint(val, &size, false)) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "invalid param 'size'");
            goto failed;
        }
    }
    else {
        purc_set_error_with_info(PURC_ERROR_WRONG_DATA_TYPE,
                "invalid param type '%s'",
                purc_variant_typename(purc_variant_get_type(val)));
        goto failed;
    }

    if (size == 0) {
        val = purc_variant_make_null();
        goto out;
    }

    int rc = parse_fetch_params(nr_args - 1, argv + 1, &result_type,
            &name_mapping, &type_conversion);
    if (rc != 0) {
        goto failed;
    }

    val = cursor_iterator_next(cursor, result_type, name_mapping,
            type_conversion);

    if (!val || purc_variant_is_null(val)) {
        goto out;
    }

    purc_variant_t arr_val = purc_variant_make_array(1, val);
    if (!arr_val) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        purc_variant_unref(val);
        goto failed;
    }

    size--;
    while (size != 0) {
        val = cursor_iterator_next(cursor, result_type, name_mapping,
             type_conversion);
        if (!val || purc_variant_is_null(val)) {
            break;
        }
        purc_variant_array_append(arr_val, val);
        size--;
    }
    val = arr_val;

out:
    return val;

failed:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t cursor_fetchall_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;

    purc_variant_type result_type = PURC_VARIANT_TYPE_TUPLE;
    purc_variant_t name_mapping = PURC_VARIANT_INVALID;
    purc_variant_t type_conversion = PURC_VARIANT_INVALID;
    purc_variant_t val;

    struct dvobj_sqlite_cursor *cursor = get_cursor_from_root(root);
    if (!check_cursor(cursor)) {
        goto failed;
    }

    int rc = parse_fetch_params(nr_args, argv, &result_type, &name_mapping,
            &type_conversion);
    if (rc != 0) {
        goto failed;
    }

    val = cursor_iterator_next(cursor, result_type, name_mapping,
            type_conversion);

    if (!val || purc_variant_is_null(val)) {
        goto out;
    }

    purc_variant_t arr_val = purc_variant_make_array(1, val);
    if (!arr_val) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        purc_variant_unref(val);
        goto failed;
    }

    while (true) {
        val = cursor_iterator_next(cursor, result_type, name_mapping,
             type_conversion);
        if (!val || purc_variant_is_null(val)) {
            break;
        }
        purc_variant_array_append(arr_val, val);
    }
    val = arr_val;

out:
    return val;

failed:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t cursor_close_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    bool ret = false;
    struct dvobj_sqlite_cursor *cursor = get_cursor_from_root(root);
    if (is_cursor_locked(cursor)) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "Recursive use of cursors not allowed.");
        goto out;
    }

    if (is_conn_closed(cursor->conn)) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "can not operate on a closed database");
        goto out;
    }

    if (is_cursor_closed(cursor)) {
        ret = true;
        goto out;
    }

    if (cursor->st) {
        sqlite3_reset(cursor->st);
        sqlite3_finalize(cursor->st);
        cursor->st = NULL;
    }

    cursor->closed = true;
    ret = true;

out:
    return purc_variant_make_boolean(ret);
}

static purc_variant_t cursor_rowcount_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    struct dvobj_sqlite_cursor *cursor = get_cursor_from_root(root);
    return purc_variant_make_longint(cursor->rowcount);
}

static purc_variant_t cursor_lastrowid_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    struct dvobj_sqlite_cursor *cursor = get_cursor_from_root(root);
    purc_variant_t ret;
    if (cursor->lastrowid != -1) {
        ret = purc_variant_make_longint(cursor->lastrowid);
    }
    else {
        ret = purc_variant_make_null();
    }
    return ret;
}

static purc_variant_t cursor_description_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    struct dvobj_sqlite_cursor *cursor = get_cursor_from_root(root);
    if (cursor->description) {
        return purc_variant_ref(cursor->description);
    }
    return purc_variant_make_null();;
}

static purc_variant_t
create_cursor_variant(struct dvobj_sqlite_connection *sqlite_conn)
{
    static struct purc_dvobj_method methods[] = {
        { SQLITE_KEY_EXECUTE,           cursor_execute_getter,          NULL },
        { SQLITE_KEY_EXECUTEMANY,       cursor_executemany_getter,      NULL },
        { SQLITE_KEY_FETCHONE,          cursor_fetchone_getter,         NULL },
        { SQLITE_KEY_FETCHMANY,         cursor_fetchmany_getter,        NULL },
        { SQLITE_KEY_FETCHALL,          cursor_fetchall_getter,         NULL },
        { SQLITE_KEY_CLOSE,             cursor_close_getter,            NULL },
        { SQLITE_KEY_ROWCOUNT,          cursor_rowcount_getter,         NULL },
        { SQLITE_KEY_LASTROWID,         cursor_lastrowid_getter,        NULL },
        { SQLITE_KEY_DESCRIPTION,       cursor_description_getter,      NULL },
    };

    purc_variant_t cursor_val = purc_dvobj_make_from_methods(methods,
            PCA_TABLESIZE(methods));
    if (cursor_val == PURC_VARIANT_INVALID) {
        goto failed;
    }

    /* $SQLiteCursor.connection */
    if (!purc_variant_object_set_by_static_ckey(cursor_val,
                SQLITE_KEY_CONNECTION, sqlite_conn->root)) {
        goto failed;
    }

    struct dvobj_sqlite_cursor *cursor = create_cursor(sqlite_conn);
    if (!cursor) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    cursor->root = cursor_val;

    purc_variant_t val;
    if ((val = purc_variant_make_native((void *)cursor, NULL)) == NULL) {
        goto failed;
    }

    if (!purc_variant_object_set_by_static_ckey(cursor_val, SQLITE_KEY_HANDLE,
                val)) {
        goto failed;
    }
    purc_variant_unref(val);

    cursor->listener = purc_variant_register_post_listener(cursor_val,
            PCVAR_OPERATION_RELEASING, on_sqlite_cursor_being_released,
            cursor);

failed:
    if (cursor) {
        destroy_cursor(cursor);
    }
    return PURC_VARIANT_INVALID;
}

/* $SQLiteCursor end */

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
        sqlite3_close_v2(db);
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
            sqlite3_close_v2(connection->db);
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


static purc_variant_t conn_cursor_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    return PURC_VARIANT_INVALID;
}

static purc_variant_t conn_commit_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    struct dvobj_sqlite_connection *conn = get_connection_from_root(root);
    bool ret = false;
    if (is_conn_closed(conn)) {
        ret = false;
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "can not operate on a closed database");
        goto out;
    }

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

static purc_variant_t conn_rollback_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;
    struct dvobj_sqlite_connection *conn = get_connection_from_root(root);
    bool ret = false;
    if (is_conn_closed(conn)) {
        ret = false;
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "can not operate on a closed database");
        goto out;
    }

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

static purc_variant_t conn_close_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;

    bool ret = false;
    struct dvobj_sqlite_connection *conn = get_connection_from_root(root);
    if (is_conn_closed(conn)) {
        ret = true;
        goto out;
    }

    int rc = sqlite3_close_v2(conn->db);
    if (rc == SQLITE_OK) {
        ret = true;
        conn->db = NULL;
    }

out:
    return purc_variant_make_boolean(ret);
}

static purc_variant_t conn_execute_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;

    struct dvobj_sqlite_connection *conn = get_connection_from_root(root);
    if (is_conn_closed(conn)) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "can not operate on a closed database");
        goto failed;
    }

    purc_variant_t cursor_val = create_cursor_variant(conn);
    if (!cursor_val) {
        goto failed;
    }

    purc_variant_t val = cursor_execute_getter(cursor_val, nr_args, argv,
            call_flags);
    if (val) {
        purc_variant_unref(val);
    }

    return cursor_val;

failed:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t conn_executemany_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    (void) root;
    (void) nr_args;
    (void) argv;
    (void) call_flags;

    struct dvobj_sqlite_connection *conn = get_connection_from_root(root);
    if (is_conn_closed(conn)) {
        purc_set_error_with_info(PURC_ERROR_EXTERNAL_FAILURE,
                "can not operate on a closed database");
        goto failed;
    }

    purc_variant_t cursor_val = create_cursor_variant(conn);
    if (!cursor_val) {
        goto failed;
    }

    purc_variant_t val = cursor_executemany_getter(cursor_val, nr_args, argv,
            call_flags);
    if (val) {
        purc_variant_unref(val);
    }

    return cursor_val;

failed:
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
        { SQLITE_KEY_CURSOR,                conn_cursor_getter,          NULL },
        { SQLITE_KEY_COMMIT,                conn_commit_getter,          NULL },
        { SQLITE_KEY_ROLLBACK,              conn_rollback_getter,        NULL },
        { SQLITE_KEY_CLOSE,                 conn_close_getter,           NULL },
        { SQLITE_KEY_EXECUTE,               conn_execute_getter,         NULL },
        { SQLITE_KEY_EXECUTEMANY,           conn_executemany_getter,     NULL },
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
    db_name = purc_variant_get_string_const_ex(argv[0], &db_name_len);
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

    if ((val = purc_variant_make_native((void *)sqlite_connection, NULL)) == NULL) {
        goto failed;
    }

    if (!purc_variant_object_set_by_static_ckey(connect, SQLITE_KEY_HANDLE,
                val)) {
        goto failed;
    }
    purc_variant_unref(val);

    sqlite_connection->listener = purc_variant_register_post_listener(connect,
            PCVAR_OPERATION_RELEASING, on_sqlite_connection_being_released,
            sqlite_connection);

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

