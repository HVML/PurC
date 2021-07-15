/**
 * @file variant-public.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for public.
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


#ifndef _VARIANT_PUBLIC_H_
#define _VARIANT_PUBLIC_H_

#include "config.h"
#include "private/debug.h"
#include "purc-variant.h"

#define PCVARIANT_CHECK_FAIL_RET(cond, ret)                     \
    if (!(cond)) {                                              \
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);             \
        return (ret);                                           \
    }

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * Set the extra size sz_ptr[0] of one variant, and update the statistics data.
 * This function should be called only for variant with
 * the flag PCVARIANT_FLAG_EXTRA_SIZE
 *
 * Note that the caller should not set the sz_ptr[0] directly.
 */
void pcvariant_stat_set_extra_size(purc_variant_t v, size_t sz) WTF_INTERNAL;

/* Allocate a variant for the specific type. */
purc_variant_t pcvariant_get (enum purc_variant_type type) WTF_INTERNAL;

/*
 * Release a unused variant.
 *
 * Note that the caller is responsible to release the extra memory
 * used by the variant.
 */
void pcvariant_put(purc_variant_t value) WTF_INTERNAL;

// for release the resource in a variant
typedef void (* pcvariant_release_fn) (purc_variant_t value);

// for release the resource in a variant
void pcvariant_string_release  (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_sequence_release(purc_variant_t value)    WTF_INTERNAL;
void pcvariant_native_release  (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_object_release  (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_array_release   (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_set_release     (purc_variant_t value)    WTF_INTERNAL;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _VARIANT_PUBLIC_H_ */
