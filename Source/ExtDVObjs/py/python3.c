/*
 * @file python3.c
 * @author Vincent Wei
 * @date 2023/03/03
 * @brief The implementation of dynamic variant object $PY.
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

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "config.h"
#include "private/map.h"
#include "private/dvobjs.h"
#include "private/instance.h"
#include "private/atom-buckets.h"
#include "private/debug.h"
#include "purc-variant.h"
#include "purc-errors.h"

#define PY_DVOBJ_VERNAME        "0.1.0"
#define PY_DVOBJ_VERCODE        0

#define MAX_SYMBOL_LEN          64

#define STR(x)                  #x
#define STR2(x)                 STR(x)
#define PY_DVOBJ_VERCODE_STR    STR2(PY_DVOBJ_VERCODE)

#define PY_KEY_IMPL         "impl"
#define PY_KEY_INFO         "info"
#define PY_KEY_EXCEPT       "except"
#define PY_KEY_GLOBAL       "global"
#define PY_KEY_LOCAL        "local"
#define PY_KEY_RUN          "run"
#define PY_KEY_IMPORT       "import"
#define PY_KEY_PYTHONIZE    "pythonize"
#define PY_KEY_STRINGIFY    "stringify"
#define PY_KEY_COMPILE      "compile"
#define PY_KEY_EVAL         "eval"
#define PY_KEY_ENTITY       "entity"
#define PY_KEY_HANDLE       "__handle_python__"

#define PY_INFO_VERSION     "version"
#define PY_INFO_PLATFORM    "platform"
#define PY_INFO_COPYRIGHT   "copyright"
#define PY_INFO_COMPILER    "compiler"
#define PY_INFO_BUILD_INFO  "build-info"
#define PY_INFO_PATH        "path"

#define PY_ATTR_ARG_PREFIX      "__attr_hvml::"
#define PY_ATTR_ARG_PRE_LEN     (sizeof(PY_ATTR_ARG_PREFIX) - 1)

#define PY_NATIVE_PREFIX        "pyObject::"
#define PY_NATIVE_PREFIX_LEN    (sizeof(PY_NATIVE_PREFIX) - 1)
#define PY_ATTR_HVML            "__hvml__"

#define _KW_DELIMITERS          " \t\n\v\f\r"

enum {
#define _KW_command                 "command"
    K_KW_command,
#define _KW_statement               "statement"
    K_KW_statement,
#define _KW_source                  "source"
    K_KW_source,
#define _KW_module                  "module"
    K_KW_module,
#define _KW_file                    "file"
    K_KW_file,
#define _KW_skip_first_line         "skip-first-line"
    K_KW_skip_first_line,
#define _KW_dont_write_byte_code    "dont-write-byte-code"
    K_KW_dont_write_byte_code,
};

static struct keyword_to_atom {
    const char *keyword;
    purc_atom_t atom;
} keywords2atoms [] = {
    { _KW_command, 0 },                 // "command"
    { _KW_statement, 0 },               // "statement"
    { _KW_source, 0 },                  // "source"
    { _KW_module, 0 },                  // "module"
    { _KW_file, 0 },                    // "file"
    { _KW_skip_first_line, 0 },         // "skip-first-line"
    { _KW_dont_write_byte_code, 0 },    // "dont-write-byte-code"
};

#if PY_VERSION_HEX < 0x030a0000

static inline PyObject* Py_NewRef(PyObject *obj)
{
    Py_INCREF(obj);
    return obj;
}

static inline PyObject* Py_XNewRef(PyObject *obj)
{
    Py_XINCREF(obj);
    return obj;
}

#endif /* PY_VERSION_HEX < 0x030a0000 */

struct dvobj_pyinfo {
    pcutils_map             *reserved_symbols;  // the reserved symbols.
    PyObject                *locals;            // the local variables.
    purc_variant_t          root;               // the root variant, i.e., $PY itself
    struct pcvar_listener   *listener;          // the listener
};

static inline struct dvobj_pyinfo *get_pyinfo_from_root(purc_variant_t root)
{
    purc_variant_t v;

    v = purc_variant_object_get_by_ckey(root, PY_KEY_HANDLE);
    assert(v && purc_variant_is_native(v));

    return (struct dvobj_pyinfo *)purc_variant_native_get_entity(v);
}

static inline struct dvobj_pyinfo *get_pyinfo(void)
{
    assert(Py_IsInitialized());
    PyObject *m = PyImport_AddModule("__main__");
    assert(m);
    PyObject *cap = PyObject_GetAttrString(m, PY_ATTR_HVML);
    assert(PyCapsule_CheckExact(cap));
    struct dvobj_pyinfo *pyinfo = PyCapsule_GetPointer(cap, PY_ATTR_HVML);
    assert(pyinfo);

    return pyinfo;
}

static int set_python_except(struct dvobj_pyinfo *pyinfo, const char *except)
{
    int ret = 0;
    purc_variant_t val = purc_variant_make_string_static(except, false);
    if (!purc_variant_object_set_by_static_ckey(pyinfo->root,
                PY_KEY_EXCEPT, val))
        ret = -1;
    purc_variant_unref(val);
    return ret;
}

static void handle_python_error(struct dvobj_pyinfo *pyinfo)
{
    PyObject *pyerr = PyErr_Occurred();
    if (pyerr == NULL)
        return;

#ifndef NDEBUG
    PyObject *str = PyObject_Str(pyerr);
    if (str) {
        puts(PyUnicode_AsUTF8(str));
        Py_DECREF(str);
    }
#endif

    int hvml_err = PURC_ERROR_EXTERNAL_FAILURE;
    const char *pyexcept = "UnknownError";
    if (PyErr_GivenExceptionMatches(pyerr, PyExc_AssertionError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "AssertionError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_AttributeError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "AttributeError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BlockingIOError)) {
        // hvml_err = PURC_ERROR_IO_FAILURE;
        pyexcept = "BlockingIOError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BrokenPipeError)) {
        // hvml_err = PURC_ERROR_BROKEN_PIPE;
        pyexcept = "BrokenPipeError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BufferError)) {
        // hvml_err = PURC_ERROR_IO_FAILURE;
        pyexcept = "BufferError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ChildProcessError)) {
        // hvml_err = PURC_ERROR_CHILD_TERMINATED;
        pyexcept = "ChildProcessError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionAbortedError)) {
        // hvml_err = PURC_ERROR_CONNECTION_ABORTED;
        pyexcept = "ConnectionAbortedError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionRefusedError)) {
        // hvml_err = PURC_ERROR_CONNECTION_REFUSED;
        pyexcept = "ConnectionRefusedError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionResetError)) {
        // hvml_err = PURC_ERROR_CONNECTION_RESET;
        pyexcept = "ConnectionResetError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "ConnectionError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_EOFError)) {
        // hvml_err = PURC_ERROR_IO_FAILURE;
        pyexcept = "EOFError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_FileExistsError)) {
        // hvml_err = PURC_ERROR_EXISTS;
        pyexcept = "FileExistsError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_FileNotFoundError)) {
        // hvml_err = PURC_ERROR_NOT_EXISTS;
        pyexcept = "FileNotFoundError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_FloatingPointError)) {
        // hvml_err = PURC_ERROR_INVALID_FLOAT;
        pyexcept = "FloatingPointError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_GeneratorExit)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "GeneratorExit";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_TabError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "TabError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_IndentationError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "IndentationError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_IndexError)) {
        // hvml_err = PCVRNT_ERROR_OUT_OF_BOUNDS;
        pyexcept = "IndexError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_InterruptedError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "InterruptedError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_IsADirectoryError)) {
        // hvml_err = PURC_ERROR_NOT_DESIRED_ENTITY;
        pyexcept = "IsADirectoryError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_KeyError)) {
        // hvml_err = PCVRNT_ERROR_NO_SUCH_KEY;
        pyexcept = "KeyError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_KeyboardInterrupt)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "KeyboardInterrupt";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_MemoryError)) {
        // hvml_err = PURC_ERROR_OUT_OF_MEMORY;
        pyexcept = "MemoryError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ModuleNotFoundError)) {
        // hvml_err = PURC_ERROR_ENTITY_NOT_FOUND;
        pyexcept = "ModuleNotFoundError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ImportError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "ImportError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnboundLocalError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "UnboundLocalError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_NameError)) {
        // hvml_err = PURC_ERROR_BAD_NAME;
        pyexcept = "NameError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_NotADirectoryError)) {
        // hvml_err = PURC_ERROR_NOT_DESIRED_ENTITY;
        pyexcept = "NotADirectoryError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_NotImplementedError)) {
        // hvml_err = PURC_ERROR_NOT_IMPLEMENTED;
        pyexcept = "NotImplementedError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_OSError)) {
        // hvml_err = PURC_ERROR_SYS_FAULT;
        pyexcept = "OSError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_OverflowError)) {
        // hvml_err = PURC_ERROR_OVERFLOW;
        pyexcept = "OverflowError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_PermissionError)) {
        // hvml_err = PURC_ERROR_ACCESS_DENIED;
        pyexcept = "PermissionError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ProcessLookupError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "ProcessLookupError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_RecursionError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "RecursionError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ReferenceError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "ReferenceError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_RuntimeError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "RuntimeError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_StopAsyncIteration)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "StopAsyncIteration";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_StopIteration)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "StopIteration";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_SyntaxError)) {
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "SyntaxError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_SystemError)) {
        // hvml_err = PURC_ERROR_SYS_FAULT;
        pyexcept = "SystemError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_SystemExit)) {
        // hvml_err = PURC_ERROR_SYS_FAULT;
        pyexcept = "SystemExit";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_TimeoutError)) {
        // hvml_err = PURC_ERROR_TIMEOUT;
        pyexcept = "TimeoutError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_TypeError)) {
        // hvml_err = PURC_ERROR_WRONG_DATA_TYPE;
        pyexcept = "TypeError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeDecodeError)) {
        // hvml_err = PURC_ERROR_BAD_ENCODING;
        pyexcept = "UnicodeDecodeError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeEncodeError)) {
        // hvml_err = PURC_ERROR_BAD_ENCODING;
        pyexcept = "UnicodeEncodeError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeTranslateError)) {
        // hvml_err = PURC_ERROR_BAD_ENCODING;
        pyexcept = "UnicodeTranslateError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeError)) {
        // hvml_err = PURC_ERROR_BAD_ENCODING;
        pyexcept = "UnicodeError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ValueError)) {
        // hvml_err = PURC_ERROR_INVALID_VALUE;
        pyexcept = "ValueError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ZeroDivisionError)) {
        // hvml_err = PURC_ERROR_DIVBYZERO;
        pyexcept = "ZeroDivisionError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ArithmeticError)) {
        /* The base class for those built-in exceptions that are raised for
           various arithmetic errors: OverflowError, ZeroDivisionError,
           and FloatingPointError. */
        // hvml_err = PURC_ERROR_INVALID_FLOAT;
        pyexcept = "ArithmeticError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_LookupError)) {
        /* The base class for the exceptions that are raised when
           a key or index used on a mapping or sequence is invalid:
           IndexError, KeyError. */
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "LookupError";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_Exception)) {
        /* All built-in, non-system-exiting exceptions are derived from
           this class. All user-defined exceptions should also be derived
           from this class. */
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "Exception";
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BaseException)) {
        /* The base class for all built-in exceptions. */
        // hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        pyexcept = "BaseException";
    }

    set_python_except(pyinfo, pyexcept);
    purc_set_error(hvml_err);
    PyErr_Clear();
}

