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

#include "config.h"
#include "private/map.h"
#include "private/dvobjs.h"
#include "private/instance.h"
#include "private/atom-buckets.h"
#include "purc-variant.h"
#include "purc-errors.h"

#include <Python.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#define PY_DVOBJ_VERNAME        "0.1.0"
#define PY_DVOBJ_VERCODE        0

#define STR(x)                  #x
#define STR2(x)                 STR(x)
#define PY_DVOBJ_VERCODE_STR    STR2(PY_DVOBJ_VERCODE)

#define PY_KEY_IMPL         "impl"
#define PY_KEY_INFO         "info"
#define PY_KEY_EXCEPT       "except"
#define PY_KEY_RUN          "run"
#define PY_KEY_IMPORT       "import"

#define PY_INFO_VERSION     "version"
#define PY_INFO_PLATFORM    "platform"
#define PY_INFO_COPYRIGHT   "copyright"
#define PY_INFO_COMPILER    "compiler"
#define PY_INFO_BUILD_INFO  "build-info"

#define PY_HANDLE_NAME      "__handle_python__"
#define PY_ATTR_HVML        "__hvml__"

#define _KW_DELIMITERS      " \t\n\v\f\r"

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

struct dvobj_pyinfo {
    pcutils_map *prop_map;
};

static inline struct dvobj_pyinfo *
get_pyinfo(purc_variant_t root)
{
    purc_variant_t v;

    v = purc_variant_object_get_by_ckey(root, PY_HANDLE_NAME);
    assert(v && purc_variant_is_native(v));

    return (struct dvobj_pyinfo *)purc_variant_native_get_entity(v);
}

static int set_python_except(purc_variant_t root, const char *except)
{
    int ret = 0;
    purc_variant_t val = purc_variant_make_string_static(except, false);
    if (!purc_variant_object_set_by_static_ckey(root, PY_KEY_EXCEPT, val))
        ret = -1;
    purc_variant_unref(val);
    return ret;
}

static purc_variant_t make_variant_from_pyobj(PyObject *pyobj)
{
    if (pyobj == Py_None) {
        return purc_variant_make_null();
    }
    else if (PyBool_Check(pyobj)) {
        if (pyobj == Py_True)
            return purc_variant_make_boolean(true);
        else
            return purc_variant_make_boolean(false);
    }
    else if (PyLong_Check(pyobj)) {
        int overflow;
        long l = PyLong_AsLongAndOverflow(pyobj, &overflow);
        if (overflow) {
            long long ll = PyLong_AsLongLongAndOverflow(pyobj, &overflow);
            if (overflow) {
                double d = PyLong_AsDouble(pyobj);
                return purc_variant_make_number(d);
            }

            if (sizeof(long long) <= sizeof(int64_t)) {
                return purc_variant_make_longint((int64_t)ll);
            }
            else {
                double d = PyLong_AsDouble(pyobj);
                return purc_variant_make_number(d);
            }
        }

        return purc_variant_make_longint((int64_t)l);
    }
    else if (PyFloat_Check(pyobj)) {
        double d = PyFloat_AsDouble(pyobj);
        return purc_variant_make_number(d);
    }
    else if (PyComplex_Check(pyobj)) {
        // TODO: support for complex

        double real = PyComplex_RealAsDouble(pyobj);
        double imag = PyComplex_ImagAsDouble(pyobj);
        char buff[128];
        snprintf(buff, sizeof(buff), "%.6f+%.6fi", real, imag);
        return purc_variant_make_string(buff, false);
    }
    else if (PyBytes_Check(pyobj)) {
        char *buffer;
        Py_ssize_t length;
        PyBytes_AsStringAndSize(pyobj, &buffer, &length);
        return purc_variant_make_byte_sequence(buffer, length);
    }
    else if (PyByteArray_Check(pyobj)) {
        char *buffer = PyByteArray_AS_STRING(pyobj);
        Py_ssize_t length = PyByteArray_GET_SIZE(pyobj);
        return purc_variant_make_byte_sequence(buffer, length);
    }
    else if (PyUnicode_Check(pyobj)) {
        PyObject *bytes = PyUnicode_AsUTF8String(pyobj);

        char *buffer;
        buffer = PyBytes_AsString(bytes);
        return purc_variant_make_string(buffer, false);
    }

    // TODO: more Python objects.
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return PURC_VARIANT_INVALID;
}

