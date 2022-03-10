/*
 * @file session.c
 * @author Xue Shuming
 * @date 2022/01/04
 * @brief The implementation of Session dynamic variant object.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/vdom.h"
#include "private/dvobjs.h"
#include "private/url.h"
#include "purc-variant.h"
#include "helper.h"

#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DVOBJ_SESSION_DATA_NAME        "__handle_dvobj_session"

static purc_variant_t
cwd_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    if (root == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return purc_variant_make_string(cwd, false);
    }
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
cwd_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(silently);

    if ((root == PURC_VARIANT_INVALID) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    const char* dir = purc_variant_get_string_const (argv[0]);
    struct stat st;
    if (dir == NULL || stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        pcinst_set_error(PURC_ERROR_NOT_DESIRED_ENTITY);
        return PURC_VARIANT_INVALID;
    }

    if (chdir(dir) == 0) {
        return purc_variant_make_boolean(true);
    }

    pcinst_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
user_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(silently);

    if ((root == PURC_VARIANT_INVALID)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t user = purc_variant_object_get_by_ckey(root,
            DVOBJ_SESSION_DATA_NAME, false);

    if (nr_args < 1) {
        purc_variant_ref(user);
        return user;
    }

    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t var = purc_variant_object_get(user, argv[0], false);
    if (var != PURC_VARIANT_INVALID) {
        purc_variant_ref(var);
        return var;
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
user_setter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(silently);

    if ((root == PURC_VARIANT_INVALID) || (argv == NULL) || (nr_args < 2)) {
        pcinst_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_object (root)) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    if (!purc_variant_is_string (argv[0])) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t user = purc_variant_object_get_by_ckey(root,
            DVOBJ_SESSION_DATA_NAME, false);
    if (user == PURC_VARIANT_INVALID) {
        pcinst_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    bool ret = purc_variant_object_set(user, argv[0], argv[1]);
    return purc_variant_make_boolean(ret);
}


purc_variant_t
pcdvobjs_get_session(void)
{
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    static struct pcdvobjs_dvobjs method [] = {
        {"cwd",               cwd_getter,              cwd_setter},
        {"user",              user_getter,             user_setter},
    };

    ret_var = pcdvobjs_make_dvobjs(method, PCA_TABLESIZE(method));
    if (ret_var == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t user_data = purc_variant_make_object(0, PURC_VARIANT_INVALID,
                                                    PURC_VARIANT_INVALID);
    if (user_data == PURC_VARIANT_INVALID) {
        purc_variant_unref(ret_var);
        return PURC_VARIANT_INVALID;
    }
    purc_variant_object_set_by_static_ckey(ret_var, DVOBJ_SESSION_DATA_NAME,
            user_data);
    purc_variant_unref(user_data);

    return ret_var;
}