static PyObject *make_pyobj_from_variant(struct dvobj_pyinfo *pyinfo,
        purc_variant_t v)
{
    PyObject *pyobj = NULL;

    enum purc_variant_type t = purc_variant_get_type(v);
    switch (t) {
    case PURC_VARIANT_TYPE_UNDEFINED:
    case PURC_VARIANT_TYPE_NULL:
        pyobj = Py_NewRef(Py_None);
        break;

    case PURC_VARIANT_TYPE_BOOLEAN:
        if (purc_variant_is_true(v))
            pyobj = Py_NewRef(Py_True);
        else
            pyobj = Py_NewRef(Py_False);
        break;

    case PURC_VARIANT_TYPE_EXCEPTION:
    case PURC_VARIANT_TYPE_ATOMSTRING:
    case PURC_VARIANT_TYPE_STRING: {
        const char *string;
        size_t length;
        string = purc_variant_get_string_const_ex(v, &length);
        assert(string);
        pyobj = PyUnicode_DecodeUTF8(string, length, NULL);
        break;
    }

    case PURC_VARIANT_TYPE_NUMBER: {
        double d;
        purc_variant_cast_to_number(v, &d, false);
        pyobj = PyFloat_FromDouble(d);
        break;
    }

    case PURC_VARIANT_TYPE_LONGINT: {
        int64_t l;
        purc_variant_cast_to_longint(v, &l, false);
        if (sizeof(long long) >= sizeof(int64_t))
            pyobj = PyLong_FromLongLong((long long)l);
        else
            pyobj = PyLong_FromDouble((double)l);
        break;
    }

    case PURC_VARIANT_TYPE_ULONGINT: {
        uint64_t ul;
        purc_variant_cast_to_ulongint(v, &ul, false);
        if (sizeof(unsigned long long) >= sizeof(int64_t))
            pyobj = PyLong_FromUnsignedLongLong((unsigned long long)ul);
        else
            pyobj = PyLong_FromDouble((double)ul);
        break;
    }

    case PURC_VARIANT_TYPE_LONGDOUBLE: {
        long double ld;
        purc_variant_cast_to_longdouble(v, &ld, false);
        pyobj = PyFloat_FromDouble((double)ld);
        break;
    }

    case PURC_VARIANT_TYPE_BIGINT: {
        /* XXX: optimize */
        void *str = purc_variant_serialize_alloc(v, 0,
                PCVRNT_SERIALIZE_OPT_BIGINT_HEX, NULL, NULL);
        if (str == NULL)
            break;

        pyobj = PyLong_FromString(str, NULL, 16);
        free(str);
        break;
    }

    case PURC_VARIANT_TYPE_BSEQUENCE: {
        const unsigned char* bytes;
        size_t length;
        bytes = purc_variant_get_bytes_const(v, &length);
        pyobj = PyByteArray_FromStringAndSize((const char *)bytes,
                length);
        break;
    }

    case PURC_VARIANT_TYPE_DYNAMIC:
        pyobj = Py_NewRef(Py_None);
        break;

    case PURC_VARIANT_TYPE_NATIVE: {
        const char *name = purc_variant_native_get_name(v);
        if (!strncmp(name, PY_NATIVE_PREFIX, PY_NATIVE_PREFIX_LEN)) {
            pyobj = purc_variant_native_get_entity(v);
            if (pyobj) {
                pyobj = Py_NewRef(pyobj);
                break;
            }
        }
        pyobj = Py_NewRef(Py_None);
        break;
    }

    case PURC_VARIANT_TYPE_OBJECT: {
        pyobj = PyDict_New();
        if (pyobj == NULL)
            goto failed;

        struct pcvrnt_object_iterator* it;
        it = pcvrnt_object_iterator_create_begin(v);
        while (it) {
            const char     *key = pcvrnt_object_iterator_get_ckey(it);
            purc_variant_t  val = pcvrnt_object_iterator_get_value(it);

            PyObject *pyval = make_pyobj_from_variant(pyinfo, val);
            if (pyval == NULL) {
                pcvrnt_object_iterator_release(it);
                goto failed_subcall;
            }

            if (PyDict_SetItemString(pyobj, key, pyval)) {
                pcvrnt_object_iterator_release(it);
                goto failed;
            }

            if (!pcvrnt_object_iterator_next(it))
                break;
        }
        pcvrnt_object_iterator_release(it);
        break;
    }

    case PURC_VARIANT_TYPE_ARRAY: {
        size_t sz;
        purc_variant_array_size(v, &sz);
        pyobj = PyList_New(sz);
        if (pyobj == NULL)
            goto failed;

        for (size_t i = 0; i < sz; i++) {
            purc_variant_t mbr = purc_variant_array_get(v, i);

            PyObject *pymbr = make_pyobj_from_variant(pyinfo, mbr);
            if (pymbr == NULL) {
                goto failed_subcall;
            }

            if (PyList_SetItem(pyobj, i, pymbr)) {
                goto failed;
            }
        }

        break;
    }

    case PURC_VARIANT_TYPE_TUPLE: {
        size_t sz;
        purc_variant_tuple_size(v, &sz);
        pyobj = PyTuple_New(sz);
        if (pyobj == NULL)
            goto failed;

        for (size_t i = 0; i < sz; i++) {
            purc_variant_t mbr = purc_variant_tuple_get(v, i);

            PyObject *pymbr = make_pyobj_from_variant(pyinfo, mbr);
            if (pymbr == NULL) {
                goto failed_subcall;
            }

            if (PyTuple_SetItem(pyobj, i, pymbr)) {
                goto failed;
            }
        }

        break;
    }

    case PURC_VARIANT_TYPE_SET: {
        const char *unique_keys;
        purc_variant_set_unique_keys(v, &unique_keys);
        size_t sz;
        purc_variant_set_size(v, &sz);

        if (unique_keys == NULL) {
            /* make a PySet from this set variant */
            pyobj = PySet_New(NULL);
            if (pyobj == NULL)
                goto failed;

            for (size_t i = 0; i < sz; i++) {
                purc_variant_t mbr;
                mbr = purc_variant_set_get_by_index(v, i);

                PyObject *pymbr = make_pyobj_from_variant(pyinfo, mbr);
                if (pymbr == NULL) {
                    goto failed_subcall;
                }

                if (PySet_Add(pyobj, pymbr)) {
                    goto failed;
                }
            }
        }
        else {
            /* make a PyList from this set variant */
            pyobj = PyList_New(sz);
            if (pyobj == NULL)
                goto failed;

            for (size_t i = 0; i < sz; i++) {
                purc_variant_t mbr = purc_variant_array_get(v, i);

                PyObject *pymbr = make_pyobj_from_variant(pyinfo, mbr);
                if (pymbr == NULL) {
                    goto failed_subcall;
                }

                if (PyList_SetItem(pyobj, i, pymbr)) {
                    goto failed;
                }
            }
        }

        break;
    }
    }

failed:
    if (pyobj == NULL)
        handle_python_error(pyinfo);

failed_subcall:
    return pyobj;
}

static purc_nvariant_method
pyobject_property_getter_getter(void* native_entity,
        const char* property_name);
static purc_nvariant_method
pyobject_property_setter_getter(void* native_entity,
        const char* property_name);
static void on_release_pyobject(void* native_entity);

static struct purc_native_ops native_pyobject_ops = {
    .property_getter = pyobject_property_getter_getter,
    .property_setter = pyobject_property_setter_getter,
    .on_release = on_release_pyobject,
};

static purc_variant_t
make_variant_from_basic_pyobj(PyObject *pyobj, bool *is_basic)
{
    purc_variant_t v = PURC_VARIANT_INVALID;

    *is_basic = true;
    if (pyobj == Py_None) {
        v = purc_variant_make_null();
    }
    else if (PyBool_Check(pyobj)) {
        if (pyobj == Py_True)
            v = purc_variant_make_boolean(true);
        else
            v = purc_variant_make_boolean(false);
    }
    else if (PyLong_Check(pyobj)) {
#if 1
        int overflow;
        long long ll = PyLong_AsLongLongAndOverflow(pyobj, &overflow);
        if (overflow) {
            /* XXX: optimize */
            // Format big integer to hexadecimal string without prefix.
            PyObject *format_str = PyUnicode_FromString("x");
            if (format_str == NULL) {
                goto done;
            }

            PyObject *hex_str = PyObject_Format(pyobj, format_str);
            Py_DECREF(format_str);
            if (hex_str == NULL) {
                goto done;
            }

            const char *c_str = PyUnicode_AsUTF8(hex_str);
            if (c_str == NULL) {
                Py_DECREF(hex_str);
                goto done;
            }

            v = purc_variant_make_bigint_from_string(c_str, NULL, 16);
            Py_DECREF(hex_str);
        }
        else {
            v = purc_variant_make_longint(ll);
        }
#else
        int overflow;
        long l = PyLong_AsLongAndOverflow(pyobj, &overflow);
        if (overflow) {
            long long ll = PyLong_AsLongLongAndOverflow(pyobj, &overflow);
            if (overflow > 0) {
                if (sizeof(unsigned long long) <= sizeof(uint64_t)) {
                    unsigned long long ull = PyLong_AsUnsignedLongLong(pyobj);
                    v = purc_variant_make_ulongint((uint64_t)ull);
                }
                else {
                    double d = PyLong_AsDouble(pyobj);
                    v = purc_variant_make_number(d);
                }
            }
            else if (overflow) {
                if (sizeof(long long) <= sizeof(int64_t)) {
                    v = purc_variant_make_longint((int64_t)ll);
                }
                else {
                    double d = PyLong_AsDouble(pyobj);
                    v = purc_variant_make_number(d);
                }
            }
            else {
                if (sizeof(long long) <= sizeof(int64_t)) {
                    v = purc_variant_make_longint((int64_t)ll);
                }
                else {
                    double d = PyLong_AsDouble(pyobj);
                    v = purc_variant_make_number(d);
                }
            }
        }
        else {
            v = purc_variant_make_longint((int64_t)l);
        }
#endif
    }
    else if (PyFloat_Check(pyobj)) {
        double d = PyFloat_AsDouble(pyobj);
        v = purc_variant_make_number(d);
    }
    else {
        *is_basic = false;
    }

done:
    return v;
}

