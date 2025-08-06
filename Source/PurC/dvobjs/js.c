/*
 * @file js.c
 * @author Vincent Wei
 * @date 2025/08/06
 * @brief The implementation of JS dynamic variant object.
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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

#if ENABLE(QUICKJS)

#include "private/instance.h"
#include "private/errors.h"
#include "private/atom-buckets.h"
#include "private/dvobjs.h"

#include "purc-variant.h"
#include "purc-dvobjs.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#include "quickjs/quickjs.h"
#include "quickjs/quickjs-libc.h"
#pragma GCC diagnostic pop

#include <errno.h>

#define JS_KEY_ARGS         "args"
#define JS_KEY_LOAD         "load"
#define JS_KEY_EVAL         "eval"
#define JS_KEY_CONTEXT      "__js_context"

struct dvobj_jsinfo {
    purc_variant_t          root;       // the root variant, i.e., $JS itself
    JSRuntime              *rt;         // the JavaScript runtime
    JSContext              *ctx;        // the JavaScript context
    struct pcvar_listener  *listener;   // the listener
};

static inline struct dvobj_jsinfo *get_jsinfo_from_root(purc_variant_t root)
{
    purc_variant_t v;

    v = purc_variant_object_get_by_ckey(root, JS_KEY_CONTEXT);
    assert(v && purc_variant_is_native(v));

    return (struct dvobj_jsinfo *)purc_variant_native_get_entity(v);
}

static purc_variant_t args_setter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    int ec = PURC_ERROR_OK;
    struct dvobj_jsinfo *jsinfo = get_jsinfo_from_root(root);

    if (jsinfo == NULL)
        goto error;

    if (nr_args == 0) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    size_t argc;
    if (!purc_variant_linear_container_size(argv[0], &argc)) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto error;
    }

    if (argc > 0) {
        JSValue global_obj, args;
        global_obj = JS_GetGlobalObject(jsinfo->ctx);
        args = JS_NewArray(jsinfo->ctx);

        for (size_t i = 0; i < argc; i++) {
            purc_variant_t v = purc_variant_linear_container_get(argv[0], i);
            const char *arg = purc_variant_get_string_const(v);
            JS_SetPropertyUint32(jsinfo->ctx, args, i,
                    JS_NewString(jsinfo->ctx, arg));
        }

        JS_SetPropertyStr(jsinfo->ctx, global_obj, "scriptArgs", args);
        JS_FreeValue(jsinfo->ctx, global_obj);
    }

    return purc_variant_make_boolean(true);

error:
    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static int eval_buf(JSContext *ctx, const void *buf, int buf_len,
                    const char *filename, int eval_flags)
{
    JSValue val;
    int ret;

    if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
        /* for the modules, we compile then run to be able to set
           import.meta */
        val = JS_Eval(ctx, buf, buf_len, filename,
                      eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(val)) {
            js_module_set_import_meta(ctx, val, TRUE, TRUE);
            val = JS_EvalFunction(ctx, val);
        }
        val = js_std_await(ctx, val);
    } else {
        val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
    }
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        ret = -1;
    } else {
        ret = 0;
    }
    JS_FreeValue(ctx, val);
    return ret;
}

static int has_suffix(const char *str, const char *suffix)
{
    size_t len = strlen(str);
    size_t slen = strlen(suffix);
    return (len >= slen && !memcmp(str + len - slen, suffix, slen));
}

static int eval_file(JSContext *ctx, const char *filename, int module)
{
    uint8_t *buf;
    int ret, eval_flags;
    size_t buf_len;

    if (strcmp(filename, "std") == 0) {
        const char *str = "import * as std from 'std';\n"
            "import * as os from 'os';\n"
            "globalThis.std = std;\n"
            "globalThis.os = os;\n";
        return eval_buf(ctx, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);
    }

    buf = js_load_file(ctx, &buf_len, filename);
    if (!buf) {
        PC_ERROR("Failed loading file (%s): %s.\n", filename, strerror(errno));
        return -1;
    }

    if (module < 0) {
        module = (has_suffix(filename, ".mjs") ||
                  JS_DetectModule((const char *)buf, buf_len));
    }

    if (module)
        eval_flags = JS_EVAL_TYPE_MODULE;
    else
        eval_flags = JS_EVAL_TYPE_GLOBAL;
    ret = eval_buf(ctx, buf, buf_len, filename, eval_flags);
    js_free(ctx, buf);
    return ret;
}

static struct pcdvobjs_option_to_atom load_type_skws[] = {
    { "autodetect", 0, -1 },
    { "script",     0, 0 },
    { "module",     0, 1 },
};

