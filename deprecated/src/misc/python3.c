
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

    pcutils_map_entry *entry = pcutils_map_find(pyinfo->reserved_symbols,
            property_name);
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

#if 0
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

#if 0
static struct purc_native_ops native_pydict_ops = {
    .getter = pydict_getter,
    .setter = pydict_setter,
    .property_getter = pydict_property_getter_getter,
    .property_setter = pydict_property_setter_getter,
    .on_release = NULL, // on_release_pyobject,
};

static struct purc_native_ops native_pycode_locals_ops = {
    .property_getter = pydict_property_getter_getter,
    .property_setter = pydict_property_setter_getter,
    .on_release = on_release_pyobject,
};
#endif