static purc_variant_t make_variant_from_pyobj(PyObject *pyobj)
{
    purc_variant_t v = PURC_VARIANT_INVALID;

    bool is_basic;
    v = make_variant_from_basic_pyobj(pyobj, &is_basic);
    if (!is_basic) {
        Py_INCREF(pyobj);
        v = purc_variant_make_native_entity(pyobj, &native_pyobject_ops,
                PY_NATIVE_PREFIX "any");
        if (v == PURC_VARIANT_INVALID) {
            Py_DECREF(pyobj);
        }
    }

    return v;
}

static purc_variant_t convert_pyobj_to_variant(struct dvobj_pyinfo *pyinfo,
        PyObject *pyobj)
{
    purc_variant_t v;
    bool is_basic;
    v = make_variant_from_basic_pyobj(pyobj, &is_basic);

    if (is_basic) {
        // do nothing.
    }
    else if (PyBytes_Check(pyobj)) {
        char *buffer;
        Py_ssize_t length;
        PyBytes_AsStringAndSize(pyobj, &buffer, &length);
        v = purc_variant_make_byte_sequence(buffer, length);
    }
    else if (PyByteArray_Check(pyobj)) {
        char *buffer = PyByteArray_AS_STRING(pyobj);
        Py_ssize_t length = PyByteArray_GET_SIZE(pyobj);
        v = purc_variant_make_byte_sequence(buffer, length);
    }
    else if (PyUnicode_Check(pyobj)) {
        const char *c_str;
        c_str = PyUnicode_AsUTF8(pyobj);
        if (c_str == NULL) {
            handle_python_error(pyinfo);
            goto failed;
        }

        v = purc_variant_make_string(c_str, false);
    }
    else if (PyDict_Check(pyobj)) {
        v = purc_variant_make_object_0();
        if (v == PURC_VARIANT_INVALID)
            goto failed;

        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(pyobj, &pos, &key, &value)) {
            const char *c_key = PyUnicode_AsUTF8(key);
            if (c_key == NULL) {
                handle_python_error(pyinfo);
                goto failed;
            }

            /* ignore internal key/value pair */
            size_t key_len = strlen(c_key);
            if (key_len >= 4 && c_key[0] == '_' && c_key[1] == '_' &&
                    c_key[key_len - 1] == '_' && c_key[key_len - 2] == '_')
                continue;

            purc_variant_t hvml_k, hvml_v;
            hvml_k = purc_variant_make_string(c_key, false);
            hvml_v = convert_pyobj_to_variant(pyinfo, value);
            if (hvml_k && hvml_v) {
                if (!purc_variant_object_set(v, hvml_k, hvml_v))
                    goto failed;
                purc_variant_unref(hvml_k);
                purc_variant_unref(hvml_v);
            }
            else {
                if (hvml_k)
                    purc_variant_unref(hvml_k);
                if (hvml_v)
                    purc_variant_unref(hvml_v);
                goto failed;
            }
        }
    }
    else if (PyList_Check(pyobj)) {
        v = purc_variant_make_array_0();
        if (v == PURC_VARIANT_INVALID)
            goto failed;

        Py_ssize_t sz = PyList_Size(pyobj);
        for (Py_ssize_t i = 0; i < sz; i++) {
            PyObject *member = PyList_GetItem(pyobj, i);
            purc_variant_t hvml_m = convert_pyobj_to_variant(pyinfo, member);
            if (hvml_m) {
                if (!purc_variant_array_append(v, hvml_m))
                    goto failed;
                purc_variant_unref(hvml_m);
            }
            else
                goto failed;
        }
    }
    else if (PyTuple_Check(pyobj)) {
        Py_ssize_t sz = PyTuple_Size(pyobj);
        v = purc_variant_make_tuple(sz, NULL);
        if (v == PURC_VARIANT_INVALID)
            goto failed;

        for (Py_ssize_t i = 0; i < sz; i++) {
            PyObject *member = PyTuple_GetItem(pyobj, i);
            purc_variant_t hvml_m = convert_pyobj_to_variant(pyinfo, member);
            if (hvml_m) {
                if (!purc_variant_tuple_set(v, i, hvml_m))
                    goto failed;
                purc_variant_unref(hvml_m);
            }
            else
                goto failed;
        }
    }
    else if (PyAnySet_Check(pyobj)) {
        PyObject *iter = PyObject_GetIter(pyobj);
        PyObject *item;

        if (iter == NULL) {
            handle_python_error(pyinfo);
            goto failed;
        }

        v = purc_variant_make_set_0(PURC_VARIANT_INVALID);
        if (v == PURC_VARIANT_INVALID)
            goto failed;

        while ((item = PyIter_Next(iter))) {
            purc_variant_t hvml_item = convert_pyobj_to_variant(pyinfo, item);
            Py_DECREF(item);

            if (hvml_item == PURC_VARIANT_INVALID) {
                goto failed;
            }

            if (purc_variant_set_add(v, hvml_item,
                        PCVRNT_CR_METHOD_IGNORE) < 0) {
                purc_variant_unref(hvml_item);
                goto failed;
            }

            purc_variant_unref(hvml_item);
        }

        Py_DECREF(iter);
    }
    else {
        PyObject *result = PyObject_Str(pyobj);
        if (result == NULL) {
            goto failed_python;
        }

        const char *c_str;
        c_str = PyUnicode_AsUTF8(result);
        Py_DECREF(result);
        if (c_str == NULL) {
            goto failed_python;
        }

        v = purc_variant_make_string(c_str, false);
    }

#if 0
    else if (PyComplex_Check(pyobj)) {
        // TODO: support for complex

        double real = PyComplex_RealAsDouble(pyobj);
        double imag = PyComplex_ImagAsDouble(pyobj);
        char buff[128];
        snprintf(buff, sizeof(buff), "%.6f+%.6fi", real, imag);
        v = purc_variant_make_string(buff, false);
    }

    purc_variant_t ret = make_variant_from_pyobj(pyobj);
    if (ret == PURC_VARIANT_INVALID)
        goto failed;
    return ret;
#endif

    return v;

failed_python:
    handle_python_error(pyinfo);
failed:
    return PURC_VARIANT_INVALID;
}

/*
 * This getter returns the result of a callable PyObject by using the
 * variants as a tuple argument.
 *
 * The self getter can be used to support the following usage:
    {{
        $PY.import('datetime', ['datetime:dt', 'timedelta:td']);
        $PY.dt(2020,8,31,12,10,10)
    }}
 */
static purc_variant_t pycallable_self_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    assert(native_entity);
    UNUSED_PARAM(property_name);

    PyObject *pyobj = native_entity;
    assert(PyCallable_Check(pyobj));

    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *result;
    if (nr_args == 0) {
        result = PyObject_CallNoArgs(pyobj);
    }
    else if (nr_args == 1) {
        PyObject *arg = make_pyobj_from_variant(pyinfo, argv[0]);
        if (arg == NULL)
            goto failed;

        result = PyObject_CallOneArg(pyobj, arg);
        Py_DECREF(arg);
    }
    else {
        PyObject *arg = PyTuple_New(nr_args);
        if (arg == NULL)
            goto failed_python;

        for (size_t i = 0; i < nr_args; i++) {
            PyObject *pymbr = make_pyobj_from_variant(pyinfo, argv[i]);
            if (pymbr == NULL) {
                Py_DECREF(arg);
                goto failed;
            }

            if (PyTuple_SetItem(arg, i, pymbr)) {
                Py_DECREF(arg);
                goto failed_python;
            }
        }

        result = PyObject_Call(pyobj, arg, NULL);
    }

    if (result == NULL)
        goto failed_python;

    purc_variant_t ret = make_variant_from_pyobj(result);
    Py_DECREF(result);
    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

/*
 * This self setter returns the result of a callable PyObject by using the
 * first variant (must be an HVML object) as the keyword arguments.
 *
 * This self setter can be used to support the following usage:
    {{
        $PY.import('datetime', ['datetime:dt', 'timedelta:td']);
        $PY.dt(! { year: 2020, month: 8, day: 31,
                    hour: 12, minute: 10, seconod: 10 } )
    }}
 */
