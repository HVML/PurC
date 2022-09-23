/*
 * @file make_simple_array.c
 * @author Vincent Wei
 * @date 2021/09/16
 * @brief A sample demonstrating how to make a simple array and
 *      manage the anonymous members correctly.
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

#include "purc/purc.h"
#include "purc/purc-variant.h"

#undef NDEBUG
#include <assert.h>
#include <stdlib.h>

/* XXX: overflow uint64 since fibonacci[93] */
#define NR_MEMBERS      93

#define APPEND_ANONY_VAR(array, v)                                      \
    do {                                                                \
        if (v == PURC_VARIANT_INVALID ||                                \
                !purc_variant_array_append(array, v))                   \
            goto error;                                                 \
        purc_variant_unref(v);                                          \
    } while (0)

static purc_variant_t make_fibonacci_array(void)
{
    purc_variant_t fibonacci;

    fibonacci = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (fibonacci != PURC_VARIANT_INVALID) {
        int i;
        uint64_t a1 = 1, a2 = 1, a3;
        purc_variant_t v;

        v = purc_variant_make_ulongint(a1);
        APPEND_ANONY_VAR(fibonacci, v);

        v = purc_variant_make_ulongint(a2);
        APPEND_ANONY_VAR(fibonacci, v);

        for (i = 2; i < NR_MEMBERS; i++) {
            a3 = a1 + a2;
            v = purc_variant_make_ulongint(a3);
            APPEND_ANONY_VAR(fibonacci, v);

            a1 = a2;
            a2 = a3;
        }
    }

    return fibonacci;

error:
    purc_variant_unref(fibonacci);
    return PURC_VARIANT_INVALID;
}

static void quit_on_error(int errcode)
{
    fprintf(stderr, "Failed: %d\n", errcode);
    exit (errcode);
}

int main(void)
{
    purc_instance_extra_info info = {};
    purc_init_ex(PURC_MODULE_VARIANT,
            "cn.fmsoft.hybridos.sample", "make_dynamic_object", &info);

    purc_variant_t fibonacci = make_fibonacci_array();
    if (fibonacci == PURC_VARIANT_INVALID)
        quit_on_error(1);

    for (ssize_t i = 0; i < purc_variant_array_get_size(fibonacci); i++) {
        purc_variant_t v;
        uint64_t u;

        v = purc_variant_array_get(fibonacci, i);
        if (purc_variant_cast_to_ulongint(v, &u, false)) {
            printf ("fibonacci[%02u]: %llu\n", (unsigned)i,
                    (unsigned long long)u);
        }
        else {
            quit_on_error(i + 100);
        }
    }

    purc_variant_unref(fibonacci);

    const struct purc_variant_stat *stat = NULL;
    stat = purc_variant_usage_stat();

    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_ARRAY]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_OBJECT]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_DYNAMIC]);
    assert(0 == stat->nr_values[PURC_VARIANT_TYPE_STRING]);

    purc_cleanup ();

    return 0;
}