static void handle_pyerr(purc_variant_t root)
{
    int hvml_err = PURC_ERROR_OK;

    PyObject *pyerr = PyErr_Occurred();
    if (pyerr == NULL)
        return;
    if (PyErr_GivenExceptionMatches(pyerr, PyExc_ArithmeticError)) {
        hvml_err = PURC_ERROR_INVALID_FLOAT;
        set_python_except(root, "ArithmeticError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_AssertionError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "AssertionError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_AttributeError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "AttributeError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BlockingIOError)) {
        hvml_err = PURC_ERROR_IO_FAILURE;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BrokenPipeError)) {
        hvml_err = PURC_ERROR_BROKEN_PIPE;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_BufferError)) {
        hvml_err = PURC_ERROR_IO_FAILURE;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ChildProcessError)) {
        hvml_err = PURC_ERROR_CHILD_TERMINATED;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionAbortedError)) {
        hvml_err = PURC_ERROR_CONNECTION_ABORTED;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionRefusedError)) {
        hvml_err = PURC_ERROR_CONNECTION_REFUSED;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionResetError)) {
        hvml_err = PURC_ERROR_CONNECTION_RESET;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ConnectionError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "ConnectionError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_EOFError)) {
        hvml_err = PURC_ERROR_IO_FAILURE;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_FileExistsError)) {
        hvml_err = PURC_ERROR_EXISTS;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_FileNotFoundError)) {
        hvml_err = PURC_ERROR_NOT_EXISTS;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_FloatingPointError)) {
        hvml_err = PURC_ERROR_INVALID_FLOAT;
        set_python_except(root, "FloatingPointError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_GeneratorExit)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "GeneratorExit");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ImportError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "ImportError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_IndentationError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "IndentationError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_IndexError)) {
        hvml_err = PCVRNT_ERROR_OUT_OF_BOUNDS;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_InterruptedError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "InterruptedError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_IsADirectoryError)) {
        hvml_err = PURC_ERROR_NOT_DESIRED_ENTITY;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_KeyError)) {
        hvml_err = PCVRNT_ERROR_NO_SUCH_KEY;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_KeyboardInterrupt)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "KeyboardInterrupt");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_LookupError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "LookupError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_MemoryError)) {
        hvml_err = PURC_ERROR_OUT_OF_MEMORY;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ModuleNotFoundError)) {
        hvml_err = PURC_ERROR_ENTITY_NOT_FOUND;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_NameError)) {
        hvml_err = PURC_ERROR_BAD_NAME;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_NotADirectoryError)) {
        hvml_err = PURC_ERROR_NOT_DESIRED_ENTITY;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_NotImplementedError)) {
        hvml_err = PURC_ERROR_NOT_IMPLEMENTED;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_OSError)) {
        hvml_err = PURC_ERROR_SYS_FAULT;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_OverflowError)) {
        hvml_err = PURC_ERROR_OVERFLOW;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_PermissionError)) {
        hvml_err = PURC_ERROR_ACCESS_DENIED;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ProcessLookupError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "ProcessLookupError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_RecursionError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "RecursionError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ReferenceError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "ReferenceError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_RuntimeError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "RuntimeError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_StopAsyncIteration)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "StopAsyncIteration");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_StopIteration)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "StopIteration");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_SyntaxError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "SyntaxError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_SystemError)) {
        hvml_err = PURC_ERROR_SYS_FAULT;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_SystemExit)) {
        hvml_err = PURC_ERROR_SYS_FAULT;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_TabError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "TabError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_TimeoutError)) {
        hvml_err = PURC_ERROR_TIMEOUT;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_TypeError)) {
        hvml_err = PURC_ERROR_WRONG_DATA_TYPE;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnboundLocalError)) {
        hvml_err = PURC_ERROR_INTERNAL_FAILURE;
        set_python_except(root, "UnboundLocalError");
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeDecodeError)) {
        hvml_err = PURC_ERROR_BAD_ENCODING;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeEncodeError)) {
        hvml_err = PURC_ERROR_BAD_ENCODING;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeError)) {
        hvml_err = PURC_ERROR_BAD_ENCODING;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_UnicodeTranslateError)) {
        hvml_err = PURC_ERROR_BAD_ENCODING;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ValueError)) {
        hvml_err = PURC_ERROR_INVALID_VALUE;
    }
    else if (PyErr_GivenExceptionMatches(pyerr, PyExc_ZeroDivisionError)) {
        hvml_err = PURC_ERROR_DIVBYZERO;
    }

    if (hvml_err != PURC_ERROR_OK) {
        purc_set_error(hvml_err);
        PyErr_Clear();
    }
}