static purc_variant_t pycallable_self_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *pyobj = native_entity;
    PyObject *args = NULL, *kwargs = NULL;
    PyObject *result = NULL;

    if (!PyCallable_Check(pyobj)) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto failed;
    }

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (!purc_variant_is_object(argv[nr_args - 1])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    args = PyTuple_New(0);
    if (args == NULL)
        goto failed_python;

    for (size_t i = 0; i < nr_args - 1; i++) {
        PyObject *item = make_pyobj_from_variant(pyinfo, argv[i]);
        if (item == NULL)
            goto failed;
        if (PyTuple_SetItem(args, i, item)) {
            goto failed_python;
        }
    }

    kwargs = make_pyobj_from_variant(pyinfo, argv[nr_args - 1]);
    if (kwargs == NULL)
        goto failed;

    assert(PyDict_Check(kwargs));
    result = PyObject_Call(pyobj, args, kwargs);
    if (result == NULL) {
        goto failed_python;
    }

    purc_variant_t ret = make_variant_from_pyobj(result);
    if (ret == PURC_VARIANT_INVALID)
        goto failed;
    Py_DECREF(args);
    Py_DECREF(kwargs);
    Py_DECREF(result);
    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    Py_XDECREF(result);
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t pydict_self_getter(void *native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    PyObject *dict = (PyObject *)native_entity;
    assert(PyDict_Check(dict));

    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    purc_variant_t ret = PURC_VARIANT_INVALID;
    if (nr_args == 0) {
        ret = convert_pyobj_to_variant(pyinfo, dict);
    }
    else {

        const char *symbol;
        size_t symbol_len;
        symbol = purc_variant_get_string_const_ex(argv[0], &symbol_len);
        if (symbol == NULL || symbol_len == 0) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        if (!purc_is_valid_token(symbol, MAX_SYMBOL_LEN)) {
            purc_set_error(PURC_ERROR_BAD_NAME);
            goto failed;
        }

        PyObject *val = PyDict_GetItemString(dict, symbol);
        if (val == NULL) {
            purc_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
            goto failed;
        }

        ret = make_variant_from_pyobj(val);
    }

    if (ret == PURC_VARIANT_INVALID)
        goto failed;
    return ret;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

/*
 * This getter returns the specified attribute value if the first argument
 * is a string and prefixed by `__attr_hvml:`. Otherwise,
 *
 * If the PyObject is callable, it calls the object with the tuple argument.
 * If there is no argument, it converts the PyObject to the HVML representation:
 *
 *  - Python Bytes and ByteArray: an HVML byte sequence.
 *  - Python string: an HVML string.
 *  - Python list: an HVML array.
 *  - Python dictionary: an HVML object.
 *  - Python set: an HVML generic set.
 *  - Others: an HVML string returned by PyObject_Str().
 *
 * If the PyObject is a dictionary, and the first argument is a string which
 * is not prefixed by `__attr_hvml:`, this getter returns the item as the
 * first argument specifies the key name.
 */
static purc_variant_t pyobject_self_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(property_name);

    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *pyobj = native_entity;

    purc_variant_t ret = PURC_VARIANT_INVALID;
    const char *name = NULL;
    if (nr_args == 1 && (name = purc_variant_get_string_const(argv[0]))) {
        if (strncmp(name, PY_ATTR_ARG_PREFIX, PY_ATTR_ARG_PRE_LEN) == 0) {
            const char *attr_name = name + PY_ATTR_ARG_PRE_LEN;
            if (!purc_is_valid_token(attr_name, MAX_SYMBOL_LEN)) {
                purc_set_error(PURC_ERROR_BAD_NAME);
                goto failed;
            }

            if (!PyObject_HasAttrString(pyobj, attr_name)) {
                goto failed_python;
            }

            PyObject *val = PyObject_GetAttrString(pyobj, attr_name);
            if (val == NULL) {
                goto failed_python;
            }

            ret = make_variant_from_pyobj(val);
            if (ret == PURC_VARIANT_INVALID)
                goto failed;
        }
    }

    if (ret == PURC_VARIANT_INVALID) {
        if (PyCallable_Check(pyobj)) {
            return pycallable_self_getter(pyobj, NULL, nr_args, argv, call_flags);
        }
        else if (PyDict_Check(pyobj)) {
            return pydict_self_getter(pyobj, NULL, nr_args, argv, call_flags);
        }
        else {
            ret = convert_pyobj_to_variant(pyinfo, pyobj);
            if (ret == PURC_VARIANT_INVALID)
                goto failed;
        }
    }

    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t pydict_self_setter(void *native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *dict = (PyObject *)native_entity;
    assert(PyDict_Check(dict));
    UNUSED_PARAM(property_name);

    if (nr_args == 0 || purc_variant_is_undefined(argv[0])) {
        PyObject *m = PyImport_AddModule("__main__");
        if (dict != PyModule_GetDict(m)) {
            PyDict_Clear(dict);
        }
    }
    else if (nr_args >= 1) {
        if (purc_variant_is_object(argv[0])) {
            // merge the data

            struct pcvrnt_object_iterator* it;
            it = pcvrnt_object_iterator_create_begin(argv[0]);
            while (it) {
                const char     *key = pcvrnt_object_iterator_get_ckey(it);
                purc_variant_t  val = pcvrnt_object_iterator_get_value(it);

                PyObject *pyval = make_pyobj_from_variant(pyinfo, val);
                if (pyval == NULL) {
                    pcvrnt_object_iterator_release(it);
                    goto failed;
                }

                if (PyDict_SetItemString(dict, key, pyval)) {
                    pcvrnt_object_iterator_release(it);
                    goto failed;
                }

                if (!pcvrnt_object_iterator_next(it))
                    break;
            }
            pcvrnt_object_iterator_release(it);
        }
        else if (nr_args == 1) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
        else if (nr_args > 1) {
            const char *symbol;
            size_t symbol_len;
            symbol = purc_variant_get_string_const_ex(argv[0], &symbol_len);
            if (symbol == NULL || symbol_len == 0) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            if (!purc_is_valid_token(symbol, MAX_SYMBOL_LEN)) {
                purc_set_error(PURC_ERROR_BAD_NAME);
                goto failed;
            }

            if (purc_variant_is_undefined(argv[1])) {
                if (PyDict_DelItemString(dict, symbol)) {
                    goto failed_python;
                }
            }
            else {
                PyObject *pyobj = make_pyobj_from_variant(pyinfo, argv[1]);
                if (pyobj == NULL) {
                    goto failed;
                }

                if (PyDict_SetItemString(dict, symbol, pyobj)) {
                    goto failed_python;
                }
            }
        }
        else {
            purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
            goto failed;
        }
    }

    return purc_variant_make_boolean(true);

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t pyset_self_setter(void *native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *pyobj = (PyObject *)native_entity;
    assert(PySet_Check(pyobj));
    UNUSED_PARAM(property_name);

    if (nr_args == 0 || purc_variant_is_undefined(argv[0])) {
        if (!PySet_Clear(pyobj))
            goto failed_python;
    }
    else {
        if (purc_variant_is_set(argv[0])) {
            if (!PySet_Clear(pyobj))
                goto failed_python;

            size_t sz;
            purc_variant_set_size(argv[0], &sz);
            for (size_t i = 0; i < sz; i++) {
                purc_variant_t mbr;
                mbr = purc_variant_set_get_by_index(argv[0], i);
                PyObject *pymbr = make_pyobj_from_variant(pyinfo, mbr);
                if (pymbr == NULL) {
                    goto failed;
                }

                if (PySet_Add(pyobj, pymbr)) {
                    goto failed_python;
                }
            }
        }
        else if (nr_args == 1) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }
    }

    return purc_variant_make_boolean(true);

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

/*
 * This setter is used to set an attribute of a PyObject, or operate on
 * a special PyObject if it is callable, a dictionary, or a set.
 *
 * If the first argument is a string prefixed by `__attr_hvml:`, the setter
 * will try to change the attribute of the PyObject.
 *
 * If the first arugment is not a string or it is not prefixed by
 * `__attr_hvml:`, the setter will operate according to the type of PyObject:
 *  - PyCallable: call the object by treating the first arugment as the
 *      keyword argument.
 *  - PyDict: merge properties of an HVML object to the dictionary,
 *      update or remove an item.
 *  - PySet: reset or clear the set.
 */
static purc_variant_t pyobject_self_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    PyObject *pyobj = native_entity;
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    assert(pyobj);

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *name;
    name = purc_variant_get_string_const(argv[0]);
    if (name && strncmp(name, PY_ATTR_ARG_PREFIX, PY_ATTR_ARG_PRE_LEN) == 0) {
        const char *attr_name = name + PY_ATTR_ARG_PRE_LEN;
        if (!purc_is_valid_token(attr_name, MAX_SYMBOL_LEN)) {
            purc_set_error(PURC_ERROR_BAD_NAME);
            goto failed;
        }

        if (!PyObject_HasAttrString(pyobj, attr_name)) {
            goto failed_python;
        }

        if (purc_variant_is_undefined(argv[1])) {
            if (PyObject_DelAttrString(pyobj, attr_name))
                goto failed_python;
        }
        else {
            PyObject *val = make_pyobj_from_variant(pyinfo, argv[1]);
            if (val == NULL)
                goto failed;

            if (PyObject_SetAttrString(pyobj, attr_name, val))
                goto failed_python;
        }
    }
    else if (PyCallable_Check(pyobj)) {
        return pycallable_self_setter(pyobj, NULL, nr_args, argv, call_flags);
    }
    else if (PyDict_Check(pyobj)) {
        return pydict_self_setter(pyobj, NULL, nr_args, argv, call_flags);
    }
    else if (PySet_Check(pyobj)) {
        return pyset_self_setter(pyobj, NULL, nr_args, argv, call_flags);
    }

    return purc_variant_make_boolean(true);

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

/*
 * This getter returns the result when calling a method on a PyObject by using
 * the tuple arguments.
 *
 * The method getter can be used to support the following usage:
    {{
        $PY.local.x(! [1, 2, 2, 3] );
        $PY.local.x.count(2)
    }}
 */
static purc_variant_t pyobject_method_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();

    PyObject *pyobj = native_entity;

#ifndef NDEBUG
    PyObject *method = PyObject_GetAttrString(pyobj, property_name);
    assert(PyMethod_Check(method));
#endif

    PyObject *name = PyUnicode_FromString(property_name);
    if (name == NULL)
        goto failed_python;

    PyObject *result;
    if (nr_args == 0) {
        result = PyObject_CallMethodNoArgs(pyobj, name);
    }
    else if (nr_args == 1) {
        PyObject *arg = make_pyobj_from_variant(pyinfo, argv[0]);
        if (arg == NULL)
            goto failed;

        result = PyObject_CallMethodOneArg(pyobj, name, arg);
        Py_DECREF(arg);
    }
    else {
        PyObject *vc_args[nr_args + 1]; // use vectorcall
        vc_args[0] = pyobj;

        for (size_t i = 0; i < nr_args; i++) {
            PyObject *pymbr = make_pyobj_from_variant(pyinfo, argv[i]);
            if (pymbr == NULL) {
                goto failed;
            }

            vc_args[i + 1] = pymbr;
        }

        result = PyObject_VectorcallMethod(name, vc_args, nr_args + 1, NULL);
        for (size_t i = 0; i < nr_args; i++) {
            Py_DECREF(vc_args[i + 1]);
        }
    }

    if (result == NULL)
        goto failed_python;

    purc_variant_t ret = make_variant_from_pyobj(result);
    Py_DECREF(result);
    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

/*
 * This setter returns the result when calling a method on a PyObject by using
 * the keyword arguments.
 *
 * The method getter can be used to support the following usage:
    {{
        $PY.local.x(! [1, 2, 2, 3] );
        $PY.local.x.foo(! {bar: 'aaa' } )
    }}
 */
static purc_variant_t pyobject_method_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *pyobj = native_entity;

#ifndef NDEBUG
    PyObject *method = PyObject_GetAttrString(pyobj, property_name);
    assert(PyMethod_Check(method));
#endif

    PyObject *args[nr_args];
    memset(args, 0, sizeof(args));
    PyObject *kwargs = NULL;
    PyObject *result = NULL;
    PyObject *name = PyUnicode_FromString(property_name);
    if (name == NULL)
        goto failed_python;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (!purc_variant_is_object(argv[nr_args - 1])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    kwargs = make_pyobj_from_variant(pyinfo, argv[nr_args - 1]);
    if (kwargs == NULL)
        goto failed;

    args[0] = pyobj;
    for (size_t i = 1; i < nr_args; i++) {
        args[i] = make_pyobj_from_variant(pyinfo, argv[i - 1]);
        if (args[i] == NULL) {
            goto failed;
        }
    }

    result = PyObject_VectorcallMethod(name, args, nr_args, kwargs);
    if (result == NULL)
        goto failed_python;

    purc_variant_t ret = make_variant_from_pyobj(result);
    for (size_t j = 1; j < nr_args; j++) {
        Py_DECREF(args[j]);
    }
    Py_DECREF(name);
    Py_DECREF(kwargs);
    Py_DECREF(result);
    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    for (size_t j = 1; j < nr_args; j++) {
        Py_XDECREF(args[j]);
    }
    Py_XDECREF(name);
    Py_XDECREF(result);
    Py_XDECREF(kwargs);
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t pyobject_callable_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    assert(native_entity);
    assert(property_name);
    PyObject *pyobj = native_entity;
    PyObject *callable = PyObject_GetAttrString(pyobj, property_name);
    assert(PyCallable_Check(callable));

    return pycallable_self_getter(callable, NULL, nr_args, argv, call_flags);
}

static purc_variant_t pyobject_callable_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    assert(native_entity);
    assert(property_name);
    PyObject *pyobj = native_entity;
    PyObject *callable = PyObject_GetAttrString(pyobj, property_name);
    assert(PyCallable_Check(callable));

    return pycallable_self_setter(callable, NULL, nr_args, argv, call_flags);
}

static purc_variant_t pyobject_attr_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    assert(native_entity);
    assert(property_name);

    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *pyobj = native_entity;
    PyObject *attr = PyObject_GetAttrString(pyobj, property_name);
    if (attr == NULL) {
        goto failed_python;
    }

    purc_variant_t ret = make_variant_from_pyobj(attr);
    if (ret == PURC_VARIANT_INVALID)
        goto failed;
    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

/*
 * This getter is used to get the value of the specified key of a PyDict:
 *
    {{
        $PY.local.x(! 3 ); $PY.local.x()
    }}
 */
static purc_variant_t pydict_item_getter(void *native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    PyObject *dict = (PyObject *)native_entity;
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    assert(property_name);
    PyObject *val = PyDict_GetItemString(dict, property_name);
    if (val == NULL) {
        purc_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
        goto failed;
    }

    if (PyCallable_Check(val)) {
        return pycallable_self_getter(val, NULL, nr_args, argv, call_flags);
    }

    purc_variant_t ret = make_variant_from_pyobj(val);
    if (ret == PURC_VARIANT_INVALID)
        goto failed;
    return ret;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

/*
 * This setter is used to set the value of the specified key of a PyDict:
 *
    {{
        $PY.local.x(! [1, 2, 2, 3] )
    }}
 */
static purc_variant_t pydict_item_setter(void *native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *dict = (PyObject *)native_entity;
    assert(PyDict_Check(dict));
    assert(property_name);

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    PyObject *val = PyDict_GetItemString(dict, property_name);
    if (val != NULL && purc_variant_is_undefined(argv[0])) {
        if (PyDict_DelItemString(dict, property_name)) {
            goto failed_python;
        }
    }
    else if (val != NULL && PyCallable_Check(val)) {
        return pycallable_self_setter(val, NULL, nr_args, argv, call_flags);
    }
    else {
        PyObject *pyobj = make_pyobj_from_variant(pyinfo, argv[0]);
        if (pyobj == NULL) {
            goto failed;
        }

        if (PyDict_SetItemString(dict, property_name, pyobj)) {
            goto failed_python;
        }
    }

    return purc_variant_make_boolean(true);

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_nvariant_method
pyobject_property_getter_getter(void* native_entity, const char* property_name)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *pyobj = (PyObject *)native_entity;
    assert(pyobj);

    if (property_name == NULL) {
        return pyobject_self_getter;
    }
    else {
        PyObject *val = PyObject_GetAttrString(pyobj, property_name);
        if (val == NULL && !PyDict_Check(pyobj)) {
            handle_python_error(pyinfo);
            return NULL;
        }
        else if (val != NULL && PyMethod_Check(val)) {
            return pyobject_method_getter;
        }
        else if (val != NULL && PyCallable_Check(val)) {
            return pyobject_callable_getter;
        }
        else if (val == NULL && PyDict_Check(pyobj)) {
            if (PyErr_Occurred())
                PyErr_Clear();
            return pydict_item_getter;
        }
        else if (val) {
            return pyobject_attr_getter;
        }
    }

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static purc_nvariant_method
pyobject_property_setter_getter(void* native_entity, const char* property_name)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *pyobj = (PyObject *)native_entity;
    assert(pyobj);

    if (property_name == NULL) {
        return pyobject_self_setter;
    }
    else {
        PyObject *val = PyObject_GetAttrString(pyobj, property_name);
        if (val == NULL && !PyDict_Check(pyobj)) {
            handle_python_error(pyinfo);
            return NULL;
        }
        else if (val != NULL && PyMethod_Check(val)) {
            return pyobject_method_setter;
        }
        else if (val != NULL && PyCallable_Check(val)) {
            return pyobject_callable_setter;
        }
        else if (PyDict_Check(pyobj)) {
            if (PyErr_Occurred())
                PyErr_Clear();
            return pydict_item_setter;
        }
        else {
            handle_python_error(pyinfo);
            purc_set_error(PURC_ERROR_NOT_SUPPORTED);
            return NULL;
        }
    }

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static void on_release_pyobject(void* native_entity)
{
    PyObject *pyobj = native_entity;
    Py_DECREF(pyobj);
}

enum {
    RUN_OPT_SKIP_FIRST_LINE         = 0x0001,
    RUN_OPT_DONT_WRITE_BYTE_CODE    = 0x0002,
    RUN_OPT_SET_ARGV0               = 0x0004,
};

static purc_variant_t run_string(purc_variant_t root,
        const char *cmd, size_t len, int start,
        PyCompilerFlags *cf, unsigned options)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    UNUSED_PARAM(len);

    if (options & RUN_OPT_SKIP_FIRST_LINE) {
        size_t pos = 0;
        while (*cmd) {
            if (*cmd == '\n' && pos > 0) {
                cmd--;
                break;
            }
            cmd++;
            pos++;
        }
    }

    PyObject *m, *globals, *result;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        goto failed;

    globals = PyModule_GetDict(m);
    result = PyRun_StringFlags(cmd, start, globals,
            PyDict_Size(pyinfo->locals) == 0 ? globals : pyinfo->locals,
            cf);
    if (result == NULL) {
        handle_python_error(pyinfo);
        goto failed;
    }

    purc_variant_t ret = make_variant_from_pyobj(result);
    Py_DECREF(result);
    return ret;

failed:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t run_module(purc_variant_t root,
        const char *modname, size_t len, PyCompilerFlags *cf, unsigned options)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    UNUSED_PARAM(cf);
    UNUSED_PARAM(options);

    PyObject *module, *runpy, *runmodule, *runargs, *result;
    runpy = PyImport_ImportModule("runpy");
    if (runpy == NULL) {
        goto failed;
    }

    runmodule = PyObject_GetAttrString(runpy, "_run_module_as_main");
    if (runmodule == NULL) {
        Py_DECREF(runpy);
        goto failed;
    }

    module = PyUnicode_DecodeUTF8(modname, len, NULL);
    if (module == NULL) {
        Py_DECREF(runpy);
        Py_DECREF(runmodule);
        goto failed;
    }

    runargs = PyTuple_Pack(2, module,
            (options & RUN_OPT_SET_ARGV0) ? Py_True : Py_False);
    if (runargs == NULL) {
        Py_DECREF(runpy);
        Py_DECREF(runmodule);
        Py_DECREF(module);
        goto failed;
    }

    result = PyObject_Call(runmodule, runargs, NULL);
    Py_DECREF(runpy);
    Py_DECREF(runmodule);
    Py_DECREF(module);
    Py_DECREF(runargs);
    if (result == NULL) {
        goto failed;
    }

    purc_variant_t ret = make_variant_from_pyobj(result);
    Py_DECREF(result);
    return ret;

failed:
    handle_python_error(pyinfo);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t run_file(purc_variant_t root,
        const char *fname, size_t len, PyCompilerFlags *cf, unsigned options)
{
    UNUSED_PARAM(len);
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);

    FILE *fp = fopen(fname, "rb");
    if (fp == NULL) {
        purc_set_error(PURC_ERROR_IO_FAILURE);
        return PURC_VARIANT_INVALID;
    }

    PyObject *m, *globals, *result;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        goto failed;

    if (options & RUN_OPT_SKIP_FIRST_LINE) {
        int ch;
        /* Push back first newline so line numbers remain the same */
        while ((ch = getc(fp)) != EOF) {
            if (ch == '\n') {
                (void)ungetc(ch, fp);
                break;
            }
        }
    }

    globals = PyModule_GetDict(m);
    result = PyRun_FileFlags(fp, fname, Py_file_input, globals,
            PyDict_Size(pyinfo->locals) == 0 ? globals : pyinfo->locals, cf);
    if (result == NULL) {
        goto failed;
    }

    fclose(fp);
    purc_variant_t ret = make_variant_from_pyobj(result);
    Py_DECREF(result);
    return ret;

failed:
    fclose(fp);
    handle_python_error(pyinfo);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t run_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    enum {
        RUN_TYPE_UNKNOWN = -1,
        RUN_TYPE_COMMAND = 0,
        RUN_TYPE_STATEMENT,
        RUN_TYPE_SOURCE,
        RUN_TYPE_MODULE,
        RUN_TYPE_FILE,
    } run_type = RUN_TYPE_UNKNOWN;
    unsigned run_options = 0;

    PyCompilerFlags cf = _PyCompilerFlags_INIT;
    cf.cf_flags |= PyCF_IGNORE_COOKIE;
    size_t len;
    const char *cmd_mod_file;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (nr_args > 1) {

        const char *options;
        size_t options_len;
        options = purc_variant_get_string_const_ex(argv[1], &options_len);
        if (options == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        options = pcutils_trim_spaces(options, &options_len);
        if (options_len == 0) {
            goto empty_option;
        }

        size_t length = 0;
        const char *option = pcutils_get_next_token_len(options, options_len,
                _KW_DELIMITERS, &length);
        do {
            purc_atom_t atom;

            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = keywords2atoms[K_KW_command].atom;
            }
            else {
                char tmp[length + 1];
                strncpy(tmp, option, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(ATOM_BUCKET_DVOBJ, tmp);
            }

            if (run_type == RUN_TYPE_UNKNOWN) {
                if (atom == keywords2atoms[K_KW_command].atom) {
                    // command
                    run_type = RUN_TYPE_COMMAND;
                }
                else if (atom == keywords2atoms[K_KW_statement].atom) {
                    // statement
                    run_type = RUN_TYPE_STATEMENT;
                }
                else if (atom == keywords2atoms[K_KW_source].atom) {
                    // source
                    run_type = RUN_TYPE_SOURCE;
                }
                else if (atom == keywords2atoms[K_KW_module].atom) {
                    // module
                    run_options |= RUN_OPT_SET_ARGV0;
                    run_type = RUN_TYPE_MODULE;
                }
                else if (atom == keywords2atoms[K_KW_file].atom) {
                    // file
                    run_type = RUN_TYPE_FILE;
                }
            }

            if (atom == keywords2atoms[K_KW_skip_first_line].atom) {
                // skip-first-line
                run_options |= RUN_OPT_SKIP_FIRST_LINE;
            }
            else if (atom == keywords2atoms[K_KW_dont_write_byte_code].atom) {
                // dont-write-byte-code
                run_options |= RUN_OPT_DONT_WRITE_BYTE_CODE;
            }

            if (options_len <= length)
                break;

            options_len -= length;
            option = pcutils_get_next_token_len(option + length, options_len,
                    _KW_DELIMITERS, &length);
        } while (option);
    }

empty_option:
    cmd_mod_file = purc_variant_get_string_const_ex(argv[0], &len);
    if (cmd_mod_file == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    purc_variant_t ret;
    switch (run_type) {
        case RUN_TYPE_UNKNOWN:
        case RUN_TYPE_COMMAND:
        case RUN_TYPE_STATEMENT:
        case RUN_TYPE_SOURCE:
            cf.cf_flags |= PyCF_SOURCE_IS_UTF8;
            ret = run_string(root, cmd_mod_file, len,
                    (run_type == RUN_TYPE_SOURCE) ? Py_file_input :
                        ((run_type == RUN_TYPE_STATEMENT) ? Py_single_input :
                            Py_eval_input),
                    &cf, run_options);
            break;

        case RUN_TYPE_MODULE:
            ret = run_module(root, cmd_mod_file, len, &cf, run_options);
            break;

        case RUN_TYPE_FILE:
            ret = run_file(root, cmd_mod_file, len, &cf, run_options);
            break;
    }

    if (ret == PURC_VARIANT_INVALID)
        goto failed;

    return ret;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static int split_module_names(const char *string, size_t str_len,
            char *package_name, char *module_name, char *module_aliase)
{
    char dup[str_len + 1];
    char *p;
    const char *module_start = NULL;
    const char *module_end = NULL;

    strncpy(dup, string, str_len);
    dup[str_len] = '\0';

    if ((p = strrchr(dup, ':'))) {
        if (!purc_is_valid_token(p + 1, MAX_SYMBOL_LEN))
            goto failed;

        strncpy(module_aliase, p + 1, MAX_SYMBOL_LEN);
        module_aliase[MAX_SYMBOL_LEN] = '\0';
        module_end = p;
    }
    else {
        module_aliase[0] = '\0';
        module_end = dup + str_len;
    }

    char *dot;
    size_t len;
    if ((dot = strchr(dup, '.'))) {
        len = dot - dup;
        if (len == 0 || len > MAX_SYMBOL_LEN)
            goto failed;

        strncpy(package_name, dup, len);
        package_name[len] = '\0';
        if (!purc_is_valid_token(package_name, MAX_SYMBOL_LEN))
            goto failed;
        module_start = dot + 1;
    }
    else {
        package_name[0] = '\0';
        module_start = dup;
    }

    len = module_end - module_start;
    if (len == 0 || len > MAX_SYMBOL_LEN)
        goto failed;

    strncpy(module_name, module_start, len);
    module_name[len] = '\0';
    if (!purc_is_valid_token(module_name, MAX_SYMBOL_LEN))
        goto failed;
    return 0;

failed:
    return -1;
}

static int split_symbol_names(const char *string, size_t str_len,
            char *symbol_name, char *symbol_aliase)
{
    char dup[str_len + 1];
    char *p;
    const char *symbol_end = NULL;

    strncpy(dup, string, str_len);
    dup[str_len] = '\0';

    const char *symbol_start = dup;
    if ((p = strrchr(dup, ':'))) {
        if (!purc_is_valid_token(p + 1, MAX_SYMBOL_LEN))
            goto failed;

        strncpy(symbol_aliase, p + 1, MAX_SYMBOL_LEN);
        symbol_aliase[MAX_SYMBOL_LEN] = '\0';
        symbol_end = p;
    }
    else {
        symbol_aliase[0] = '\0';
        symbol_end = dup + str_len;
    }

    size_t len = symbol_end - symbol_start;
    if (len == 0 || len > MAX_SYMBOL_LEN)
        goto failed;

    strncpy(symbol_name, symbol_start, len);
    symbol_name[len] = '\0';
    if (!purc_is_valid_token(symbol_name, MAX_SYMBOL_LEN))
        goto failed;

    return 0;

failed:
    return -1;
}

static purc_variant_t import_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    PyObject *fromlist = NULL, *aliaselist = NULL;
    purc_variant_t val = PURC_VARIANT_INVALID;

    PyObject *m, *globals, *locals;
    m = PyImport_AddModule("__main__");
    if (m == NULL) {
        goto failed_python;
    }

    globals = PyModule_GetDict(m);
    locals = pyinfo->locals;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *string;
    size_t str_len;
    string = purc_variant_get_string_const_ex(argv[0], &str_len);
    if (string == NULL || str_len == 0) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    string = pcutils_trim_spaces(string, &str_len);
    if (str_len == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    char package_name[MAX_SYMBOL_LEN + 1];
    char module_name[MAX_SYMBOL_LEN + 1];
    char module_aliase[MAX_SYMBOL_LEN + 1];
    if (split_module_names(string, str_len,
            package_name, module_name, module_aliase)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    /* DO NOT check duplication of the variable name
    if (module_aliase[0]) {
        if (PyDict_GetItemString(globals, module_aliase)) {
            purc_set_error(PURC_ERROR_DUPLICATE_NAME);
            goto failed;
        }
    } */

    if (nr_args > 1) {
        if (!purc_variant_is_array(argv[1])) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        fromlist = PyList_New(0);
        aliaselist = PyList_New(0);
        if (fromlist == NULL || aliaselist == NULL) {
            goto failed_python;
        }

        size_t sz;
        purc_variant_array_size(argv[1], &sz);
        for (size_t i = 0; i < sz; i++) {
            purc_variant_t mbr = purc_variant_array_get(argv[1], i);

            string = purc_variant_get_string_const_ex(mbr, &str_len);
            if (string == NULL || str_len == 0) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            string = pcutils_trim_spaces(string, &str_len);
            if (str_len == 0) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            char symbol_name[MAX_SYMBOL_LEN + 1];
            char symbol_aliase[MAX_SYMBOL_LEN + 1];
            if (split_symbol_names(string, str_len,
                        symbol_name, symbol_aliase)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }

            const char *symbol_str;
            if (symbol_aliase[0]) {
                symbol_str = symbol_aliase;
            }
            else {
                symbol_str = symbol_name;
            }

            if (pcutils_map_find(pyinfo->reserved_symbols, symbol_str)) {
                pcinst_set_error(PURC_ERROR_CONFLICT);
                goto failed;
            }

            PyObject *pymbr = PyUnicode_FromString(symbol_name);
            if (pymbr == NULL) {
                goto failed_python;
            }

            if (PyList_Append(fromlist, pymbr)) {
                goto failed_python;
            }

            if (symbol_aliase[0]) {
                pymbr = PyUnicode_FromString(symbol_aliase);
                if (pymbr == NULL) {
                    goto failed_python;
                }
            }
            else {
                pymbr = Py_NewRef(Py_None);
            }

            if (PyList_Append(aliaselist, pymbr)) {
                goto failed_python;
            }
        }
    }

    Py_ssize_t sz_fromlist = fromlist ? PyList_Size(fromlist) : 0;

    if (module_aliase[0] == '\0') {
        if (sz_fromlist > 0)
            strcpy(module_aliase, module_name);
        else if (package_name[0])
            strcpy(module_aliase, package_name);
        else
            strcpy(module_aliase, module_name);
    }

    if (pcutils_map_find(pyinfo->reserved_symbols, module_aliase)) {
        pcinst_set_error(PURC_ERROR_CONFLICT);
        goto failed;
    }

    PyObject *module = PyImport_ImportModuleEx(module_name, globals,
            (PyDict_Size(locals) == 0) ? globals : locals,
            fromlist);
    if (module == NULL) {
        goto failed_python;
    }

    if (PyDict_SetItemString(globals, module_aliase, module)) {
        goto failed_python;
    }

    Py_INCREF(module);
    if ((val = purc_variant_make_native_entity(module,
                    &native_pyobject_ops, PY_NATIVE_PREFIX "module")) == NULL) {
        Py_DECREF(module);
        goto failed;
    }

    if (!purc_variant_object_set_by_ckey(pyinfo->root,
                module_aliase, val))
        goto failed;
    purc_variant_unref(val);

    for (Py_ssize_t i = 0; i < sz_fromlist; i++) {
        PyObject *symbol = PyList_GetItem(fromlist, i);
        PyObject *aliase = PyList_GetItem(aliaselist, i);

        PyObject *obj = PyObject_GetAttr(module, symbol);
        if (obj) {
            const char *symbol_str = NULL;
            if (PyUnicode_Check(aliase)) {
                if (PyDict_SetItem(globals, aliase, obj)) {
                    goto failed_python;
                }
                symbol_str = PyUnicode_AsUTF8(aliase);
            }
            else {
                if (PyDict_SetItem(globals, symbol, obj)) {
                    goto failed_python;
                }
                symbol_str = PyUnicode_AsUTF8(symbol);
            }

            if (PyDict_SetItem(globals, symbol, obj)) {
                goto failed_python;
            }

            Py_INCREF(obj);
            if (PyCallable_Check(obj)) {
                if ((val = purc_variant_make_native_entity(obj,
                                &native_pyobject_ops,
                                PY_NATIVE_PREFIX "callable")) == NULL) {
                    Py_DECREF(obj);
                    goto failed;
                }

                if (!purc_variant_object_set_by_ckey(pyinfo->root,
                            symbol_str, val))
                    goto failed;
                purc_variant_unref(val);
            }
            else if (PyModule_Check(obj)) {
                if ((val = purc_variant_make_native_entity(obj,
                                &native_pyobject_ops,
                                PY_NATIVE_PREFIX "module")) == NULL) {
                    Py_DECREF(obj);
                    goto failed;
                }

                if (!purc_variant_object_set_by_ckey(pyinfo->root,
                            symbol_str, val))
                    goto failed;
                purc_variant_unref(val);
            }
            else {
                if ((val = purc_variant_make_native_entity(obj,
                                &native_pyobject_ops,
                                PY_NATIVE_PREFIX "any")) == NULL) {
                    Py_DECREF(obj);
                    goto failed;
                }

                if (!purc_variant_object_set_by_ckey(pyinfo->root,
                            symbol_str, val))
                    goto failed;
                purc_variant_unref(val);
            }
        }
    }

    Py_XDECREF(aliaselist);
    Py_XDECREF(fromlist);
    // bind aliase to globals
    return purc_variant_make_boolean(true);

failed_python:
    handle_python_error(pyinfo);
failed:
    if (val)
        purc_variant_unref(val);
    Py_XDECREF(aliaselist);
    Py_XDECREF(fromlist);
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t pythonize_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    PyObject *result = NULL;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else {
        switch (purc_variant_get_type(argv[0])) {
        case PURC_VARIANT_TYPE_UNDEFINED:
        case PURC_VARIANT_TYPE_NULL:
        case PURC_VARIANT_TYPE_BOOLEAN:
        case PURC_VARIANT_TYPE_NUMBER:
        case PURC_VARIANT_TYPE_LONGINT:
        case PURC_VARIANT_TYPE_ULONGINT:
        case PURC_VARIANT_TYPE_LONGDOUBLE:
        case PURC_VARIANT_TYPE_DYNAMIC:
            break;

        case PURC_VARIANT_TYPE_NATIVE: {
            const char *name = purc_variant_native_get_name(argv[0]);
            if (strncmp(name, PY_NATIVE_PREFIX, PY_NATIVE_PREFIX_LEN)) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            result = purc_variant_native_get_entity(argv[0]);
            if (result) {
                Py_NewRef(result);
            }

            break;
        }

        default:
            result = make_pyobj_from_variant(pyinfo, argv[0]);
            if (result == NULL)
                goto failed;
            break;
        }
    }

    if (result == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    purc_variant_t ret;
    ret = purc_variant_make_native_entity(result, &native_pyobject_ops,
            PY_NATIVE_PREFIX "any");
    if (ret == PURC_VARIANT_INVALID) {
        Py_DECREF(result);
        goto failed;
    }

    return ret;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t stringify_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    PyObject *result = NULL;

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (!purc_variant_is_native(argv[0])) {
        PyObject *pyobj = make_pyobj_from_variant(pyinfo, argv[0]);
        if (pyobj == NULL)
            goto failed;

        result = PyObject_Str(pyobj);
        Py_DECREF(pyobj);
        if (result == NULL) {
            goto failed_python;
        }
    }
    else {
        const char *name = purc_variant_native_get_name(argv[0]);
        if (strncmp(name, PY_NATIVE_PREFIX, PY_NATIVE_PREFIX_LEN)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        PyObject *pyobj = purc_variant_native_get_entity(argv[0]);
        result = PyObject_Str(pyobj);
        if (result == NULL) {
            goto failed_python;
        }
    }

    assert(PyUnicode_Check(result));
    const char *c_str = PyUnicode_AsUTF8(result);
    if (c_str == NULL) {
        handle_python_error(pyinfo);
        goto failed_python;
    }

    purc_variant_t ret = purc_variant_make_string(c_str, false);
    Py_DECREF(result);
    if (ret == PURC_VARIANT_INVALID)
        goto failed;
    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t code_eval_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *def_globals, *def_locals;

    PyObject *m = PyImport_AddModule("__main__");
    def_globals = PyModule_GetDict(m);

    purc_variant_t val;
    val = purc_variant_object_get_by_ckey(root, PY_KEY_LOCAL);
    assert(val && purc_variant_is_native(val));
    def_locals = purc_variant_native_get_entity(val);

    val = purc_variant_object_get_by_ckey(root, PY_KEY_HANDLE);
    assert(val && purc_variant_is_native(val));
    PyObject *code = purc_variant_native_get_entity(val);

    PyObject *globals = def_globals, *locals = def_locals;
    if (nr_args > 0) {
        if (!purc_variant_is_null(argv[0])) {
            if (!purc_variant_is_object(argv[0])) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            globals = make_pyobj_from_variant(pyinfo, argv[0]);
            if (globals == NULL) {
                goto failed;
            }
        }
    }

    if (nr_args > 1) {
        if (!purc_variant_is_null(argv[1])) {
            if (!purc_variant_is_object(argv[1])) {
                purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                goto failed;
            }

            locals = make_pyobj_from_variant(pyinfo, argv[1]);
            if (locals == NULL) {
                goto failed;
            }
        }
    }

    PyObject *result = PyEval_EvalCode(code, globals,
            (PyDict_Size(locals) == 0) ? globals : locals);
    if (globals != def_globals)
        Py_DECREF(globals);
    if (locals != def_locals)
        Py_DECREF(locals);
    if (result == NULL) {
        goto failed_python;
    }

    purc_variant_t ret;
    ret = make_variant_from_pyobj(result);
    if (ret == PURC_VARIANT_INVALID)
        goto failed;

    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t code_entity_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    purc_variant_t val = purc_variant_object_get_by_ckey(root, PY_KEY_HANDLE);
    assert(val && purc_variant_is_native(val));

    return purc_variant_ref(val);
}

static purc_variant_t compile_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    PyObject *pycode = NULL, *locals = NULL;
    purc_variant_t ret = PURC_VARIANT_INVALID, val = PURC_VARIANT_INVALID;
    const char *code;
    size_t code_len;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (nr_args > 0) {
        code = purc_variant_get_string_const_ex(argv[0], &code_len);
        if (code == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto failed;
        }

        code = pcutils_trim_spaces(code, &code_len);
        if (code_len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    PyCompilerFlags cf = _PyCompilerFlags_INIT;
    cf.cf_flags |= PyCF_IGNORE_COOKIE;
    cf.cf_flags |= PyCF_SOURCE_IS_UTF8;
    pycode = Py_CompileStringFlags(code, "hvml.py", Py_eval_input, &cf);
    if (pycode == NULL)
        goto failed_python;

    locals = PyDict_New();
    if (locals == NULL)
        goto failed_python;

    static struct purc_dvobj_method methods[] = {
        { PY_KEY_EVAL,      code_eval_getter,       NULL },
        { PY_KEY_ENTITY,    code_entity_getter,     NULL },
    };

    ret = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
    if (ret == PURC_VARIANT_INVALID) {
        goto failed;
    }

    if ((val = purc_variant_make_native_entity(locals,
                    &native_pyobject_ops,
                    PY_NATIVE_PREFIX "dict")) == NULL)
        goto failed;
    if (!purc_variant_object_set_by_static_ckey(ret, PY_KEY_LOCAL, val))
        goto failed;
    purc_variant_unref(val);

    static struct purc_native_ops native_pycode_ops = {
        .on_release = on_release_pyobject,
    };

    if ((val = purc_variant_make_native_entity(pycode,
                    &native_pycode_ops, PY_NATIVE_PREFIX "code")) == NULL)
        goto failed;
    if (!purc_variant_object_set_by_static_ckey(ret, PY_KEY_HANDLE, val))
        goto failed;
    purc_variant_unref(val);

    return ret;

failed_python:
    handle_python_error(pyinfo);
    Py_XDECREF(locals);
failed:
    Py_XDECREF(pycode);
    if (val)
        purc_variant_unref(val);
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t make_impl_object(void)
{
    static const char *kvs[] = {
        "vendor",
        "HVML Community",
        "author",
        "Vincent Wei",
        "verName",
        PY_DVOBJ_VERNAME,
        "verCode",
        PY_DVOBJ_VERCODE_STR,
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
        if (val == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(retv, kvs[i], val))
            goto fatal;
        purc_variant_unref(val);
    }

    return retv;

fatal:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}

#if 0 /* deprecated */
static purc_variant_t info_path_getter(purc_variant_t root,
        size_t nr_args, purc_variant_t * argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    char *path = Py_EncodeLocale(Py_GetPath(), NULL);
    if (path) {
        purc_variant_t ret = purc_variant_make_string(path, false);
        PyMem_Free(path);
        return ret;
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t info_path_setter(purc_variant_t root,
        size_t nr_args, purc_variant_t * argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *str = purc_variant_get_string_const(argv[0]);
    if (str == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    wchar_t *path = Py_DecodeLocale(str, NULL);
    if (path == NULL) {
        purc_set_error(PURC_ERROR_BAD_STDC_CALL);
        goto failed;
    }

    Py_SetPath(path);
    PyMem_RawFree(path);
    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}
#endif

static purc_variant_t make_info_object(void)
{
    purc_variant_t retv = PURC_VARIANT_INVALID;
    purc_variant_t val  = PURC_VARIANT_INVALID;

    retv = purc_variant_make_object_0();
    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    val = purc_variant_make_string_static(Py_GetVersion(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_VERSION, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string_static(Py_GetPlatform(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_PLATFORM, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string_static(Py_GetCopyright(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_COPYRIGHT, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string_static(Py_GetCompiler(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_COMPILER, val))
        goto fatal;
    purc_variant_unref(val);

    val = purc_variant_make_string_static(Py_GetBuildInfo(), false);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_BUILD_INFO, val))
        goto fatal;
    purc_variant_unref(val);

#if 0 /* deprecated */
    val = purc_variant_make_dynamic(info_path_getter, info_path_setter);
    if (val == PURC_VARIANT_INVALID)
        goto fatal;
    if (!purc_variant_object_set_by_static_ckey(retv,
                PY_INFO_PATH, val))
        goto fatal;
    purc_variant_unref(val);
#endif

    return retv;

fatal:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}

static bool on_py_being_released(purc_variant_t src, pcvar_op_t op,
        void *ctxt, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    if (op == PCVAR_OPERATION_RELEASING) {
        struct dvobj_pyinfo *pyinfo = ctxt;

        purc_variant_revoke_listener(src, pyinfo->listener);
        Py_DECREF(pyinfo->locals);

        assert(Py_IsInitialized());
        Py_Finalize();

        pcutils_map_destroy(pyinfo->reserved_symbols);
        free(pyinfo);
    }

    return true;
}

static purc_variant_t create_py(void)
{
    static struct purc_dvobj_method methods[] = {
        { PY_KEY_RUN,           run_getter,         NULL },
        { PY_KEY_IMPORT,        import_getter,      NULL },
        { PY_KEY_PYTHONIZE,     pythonize_getter,   NULL },
        { PY_KEY_STRINGIFY,     stringify_getter,   NULL },
        { PY_KEY_COMPILE,       compile_getter,     NULL },
    };

    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    if (keywords2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
            keywords2atoms[i].atom =
                purc_atom_from_static_string_ex(ATOM_BUCKET_DVOBJ,
                    keywords2atoms[i].keyword);
        }
    }

    purc_variant_t py = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;

    PyObject *m = PyImport_AddModule("__main__");
    if (m == NULL)
        goto fatal;

    py = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
    struct dvobj_pyinfo *pyinfo = NULL;
    if (py) {
        pyinfo = calloc(1, sizeof(*pyinfo));
        if (pyinfo == NULL)
            goto failed_info;
        pyinfo->reserved_symbols = pcutils_map_create(
                NULL, NULL, NULL, NULL, comp_key_string, true);
        if (pyinfo->reserved_symbols == NULL)
            goto failed_info;

        pyinfo->locals = PyDict_New();
        if (pyinfo->locals == NULL)
            goto failed_info;

        pyinfo->root = py;

        PyObject *cap = PyCapsule_New(pyinfo, PY_ATTR_HVML, NULL);
        if (cap == NULL)
            goto failed_info;
        if (PyObject_SetAttrString(m, PY_ATTR_HVML, cap))
            goto failed_info;

        /* reserved symbols */
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_IMPL, NULL);
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_INFO, NULL);
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_EXCEPT, NULL);
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_GLOBAL, NULL);
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_LOCAL, NULL);
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_RUN, NULL);
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_IMPORT, NULL);
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_STRINGIFY, NULL);
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_COMPILE, NULL);
        pcutils_map_insert(pyinfo->reserved_symbols, PY_KEY_HANDLE, NULL);

        if ((val = make_impl_object()) == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_IMPL, val))
            goto fatal;
        purc_variant_unref(val);

        if ((val = make_info_object()) == PURC_VARIANT_INVALID)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_INFO, val))
            goto fatal;
        purc_variant_unref(val);

        PyObject *globals = PyModule_GetDict(m);
        if ((val = purc_variant_make_native_entity(globals,
                        &native_pyobject_ops, PY_NATIVE_PREFIX "dict")) == NULL)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_GLOBAL, val))
            goto fatal;
        purc_variant_unref(val);

        if ((val = purc_variant_make_native_entity(pyinfo->locals,
                        &native_pyobject_ops, PY_NATIVE_PREFIX "dict")) == NULL)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_LOCAL, val))
            goto fatal;
        purc_variant_unref(val);

        if ((val = purc_variant_make_native((void *)pyinfo, NULL)) == NULL)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_HANDLE, val))
            goto fatal;
        purc_variant_unref(val);

        val = purc_variant_make_null();
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_EXCEPT, val))
            goto fatal;
        purc_variant_unref(val);

        pyinfo->listener = purc_variant_register_post_listener(py,
                PCVAR_OPERATION_RELEASING, on_py_being_released, pyinfo);
        return py;
    }

