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

#undef NDEBUG

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
#define PY_KEY_STRINGIFY    "stringify"
#define PY_KEY_COMPILE      "compile"
#define PY_KEY_EVAL         "eval"
#define PY_KEY_HANDLE       "__handle_python__"

#define PY_INFO_VERSION     "version"
#define PY_INFO_PLATFORM    "platform"
#define PY_INFO_COPYRIGHT   "copyright"
#define PY_INFO_COMPILER    "compiler"
#define PY_INFO_BUILD_INFO  "build-info"

#define PY_NATIVE_PREFIX        "pyObject::"
#define PY_NATIVE_PREFIX_LEN    (sizeof(PY_NATIVE_PREFIX) - 1)
#define PY_ATTR_HVML            "__hvml__"

#define _KW_DELIMITERS          " \t\n\v\f\r"

enum {
#define _KW_command                 "command"
    K_KW_command,
#define _KW_module                  "module"
    K_KW_module,
#define _KW_file                    "file"
    K_KW_file,
#define _KW_skip_first_line         "skip-first-line"
    K_KW_skip_first_line,
#define _KW_dont_write_byte_code    "dont-write-byte-code"
    K_KW_dont_write_byte_code,
#define _KW_return_stdout           "return-stdout"
    K_KW_return_stdout,
};

