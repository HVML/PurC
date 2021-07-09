/**
 * @file variant-set.c
 * @author Xu Xiaohong (freemine)
 * @date 2021/07/09
 * @brief The API for variant.
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


#include "config.h"
#include "private/variant.h"
#include "private/hashtable.h"
#include "private/errors.h"
#include "purc-errors.h"
#include "variant-internals.h"


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/*
 * set holds purc_variant_t values uniquely by keys which is separated by space
 * once a purc_variant_t value is added into set,
 * no matter via `purc_variant_make_set` or `purc_variant_set_add/...`,
 * this value's ref + 1
 * once a purc_variant_t value in set get removed,
 * no matter via `pcvariant_set_release` or `purc_variant_set_remove`
 * or even implicitly being overwritten by `purc_variant_set_add/...`,
 * this value's ref - 1
 * note: value with unique-key can be added into set for no more than 1 time
 *
 * thinking: shall we recursively check if there's ref-loop among set and it's
 *           children element?
 */

#if 0
purc_variant_t purc_variant_make_set_c (size_t sz, const char* unique_key, purc_variant_t value0, ...);

purc_variant_t purc_variant_make_set (size_t sz, purc_variant_t unique_key, purc_variant_t value0, ...);
#endif // 0

void pcvariant_set_release (purc_variant_t value)
{
    // todo
    UNUSED_PARAM(value);
}

int pcvariant_set_compare (purc_variant_t lv, purc_variant_t rv)
{
    // todo
    UNUSED_PARAM(lv);
    UNUSED_PARAM(rv);
    return -1;
}

#if 0
bool purc_variant_set_add (purc_variant_t obj, purc_variant_t value);

bool purc_variant_set_remove (purc_variant_t obj, purc_variant_t value);

purc_variant_t purc_variant_set_get_value_c (const purc_variant_t set, const char * match_key);

size_t purc_variant_set_get_size(const purc_variant_t set);

struct purc_variant_set_iterator;

struct purc_variant_set_iterator* purc_variant_set_make_iterator_begin (purc_variant_t set);

struct purc_variant_set_iterator* purc_variant_set_make_iterator_end (purc_variant_t set);

void purc_variant_set_release_iterator (struct purc_variant_set_iterator* it);

bool purc_variant_set_iterator_next (struct purc_variant_set_iterator* it);

bool purc_variant_set_iterator_prev (struct purc_variant_set_iterator* it);

purc_variant_t purc_variant_set_iterator_get_value (struct purc_variant_set_iterator* it);

#endif //0