failed_info:
    if (pyinfo) {
        if (pyinfo->reserved_symbols) {
            pcutils_map_destroy(pyinfo->reserved_symbols);
        }
        Py_XDECREF(pyinfo->locals);
        free(pyinfo);
    }

fatal:
    if (val)
        purc_variant_unref(val);
    if (py)
        purc_variant_unref(py);

    pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
    return PURC_VARIANT_INVALID;
}

static struct dvobj_info {
    const char *name;
    const char *desc;
    purc_variant_t (*create_func)(void);
} dvobjs[] = {
    {
        "PY",                                       // name
        "Implementaion of $PY based on CPython",    // description
        create_py                                   // create function
    },
};

#ifndef NDEBUG
static void test_split_module_names(void)
{
    /* test some internal helpers */
    static const char *good_strings[] = {
        "foo",
        "foo.foo:foo",
        "foo.foo",
    };
    static const char *bad_strings[] = {
        "foofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoo",
        "foo bar",
        "foo.",
        "foo.:",
        " asfa sdf ",
        ".asfa sdf:",
        "foo.foo bar:",
        "foo.foo:",
        "foo.foo:foo bar",
        "foo.foofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoo:foo",
        "foo.foo:foofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoo",
    };

    char package_name[MAX_SYMBOL_LEN + 1];
    char module_name[MAX_SYMBOL_LEN + 1];
    char module_aliase[MAX_SYMBOL_LEN + 1];
    for (size_t i = 0; i < PCA_TABLESIZE(good_strings); i++) {
        int ret = split_module_names(good_strings[i],
                strlen(good_strings[i]),
                package_name, module_name, module_aliase);

        assert(ret == 0);
        assert(strcmp(module_name, "foo") == 0);
    }

    for (size_t i = 0; i < PCA_TABLESIZE(bad_strings); i++) {
        int ret = split_module_names(bad_strings[i],
                strlen(bad_strings[i]),
                package_name, module_name, module_aliase);

        if (ret == 0)
            printf("module_name %d): %s\n", (int)i, module_name);
        assert(ret != 0);
    }
}