enum {
    RUN_OPT_SKIP_FIRST_LINE         = 0x0001,
    RUN_OPT_DONT_WRITE_BYTE_CODE    = 0x0002,
    RUN_OPT_RETURN_STDOUT           = 0x0004,
    RUN_OPT_SET_ARGV0               = 0x0008,
};

#if 0
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
#endif

static purc_variant_t run_command(purc_variant_t root,
        const char *cmd, size_t len, PyCompilerFlags *cf, unsigned options)
{
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

    PyObject *m, *d, *result;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        goto failed;

    d = PyModule_GetDict(m);
    result = PyRun_StringFlags(cmd, Py_eval_input, d, d, cf);
    if (result == NULL) {
        handle_pyerr(root);
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
    handle_pyerr(root);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t run_file(purc_variant_t root,
        const char *fname, size_t len, PyCompilerFlags *cf, unsigned options)
{
    UNUSED_PARAM(len);

    FILE *fp = fopen(fname, "rb");
    if (fp == NULL) {
        purc_set_error(PURC_ERROR_IO_FAILURE);
        return PURC_VARIANT_INVALID;
    }

    PyObject *m, *d, *result;
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

    d = PyModule_GetDict(m);
    result = PyRun_FileFlags(fp, fname, Py_eval_input, d, d, cf);
    if (result == NULL) {
        goto failed;
    }

    fclose(fp);
    purc_variant_t ret = make_variant_from_pyobj(result);
    Py_DECREF(result);
    return ret;

failed:
    fclose(fp);
    handle_pyerr(root);
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
    size_t len;
    const char *cmd_mod_file = purc_variant_get_string_const_ex(argv[0], &len);
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

#if 0
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

    pcutils_map_entry *entry = pcutils_map_find(pyinfo->prop_map, property_name);
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
#endif

static purc_variant_t import_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    return purc_variant_make_undefined();
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

    assert(Py_IsInitialized());

    Py_Finalize();

    pcutils_map_destroy(pyinfo->prop_map);
    free(pyinfo);
}

static purc_variant_t create_py(void)
{
    static struct purc_dvobj_method methods[] = {
        { PY_KEY_RUN,           run_getter,      NULL },
        { PY_KEY_IMPORT,        import_getter,   NULL },
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

    purc_variant_t py;
    py = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));

    purc_variant_t val = PURC_VARIANT_INVALID;
    struct dvobj_pyinfo *pyinfo = NULL;
    if (py) {
        pyinfo = calloc(1, sizeof(*pyinfo));
        if (pyinfo == NULL)
            goto failed_info;
        pyinfo->prop_map = pcutils_map_create(
                copy_key_string, free_key_string,
                NULL, NULL, comp_key_string, true);
        if (pyinfo->prop_map == NULL)
            goto failed_info;

        /* placeholders for built-in properties of $PY */
        pcutils_map_insert(pyinfo->prop_map, PY_KEY_IMPL, NULL);
        pcutils_map_insert(pyinfo->prop_map, PY_KEY_INFO, NULL);
        pcutils_map_insert(pyinfo->prop_map, PY_KEY_EXCEPT, NULL);
        pcutils_map_insert(pyinfo->prop_map, PY_KEY_RUN, NULL);
        pcutils_map_insert(pyinfo->prop_map, PY_KEY_IMPORT, NULL);

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

        if ((val = purc_variant_make_native((void *)pyinfo,
                        &pyinfo_ops)) == NULL)
            goto fatal;
        if (!purc_variant_object_set_by_static_ckey(py,
                    PY_HANDLE_NAME, val))
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
        if (pyinfo->prop_map) {
            pcutils_map_destroy(pyinfo->prop_map);
        }
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

purc_variant_t __purcex_load_dynamic_variant(const char *name, int *ver_code)
{
    size_t i = 0;
    for (i = 0; i < PCA_TABLESIZE(dvobjs); i++) {
        if (strcasecmp(name, dvobjs[i].name) == 0)
            break;
    }

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