static purc_variant_t load_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    int ec = PURC_ERROR_OK;
    struct dvobj_jsinfo *jsinfo = get_jsinfo_from_root(root);

    if (jsinfo == NULL)
        goto error;

    if (nr_args == 0) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto error;
    }

    if (load_type_skws[0].atom == 0) {
        for (size_t j = 0; j < PCA_TABLESIZE(load_type_skws); j++) {
            load_type_skws[j].atom = purc_atom_from_static_string_ex(
                    ATOM_BUCKET_DVOBJ, load_type_skws[j].option);
        }
    }

    int module = pcdvobjs_parse_options(
            (nr_args > 1) ? argv[1] : PURC_VARIANT_INVALID,
            load_type_skws, PCA_TABLESIZE(load_type_skws),
            NULL, 0, -1, -2);
    if (module == -2) {
        /* error will be set by pcdvobjs_parse_options() */
        goto error;
    }

    size_t nr_files;
    if (purc_variant_linear_container_size(argv[0], &nr_files)) {
        for (size_t i = 0; i < nr_files; i++) {
            purc_variant_t v = purc_variant_linear_container_get(argv[0], i);
            const char *filename = purc_variant_get_string_const(v);
            if (filename == NULL) {
                ec = PURC_ERROR_INVALID_VALUE;
                goto error;
            }

            if (eval_file(jsinfo->ctx, filename, module))
                goto js_error;
        }
    }
    else {
        const char *filename = purc_variant_get_string_const(argv[0]);
        if (filename == NULL) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto error;
        }

        if (eval_file(jsinfo->ctx, filename, module))
            goto js_error;
    }

js_error:

error:
    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t eval_getter(purc_variant_t root,
            size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    struct dvobj_jsinfo *jsinfo = get_jsinfo_from_root(root);

    if (jsinfo == NULL)
        goto failed;

    if (nr_args == 0 || argv[0] == NULL) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static bool on_js_being_released(purc_variant_t src, pcvar_op_t op,
        void *ctxt, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    PC_DEBUG("%s: %p\n", __func__, ctxt);

    if (op == PCVAR_OPERATION_RELEASING) {
        struct dvobj_jsinfo *jsinfo = ctxt;

        JS_FreeContext(jsinfo->ctx);
        purc_variant_revoke_listener(src, jsinfo->listener);
        free(jsinfo);
    }

    return true;
}

purc_variant_t
purc_dvobj_js_new(pcintr_coroutine_t cor)
{
    purc_variant_t js = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    struct dvobj_jsinfo *jsinfo = NULL;

    if (!cor->owner->owner->js_rt) {
        PC_ERROR("JavaScript runtime not initialized.\n");
        goto failed;
    }

    static struct purc_dvobj_method methods[] = {
        { JS_KEY_ARGS,      NULL,           args_setter },
        { JS_KEY_LOAD,      load_getter,    NULL },
        { JS_KEY_EVAL,      eval_getter,    NULL },
    };

    js = purc_dvobj_make_from_methods(methods, PCA_TABLESIZE(methods));
    if (js) {
        jsinfo = calloc(1, sizeof(*jsinfo));
        if (jsinfo == NULL)
            goto failed;

        jsinfo->root = js;
        jsinfo->rt = cor->owner->owner->js_rt;
        jsinfo->ctx = JS_NewCustomContext(jsinfo->rt);

        if (!jsinfo->ctx) {
            PC_ERROR("Cannot allocate JS context\n");
            goto failed;
        }

        js_std_add_helpers(jsinfo->ctx, -1, NULL);

        if ((val = purc_variant_make_native((void *)jsinfo, NULL)) == NULL)
            goto failed;

        if (!purc_variant_object_set_by_static_ckey(js, JS_KEY_CONTEXT, val))
            goto failed;
        purc_variant_unref(val);
        val = PURC_VARIANT_INVALID;

        jsinfo->listener = purc_variant_register_post_listener(js,
                PCVAR_OPERATION_RELEASING, on_js_being_released, jsinfo);
        if (jsinfo->listener == NULL)
            goto failed;

        return js;
    }

failed:
    if (jsinfo) {
        if (jsinfo->ctx)
            JS_FreeContext(jsinfo->ctx);
        free(jsinfo);
    }

    if (val)
        purc_variant_unref(val);
    if (js)
        purc_variant_unref(js);

    pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
    return PURC_VARIANT_INVALID;
}

#else   /* ENABLE(QUICKJS) */

purc_variant_t
purc_dvobj_js_new(pcintr_coroutine_t cor)
{
    (void)cor;
    return PURC_VARIANT_INVALID;
}

#endif  /* !ENABLE(QUICKJS) */