static void test_split_symbol_names(void)
{
    /* test some internal helpers */
    static const char *good_strings[] = {
        "foo",
        "foo:foo",
    };
    static const char *bad_strings[] = {
        "foofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoo",
        "foo bar",
        "foo:",
        "foo.:",
        " asfa sdf ",
        "asfa sdf:",
        "foo.foo bar:",
        "foo.foo:",
        "foo:",
        "foo:foo bar",
        "foofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoo:foo",
        "foo:foofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoofoo",
    };

    char symbol_name[MAX_SYMBOL_LEN + 1];
    char symbol_aliase[MAX_SYMBOL_LEN + 1];
    for (size_t i = 0; i < PCA_TABLESIZE(good_strings); i++) {
        int ret = split_symbol_names(good_strings[i],
                strlen(good_strings[i]), symbol_name, symbol_aliase);

        assert(ret == 0);
        assert(strcmp(symbol_name, "foo") == 0);
    }

    for (size_t i = 0; i < PCA_TABLESIZE(bad_strings); i++) {
        int ret = split_symbol_names(bad_strings[i],
                strlen(bad_strings[i]), symbol_name, symbol_aliase);
        assert(ret != 0);
    }
}

#endif /* not defined NDEBUG */

purc_variant_t __purcex_load_dynamic_variant(const char *name, int *ver_code)
{
    size_t i = 0;
    for (i = 0; i < PCA_TABLESIZE(dvobjs); i++) {
        if (strcasecmp(name, dvobjs[i].name) == 0)
            break;
    }

#ifndef NDEBUG
    assert(PY_NATIVE_PREFIX_LEN > 8);
    test_split_module_names();
    test_split_symbol_names();
#endif

    if (i == PCA_TABLESIZE(dvobjs))
        return PURC_VARIANT_INVALID;
    else {
        *ver_code = PY_DVOBJ_VERCODE;
        return dvobjs[i].create_func();
    }
}

size_t __purcex_get_number_of_dynamic_variants(void)
{
    return PCA_TABLESIZE(dvobjs);
}

const char *__purcex_get_dynamic_variant_name(size_t idx)
{
    if (idx >= PCA_TABLESIZE(dvobjs))
        return NULL;
    else
        return dvobjs[idx].name;
}

const char *__purcex_get_dynamic_variant_desc(size_t idx)
{
    if (idx >= PCA_TABLESIZE(dvobjs))
        return NULL;
    else
        return dvobjs[idx].desc;
}

