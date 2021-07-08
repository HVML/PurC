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
#include "purc-variant.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

// for release the resource in a variant
typedef void (* pcvariant_release_fn)(purc_variant_t value);

// for release the resource in a variant
void pcvariant_string_release  (struct purc_variant_t value)    WTF_INTERNAL;
void pcvariant_sequence_release(struct purc_variant_t value)    WTF_INTERNAL;
void pcvariant_dynamic_release (struct purc_variant_t value)    WTF_INTERNAL;
void pcvariant_native_release  (struct purc_variant_t value)    WTF_INTERNAL;
void pcvariant_object_release  (struct purc_variant_t value)    WTF_INTERNAL;
void pcvariant_array_release   (struct purc_variant_t value)    WTF_INTERNAL;
void pcvariant_set_release     (struct purc_variant_t value)    WTF_INTERNAL;

// for custom serialization function.
typedef int (* pcvariant_to_json_string_fn)(struct purc_variant_t * value, struct purc_printbuf *pb, int level, int flags);

// for serialize a variant
int pcvariant_undefined_to_json_string  (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_null_to_json_string       (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_boolean_to_json_string    (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_number_to_json_string     (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_longint_to_json_string    (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_longdouble_to_json_string (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_string_to_json_string     (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_sequence_to_json_string   (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_dynamic_to_json_string    (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_native_to_json_string     (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_object_to_json_string     (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_array_to_json_string      (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;
int pcvariant_set_to_json_string        (struct purc_variant_t value, struct purc_printbuf *pb, int level, int flags)   WTF_INTERNAL;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _VARIANT_PUBLIC_H_ */