static struct keyword_to_atom {
    const char *keyword;
    purc_atom_t atom;
} keywords2atoms [] = {
    { _KW_command, 0 },                 // "command"
    { _KW_module, 0 },                  // "module"
    { _KW_file, 0 },                    // "file"
    { _KW_skip_first_line, 0 },         // "skip-first-line"
    { _KW_dont_write_byte_code, 0 },    // "dont-write-byte-code"
    { _KW_return_stdout, 0 },           // "return-stdout"
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
    pcutils_map *reserved_symbols;
    PyObject *locals;                   // local variables
    purc_variant_t root;                // the root variant, i.e., $PY itself
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
    int hvml_err = PURC_ERROR_OK;

    PyObject *pyerr = PyErr_Occurred();
    if (pyerr == NULL)
        return;

    if (PyErr_GivenExceptionMatches(pyerr, PyExc_AssertionError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "AssertionError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_AttributeError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "AttributeError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BlockingIOError)) {
        hvml_err = PURC_ERROR_IO_FAILURE;
        set_python_except(pyinfo, "BlockingIOError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BrokenPipeError)) {
        hvml_err = PURC_ERROR_BROKEN_PIPE;
        set_python_except(pyinfo, "BrokenPipeError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BufferError)) {
        hvml_err = PURC_ERROR_IO_FAILURE;
        set_python_except(pyinfo, "BufferError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ChildProcessError)) {
        hvml_err = PURC_ERROR_CHILD_TERMINATED;
        set_python_except(pyinfo, "ChildProcessError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionAbortedError)) {
        hvml_err = PURC_ERROR_CONNECTION_ABORTED;
        set_python_except(pyinfo, "ConnectionAbortedError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionRefusedError)) {
        hvml_err = PURC_ERROR_CONNECTION_REFUSED;
        set_python_except(pyinfo, "ConnectionRefusedError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionResetError)) {
        hvml_err = PURC_ERROR_CONNECTION_RESET;
        set_python_except(pyinfo, "ConnectionResetError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "ConnectionError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_EOFError)) {
        hvml_err = PURC_ERROR_IO_FAILURE;
        set_python_except(pyinfo, "EOFError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_FileExistsError)) {
        hvml_err = PURC_ERROR_EXISTS;
        set_python_except(pyinfo, "FileExistsError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_FileNotFoundError)) {
        hvml_err = PURC_ERROR_NOT_EXISTS;
        set_python_except(pyinfo, "FileNotFoundError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_FloatingPointError)) {
        hvml_err = PURC_ERROR_INVALID_FLOAT;
        set_python_except(pyinfo, "FloatingPointError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_GeneratorExit)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "GeneratorExit");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ImportError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "ImportError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_IndentationError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "IndentationError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_IndexError)) {
        hvml_err = PCVRNT_ERROR_OUT_OF_BOUNDS;
        set_python_except(pyinfo, "IndexError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_InterruptedError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "InterruptedError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_IsADirectoryError)) {
        hvml_err = PURC_ERROR_NOT_DESIRED_ENTITY;
        set_python_except(pyinfo, "IsADirectoryError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_KeyError)) {
        hvml_err = PCVRNT_ERROR_NO_SUCH_KEY;
        set_python_except(pyinfo, "KeyError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_KeyboardInterrupt)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "KeyboardInterrupt");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_MemoryError)) {
        hvml_err = PURC_ERROR_OUT_OF_MEMORY;
        set_python_except(pyinfo, "MemoryError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ModuleNotFoundError)) {
        hvml_err = PURC_ERROR_ENTITY_NOT_FOUND;
        set_python_except(pyinfo, "ModuleNotFoundError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_NameError)) {
        hvml_err = PURC_ERROR_BAD_NAME;
        set_python_except(pyinfo, "NameError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_NotADirectoryError)) {
        hvml_err = PURC_ERROR_NOT_DESIRED_ENTITY;
        set_python_except(pyinfo, "NotADirectoryError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_NotImplementedError)) {
        hvml_err = PURC_ERROR_NOT_IMPLEMENTED;
        set_python_except(pyinfo, "NotImplementedError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_OSError)) {
        hvml_err = PURC_ERROR_SYS_FAULT;
        set_python_except(pyinfo, "OSError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_OverflowError)) {
        hvml_err = PURC_ERROR_OVERFLOW;
        set_python_except(pyinfo, "OverflowError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_PermissionError)) {
        hvml_err = PURC_ERROR_ACCESS_DENIED;
        set_python_except(pyinfo, "PermissionError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ProcessLookupError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "ProcessLookupError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_RecursionError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "RecursionError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ReferenceError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "ReferenceError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_RuntimeError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "RuntimeError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_StopAsyncIteration)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "StopAsyncIteration");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_StopIteration)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "StopIteration");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_SyntaxError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "SyntaxError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_SystemError)) {
        hvml_err = PURC_ERROR_SYS_FAULT;
        set_python_except(pyinfo, "SystemError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_SystemExit)) {
        hvml_err = PURC_ERROR_SYS_FAULT;
        set_python_except(pyinfo, "SystemExit");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_TabError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "TabError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_TimeoutError)) {
        hvml_err = PURC_ERROR_TIMEOUT;
        set_python_except(pyinfo, "TimeoutError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_TypeError)) {
        hvml_err = PURC_ERROR_WRONG_DATA_TYPE;
        set_python_except(pyinfo, "TypeError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnboundLocalError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "UnboundLocalError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeDecodeError)) {
        hvml_err = PURC_ERROR_BAD_ENCODING;
        set_python_except(pyinfo, "UnicodeDecodeError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeEncodeError)) {
        hvml_err = PURC_ERROR_BAD_ENCODING;
        set_python_except(pyinfo, "UnicodeEncodeError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeError)) {
        hvml_err = PURC_ERROR_BAD_ENCODING;
        set_python_except(pyinfo, "UnicodeError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeTranslateError)) {
        hvml_err = PURC_ERROR_BAD_ENCODING;
        set_python_except(pyinfo, "UnicodeTranslateError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ValueError)) {
        hvml_err = PURC_ERROR_INVALID_VALUE;
        set_python_except(pyinfo, "ValueError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ZeroDivisionError)) {
        hvml_err = PURC_ERROR_DIVBYZERO;
        set_python_except(pyinfo, "ZeroDivisionError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ArithmeticError)) {
        /* The base class for those built-in exceptions that are raised for
           various arithmetic errors: OverflowError, ZeroDivisionError,
           and FloatingPointError. */
        hvml_err = PURC_ERROR_INVALID_FLOAT;
        set_python_except(pyinfo, "ArithmeticError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_LookupError)) {
        /* The base class for the exceptions that are raised when
           a key or index used on a mapping or sequence is invalid:
           IndexError, KeyError. */
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "LookupError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_Exception)) {
        /* All built-in, non-system-exiting exceptions are derived from
           this class. All user-defined exceptions should also be derived
           from this class. */
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "Exception");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BaseException)) {
        /* The base class for all built-in exceptions. */
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(pyinfo, "BaseException");
    }

    if (hvml_err != PURC_ERROR_OK) {
        purc_set_error(hvml_err);
        PyErr_Clear();
    }
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
        case PURC_VARIANT_TYPE_STRING:
            {
                const char *string;
                size_t length;
                string = purc_variant_get_string_const_ex(v, &length);
                assert(string);
                pyobj = PyUnicode_DecodeUTF8(string, length, NULL);
                break;
            }

        case PURC_VARIANT_TYPE_NUMBER:
            {
                double d;
                purc_variant_cast_to_number(v, &d, false);
                pyobj = PyFloat_FromDouble(d);
                break;
            }

        case PURC_VARIANT_TYPE_LONGINT:
            {
                int64_t l;
                purc_variant_cast_to_longint(v, &l, false);
                if (sizeof(long long) >= sizeof(int64_t))
                    pyobj = PyLong_FromLongLong((long long)l);
                else
                    pyobj = PyLong_FromDouble((double)l);
                break;
            }

        case PURC_VARIANT_TYPE_ULONGINT:
            {
                uint64_t ul;
                purc_variant_cast_to_ulongint(v, &ul, false);
                if (sizeof(unsigned long long) >= sizeof(int64_t))
                    pyobj = PyLong_FromUnsignedLongLong((unsigned long long)ul);
                else
                    pyobj = PyLong_FromDouble((double)ul);
                break;
            }

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            {
                long double ld;
                purc_variant_cast_to_longdouble(v, &ld, false);
                pyobj = PyFloat_FromDouble((double)ld);
                break;
            }

        case PURC_VARIANT_TYPE_BSEQUENCE:
            {
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

        case PURC_VARIANT_TYPE_NATIVE:
            {
                const char *name = purc_variant_native_get_name(v);
                if (strncmp(name, PY_NATIVE_PREFIX, PY_NATIVE_PREFIX_LEN) == 0) {
                    pyobj = purc_variant_native_get_entity(v);
                    pyobj = Py_NewRef(pyobj);
                }
                else {
                    pyobj = Py_NewRef(Py_None);
                }
                break;
            }

        case PURC_VARIANT_TYPE_OBJECT:
            {
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

        case PURC_VARIANT_TYPE_ARRAY:
            {
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

        case PURC_VARIANT_TYPE_TUPLE:
            {
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

        case PURC_VARIANT_TYPE_SET:
            {
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

static purc_variant_t make_variant_from_pyobj(struct dvobj_pyinfo *pyinfo,
        PyObject *pyobj);

static purc_variant_t pyobject_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    struct dvobj_pyinfo *pyinfo = get_pyinfo();

    PyObject *pyobj;
    if (property_name == NULL) {
        pyobj = native_entity;
    }
    else {
        PyObject *parent = native_entity;
        pyobj = PyObject_GetAttrString(parent, property_name);
        if (pyobj == NULL)
            goto failed_python;
    }

    purc_variant_t ret = make_variant_from_pyobj(pyinfo, pyobj);
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

static purc_variant_t pyobject_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();

    if (property_name == NULL) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto failed;
    }

    PyObject *parent = native_entity;
    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (purc_variant_is_undefined(argv[0])) {
        if (PyObject_DelAttrString(parent, property_name))
            goto failed_python;
    }
    else {
        PyObject *val = make_pyobj_from_variant(pyinfo, argv[0]);
        if (val == NULL)
            goto failed;

        if (PyObject_SetAttrString(parent, property_name, val))
            goto failed_python;
    }

    return purc_variant_make_boolean(true);

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static void on_release_pyobject(void* native_entity)
{
    PyObject *pyobj = native_entity;
    Py_DECREF(pyobj);
}

static struct purc_native_ops native_pyobject_ops = {
    .getter = pyobject_getter,
    .setter = pyobject_setter,
    .on_release = on_release_pyobject,
};

static purc_variant_t make_variant_from_pyobj(struct dvobj_pyinfo *pyinfo,
        PyObject *pyobj)
{
    purc_variant_t v = PURC_VARIANT_INVALID;

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
    }
    else if (PyFloat_Check(pyobj)) {
        double d = PyFloat_AsDouble(pyobj);
        v = purc_variant_make_number(d);
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
            hvml_v = make_variant_from_pyobj(pyinfo, value);
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
            purc_variant_t hvml_m = make_variant_from_pyobj(pyinfo, member);
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
            purc_variant_t hvml_m = make_variant_from_pyobj(pyinfo, member);
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
            purc_variant_t hvml_item = make_variant_from_pyobj(pyinfo, item);
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
    /* else if (PyComplex_Check(pyobj)) {
        // TODO: support for complex

        double real = PyComplex_RealAsDouble(pyobj);
        double imag = PyComplex_ImagAsDouble(pyobj);
        char buff[128];
        snprintf(buff, sizeof(buff), "%.6f+%.6fi", real, imag);
        v = purc_variant_make_string(buff, false);
    } */
    else {
        Py_INCREF(pyobj);
        v = purc_variant_make_native_entity(pyobj, &native_pyobject_ops,
                PY_NATIVE_PREFIX "any");
        if (v == PURC_VARIANT_INVALID) {
            Py_DECREF(pyobj);
            goto failed;
        }
    }

    return v;

failed:
    if (v)
        purc_variant_unref(v);
    return PURC_VARIANT_INVALID;
}

enum {
    RUN_OPT_SKIP_FIRST_LINE         = 0x0001,
    RUN_OPT_DONT_WRITE_BYTE_CODE    = 0x0002,
    RUN_OPT_RETURN_STDOUT           = 0x0004,
    RUN_OPT_SET_ARGV0               = 0x0008,
};

static purc_variant_t run_command(purc_variant_t root,
        const char *cmd, size_t len, PyCompilerFlags *cf, unsigned options)
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
    result = PyRun_StringFlags(cmd, Py_eval_input,
            globals, pyinfo->locals, cf);
    if (result == NULL) {
        handle_python_error(pyinfo);
        goto failed;
    }

    purc_variant_t ret = make_variant_from_pyobj(pyinfo, result);
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

    purc_variant_t ret = make_variant_from_pyobj(pyinfo, result);
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
    result = PyRun_FileFlags(fp, fname, Py_eval_input,
            globals, pyinfo->locals, cf);
    if (result == NULL) {
        goto failed;
    }

    fclose(fp);
    purc_variant_t ret = make_variant_from_pyobj(pyinfo, result);
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
            else if (atom == keywords2atoms[K_KW_return_stdout].atom) {
                // return-stdout
                run_options |= RUN_OPT_RETURN_STDOUT;
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
            cf.cf_flags |= PyCF_SOURCE_IS_UTF8;
            ret = run_command(root, cmd_mod_file, len, &cf, run_options);
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

static purc_variant_t pydict_getter(void *native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    PyObject *dict = (PyObject *)native_entity;
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    UNUSED_PARAM(property_name);

    purc_variant_t ret = PURC_VARIANT_INVALID;
    if (nr_args == 0) {
        ret = make_variant_from_pyobj(pyinfo, dict);
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

        ret = make_variant_from_pyobj(pyinfo, val);
    }

    return ret;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t pydict_setter(void *native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *dict = (PyObject *)native_entity;
    UNUSED_PARAM(property_name);

    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (nr_args >= 1) {
        if (purc_variant_is_object(argv[0])) {
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
                    handle_python_error(pyinfo);
                    goto failed;
                }
            }
            else {
                PyObject *pyobj = make_pyobj_from_variant(pyinfo, argv[1]);
                if (pyobj == NULL) {
                    goto failed;
                }

                if (PyDict_SetItemString(dict, symbol, pyobj)) {
                    handle_python_error(pyinfo);
                    goto failed;
                }
            }
        }
        else {
            purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
            goto failed;
        }
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t pydict_property_getter(void *native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *dict = (PyObject *)native_entity;
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    assert(property_name);
    PyObject *val = PyDict_GetItemString(dict, property_name);
    if (val == NULL) {
        purc_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
        goto failed;
    }

    return make_variant_from_pyobj(pyinfo, val);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t pydict_property_setter(void *native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();
    PyObject *dict = (PyObject *)native_entity;

    assert(property_name);
    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (purc_variant_is_undefined(argv[0])) {
        if (PyDict_DelItemString(dict, property_name)) {
            goto failed_python;
        }
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

static purc_nvariant_method pydict_property_getter_getter(void* native_entity,
        const char* property_name)
{
    PyObject *dict = (PyObject *)native_entity;
    assert(dict);
    assert(property_name);

    if (PyDict_GetItemString(dict, property_name))
        return pydict_property_getter;

    purc_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
    return NULL;
}

static purc_nvariant_method pydict_property_setter_getter(void* native_entity,
            const char* property_name)
{
    PyObject *dict = (PyObject *)native_entity;
    assert(dict);
    assert(property_name);

    return pydict_property_setter;
#if 0
    if (PyDict_GetItemString(dict, property_name))
        return pydict_property_setter;

    purc_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
    return NULL;
#endif
}

static struct purc_native_ops native_pydict_ops = {
    .getter = pydict_getter,
    .setter = pydict_setter,
    .property_getter = pydict_property_getter_getter,
    .property_setter = pydict_property_setter_getter,
    .on_release = NULL, // on_release_pyobject,
};

static struct purc_native_ops native_pycode_locals_ops = {
    .getter = pydict_getter,
    .setter = pydict_setter,
    .property_getter = pydict_property_getter_getter,
    .property_setter = pydict_property_setter_getter,
    .on_release = on_release_pyobject,
};

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

static purc_variant_t pycallable_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();

    PyObject *callable;
    if (property_name == NULL) {
        callable = native_entity;
    }
    else {
        PyObject *parent = native_entity;
        callable = PyObject_GetAttrString(parent, property_name);
    }

    assert(PyCallable_Check(callable));

    PyObject *result;
    if (nr_args == 0) {
        result = PyObject_CallNoArgs(callable);
    }
    else if (nr_args == 1) {
        PyObject *arg = make_pyobj_from_variant(pyinfo, argv[0]);
        if (arg == NULL)
            goto failed;

        result = PyObject_CallOneArg(callable, arg);
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

        result = PyObject_Call(callable, arg, NULL);
    }

    if (result == NULL)
        goto failed_python;

    purc_variant_t ret = make_variant_from_pyobj(pyinfo, result);
    Py_DECREF(result);
    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t pycallable_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();

    PyObject *callable;
    if (property_name == NULL) {
        callable = native_entity;
    }
    else {
        PyObject *parent = native_entity;
        callable = PyObject_GetAttrString(parent, property_name);
    }

    assert(PyCallable_Check(callable));

    PyObject *args = NULL, *kwargs = NULL;
    PyObject *result = NULL;
    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (!purc_variant_is_object(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    args = PyTuple_New(0);
    if (args == NULL)
        goto failed_python;

    kwargs = make_pyobj_from_variant(pyinfo, argv[0]);
    if (kwargs == NULL)
        goto failed;

    assert(PyDict_Check(kwargs));
    result = PyObject_Call(callable, args, kwargs);
    if (result == NULL) {
        goto failed_python;
    }

    purc_variant_t ret = make_variant_from_pyobj(pyinfo, result);
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

static purc_variant_t pycallable_method_getter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();

    PyObject *callable = native_entity;
    assert(PyCallable_Check(callable));

    PyObject *method = PyObject_GetAttrString(callable, property_name);
    assert(PyMethod_Check(method));

    PyObject *name = PyUnicode_FromString(property_name);
    if (name == NULL)
        goto failed_python;

    PyObject *result;
    if (nr_args == 0) {
        result = PyObject_CallMethodNoArgs(callable, name);
    }
    else if (nr_args == 1) {
        PyObject *arg = make_pyobj_from_variant(pyinfo, argv[0]);
        if (arg == NULL)
            goto failed;

        result = PyObject_CallMethodOneArg(callable, name, arg);
        Py_DECREF(arg);
    }
    else {
        PyObject *vc_args[nr_args + 1]; // use vectorcall
        vc_args[0] = callable;

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

    purc_variant_t ret = make_variant_from_pyobj(pyinfo, result);
    Py_DECREF(result);
    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t pycallable_method_setter(void* native_entity,
        const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo();

    PyObject *callable = native_entity;
    assert(PyCallable_Check(callable));

    PyObject *method = PyObject_GetAttrString(callable, property_name);
    assert(PyMethod_Check(method));

    PyObject *name = PyUnicode_FromString(property_name);
    if (name == NULL)
        goto failed_python;

    PyObject *kwargs = NULL;
    PyObject *result = NULL;
    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (!purc_variant_is_object(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    kwargs = make_pyobj_from_variant(pyinfo, argv[0]);
    if (kwargs == NULL)
        goto failed;

    result = PyObject_VectorcallMethod(name, &callable, 1, kwargs);
    if (result == NULL)
        goto failed_python;

    purc_variant_t ret = make_variant_from_pyobj(pyinfo, result);
    Py_DECREF(kwargs);
    Py_DECREF(result);
    return ret;

failed_python:
    handle_python_error(pyinfo);
failed:
    Py_XDECREF(result);
    Py_XDECREF(kwargs);
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_nvariant_method
pycallable_property_getter_getter(void* native_entity,
        const char* property_name)
{
    PyObject *callable = (PyObject *)native_entity;
    assert(callable);
    assert(property_name);

    PyObject *val = PyObject_GetAttrString(callable, property_name);
    if (val == NULL) {
        purc_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
        return NULL;
    }
    else if (PyMethod_Check(val)) {
        return pycallable_method_getter;
    }
    else if (PyCallable_Check(val)) {
        return pycallable_getter;
    }
    else {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return NULL;
    }
}

static purc_nvariant_method
pycallable_property_setter_getter(void* native_entity,
            const char* property_name)
{
    PyObject *callable = (PyObject *)native_entity;
    assert(callable);
    assert(property_name);

    PyObject *val = PyObject_GetAttrString(callable, property_name);
    if (val == NULL) {
        purc_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
        return NULL;
    }
    else if (PyMethod_Check(val)) {
        return pycallable_method_setter;
    }
    else if (PyCallable_Check(val)) {
        return pycallable_setter;
    }
    else {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return NULL;
    }
}

static struct purc_native_ops native_pycallable_ops = {
    .getter = pycallable_getter,
    .setter = pycallable_setter,
    .property_getter = pycallable_property_getter_getter,
    .property_setter = pycallable_property_setter_getter,
    .on_release = on_release_pyobject,
};

static purc_nvariant_method pymodule_property_getter(void* native_entity,
            const char* property_name)
{
    PyObject *module = native_entity;
    assert(PyModule_Check(module));

    PyObject *obj = PyObject_GetAttrString(module, property_name);
    if (obj == NULL) {
        struct dvobj_pyinfo *pyinfo = get_pyinfo();
        handle_python_error(pyinfo);
        return NULL;
    }
    else if (PyCallable_Check(obj)) {
        return pycallable_getter;
    }

    return pyobject_getter;
}

static purc_nvariant_method pymodule_property_setter(void* native_entity,
            const char* property_name)
{
    PyObject *module = native_entity;
    assert(PyModule_Check(module));

    PyObject *obj = PyObject_GetAttrString(module, property_name);
    if (obj == NULL) {
        struct dvobj_pyinfo *pyinfo = get_pyinfo();
        handle_python_error(pyinfo);
        return NULL;
    }
    else if (PyCallable_Check(obj)) {
        return pycallable_setter;
    }

    return pyobject_setter;
}

static struct purc_native_ops native_pymodule_ops = {
    .property_getter = pymodule_property_getter,
    .property_setter = pymodule_property_setter,
    .on_release = on_release_pyobject,
};

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

    PyObject *module = PyImport_ImportModuleEx(module_name, globals, locals,
            fromlist);
    if (module == NULL) {
        goto failed_python;
    }

    if (PyDict_SetItemString(globals, module_aliase, module)) {
        goto failed_python;
    }

    Py_INCREF(module);
    if ((val = purc_variant_make_native_entity(module,
                    &native_pymodule_ops, PY_NATIVE_PREFIX "module")) == NULL) {
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
                                &native_pycallable_ops,
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
                                &native_pymodule_ops,
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

    purc_variant_t ret = make_variant_from_pyobj(pyinfo, result);
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

    PyObject *result = PyEval_EvalCode(code, globals, locals);
    if (globals != def_globals)
        Py_DECREF(globals);
    if (locals != def_locals)
        Py_DECREF(locals);
    if (result == NULL) {
        goto failed_python;
    }

    purc_variant_t ret;
    ret = make_variant_from_pyobj(pyinfo, result);
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
    };

    ret = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
    if (ret == PURC_VARIANT_INVALID) {
        goto failed;
    }

    if ((val = purc_variant_make_native_entity(locals,
                    &native_pycode_locals_ops,
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

    return retv;

fatal:
    if (val)
        purc_variant_unref(val);
    if (retv)
        purc_variant_unref(retv);

    return PURC_VARIANT_INVALID;
}

static void on_release_pyinfo(void* native_entity)
{
    struct dvobj_pyinfo *pyinfo = native_entity;

    Py_DECREF(pyinfo->locals);

    assert(Py_IsInitialized());
    Py_Finalize();

    pcutils_map_destroy(pyinfo->reserved_symbols);
    free(pyinfo);
}

static purc_variant_t create_py(void)
{
    static struct purc_dvobj_method methods[] = {
        { PY_KEY_RUN,           run_getter,         NULL },
        { PY_KEY_IMPORT,        import_getter,      NULL },
        { PY_KEY_STRINGIFY,     stringify_getter,   NULL },
        { PY_KEY_COMPILE,       compile_getter,     NULL },
    };

    static struct purc_native_ops pyinfo_ops = {
        .on_release = on_release_pyinfo,
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
                        &native_pydict_ops, PY_NATIVE_PREFIX "dict")) == NULL)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_GLOBAL, val))
            goto fatal;
        purc_variant_unref(val);

        if ((val = purc_variant_make_native_entity(pyinfo->locals,
                        &native_pydict_ops, PY_NATIVE_PREFIX "dict")) == NULL)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_LOCAL, val))
            goto fatal;
        purc_variant_unref(val);

        if ((val = purc_variant_make_native((void *)pyinfo,
                        &pyinfo_ops)) == NULL)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_HANDLE, val))
            goto fatal;
        purc_variant_unref(val);

        val = purc_variant_make_null();
        if (!purc_variant_object_set_by_static_ckey(py, PY_KEY_EXCEPT, val))
            goto fatal;
        purc_variant_unref(val);
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

#if 0 /* deprecated */
#define TMP_FILE_TEMPLATE       "/tmp/hvml-py-XXXXXX"

static int redirect_stdout(char *tmpfile)
{
    int fd = mkstemp(tmpfile);
    if (fd >= 0) {
        close(fd);
        FILE *fp = freopen(tmpfile, "w+", stdout);
        if (fp == NULL)
            goto failed;
        return 0;
    }

failed:
    return -1;
}

static purc_variant_t restore_stdout(void)
{
    long sz;

    if (fseek(stdout, 0L, SEEK_END))
        goto failed;

    sz = ftell(stdout);
    char *p = malloc(sz + 1);
    if (p == NULL)
        goto failed;

    rewind(stdout);
    size_t read = fread(p, 1, sz, stdout);
    p[read] = '\0';

    fclose(stdout);
    stdout = fdopen(STDOUT_FILENO, "w");

    return purc_variant_make_string_reuse_buff(p, sz + 1, false);

failed:
    return purc_variant_make_string_static("", false);
}

static purc_variant_t global_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    purc_variant_t ret = PURC_VARIANT_INVALID;
    PyObject *m, *globals;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        goto failed;

    globals = PyModule_GetDict(m);
    if (nr_args == 0) {
        ret = make_variant_from_pyobj(pyinfo, globals);
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

        PyObject *val = PyDict_GetItemString(globals, symbol);
        if (val == NULL) {
            purc_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
            goto failed;
        }

        ret = make_variant_from_pyobj(pyinfo, val);
    }

    return ret;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t global_setter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    PyObject *m, *globals;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        goto failed;

    globals = PyModule_GetDict(m);
    if (nr_args > 1) {

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
            if (PyDict_DelItemString(globals, symbol)) {
                handle_python_error(pyinfo);
                goto failed;
            }
        }
        else {
            PyObject *pyobj = make_pyobj_from_variant(pyinfo, argv[1]);
            if (pyobj == NULL) {
                goto failed;
            }

            if (PyDict_SetItemString(globals, symbol, pyobj)) {
                handle_python_error(pyinfo);
                goto failed;
            }
        }
    }
    else {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t local_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    PyObject *locals = pyinfo->locals;
    purc_variant_t ret;

    if (nr_args == 0) {
        ret = make_variant_from_pyobj(pyinfo, locals);
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

        PyObject *val = PyDict_GetItemString(locals, symbol);
        if (val == NULL) {
            purc_set_error(PCVRNT_ERROR_NO_SUCH_KEY);
            goto failed;
        }

        ret = make_variant_from_pyobj(pyinfo, val);
    }

    return ret;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t local_setter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_pyinfo *pyinfo = get_pyinfo_from_root(root);
    PyObject *locals = pyinfo->locals;

    if (nr_args > 1) {

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
            if (PyDict_DelItemString(locals, symbol)) {
                handle_python_error(pyinfo);
                goto failed;
            }
        }
        else {
            PyObject *pyobj = make_pyobj_from_variant(pyinfo, argv[1]);
            if (pyobj == NULL) {
                goto failed;
            }

            if (PyDict_SetItemString(locals, symbol, pyobj)) {
                handle_python_error(pyinfo);
                goto failed;
            }
        }
    }
    else {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t call_pyfunc(void* native_entity,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    return purc_variant_make_undefined();
}

static purc_nvariant_method property_getter(void* native_entity,
            const char* property_name)
{
    struct dvobj_pyinfo *pyinfo = native_entity;
    assert(&pyinfo == pyinfo);

    pcutils_map_entry *entry = pcutils_map_find(pyinfo->reserved_symbols, property_name);
    if (entry) {
        PyObject *o = (PyObject *)entry->val;
        if (PyFunction_Check(o)) {
            return call_pyfunc;
        }
        else if (PyModule_Check(o)) {
            /* We use a Python Capsule to store the variant created for
               the module */
            PyObject *cap = PyObject_GetAttrString(o, PY_ATTR_HVML);
            if (PyCapsule_CheckExact(cap)) {
                purc_variant_t mod_entity;
                mod_entity = PyCapsule_GetPointer(p, NULL);
                assert(mod_entity);

                return 
            }
        }
    }

    return NULL;
}
#endif /* deprecated */

