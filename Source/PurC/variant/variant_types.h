/**
 * @file variant-type.h
 * @author 
 * @date 2021/07/02
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



#ifndef _json_object_private_h_
#define _json_object_private_h_

#include "purc_variant.h"

PCA_EXTERN_C_BEGIN

struct json_object;
#include "json_inttypes.h"
#include "json_types.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

/* json object int type, support extension*/
typedef enum json_object_int_type
{
	json_object_int_type_int64,
	json_object_int_type_uint64
} json_object_int_type;

struct json_object
{
	enum json_type o_type;
	uint32_t _ref_count;
	json_object_to_json_string_fn *_to_json_string;
	struct printbuf *_pb;
	json_object_delete_fn *_user_delete;
	void *_userdata;
	// Actually longer, always malloc'd as some more-specific type.
	// The rest of a struct json_object_${o_type} follows
};

struct json_object_object
{
	struct json_object base;
	struct lh_table *c_object;
};
struct json_object_array
{
	struct json_object base;
	struct array_list *c_array;
};

struct json_object_boolean
{
	struct json_object base;
	json_bool c_boolean;
};
struct json_object_double
{
	struct json_object base;
	double c_double;
};
struct json_object_int
{
	struct json_object base;
	enum json_object_int_type cint_type;
	union
	{
		int64_t c_int64;
		uint64_t c_uint64;
	} cint;
};
struct json_object_string
{
	struct json_object base;
	ssize_t len; // Signed b/c negative lengths indicate data is a pointer
	// Consider adding an "alloc" field here, if json_object_set_string calls
	// to expand the length of a string are common operations to perform.
	union
	{
		char idata[1]; // Immediate data.  Actually longer
		char *pdata;   // Only when len < 0
	} c_string;
};

void _json_c_set_last_err(const char *err_fmt, ...);

extern const char *json_hex_chars;



#define MAX(a, b)   (a) > (b)? (a): (b);

#define PCVARIANT_FLAG_NOREF     0x0001
#define PCVARIANT_FLAG_NOFREE    0x0002


struct purc_variant {

    /* variant type */
    enum variant_type type:8;

    /* real length for short string and byte sequence */
    unsigned int size:8;        

    /* flags */
    unsigned int flags:16;

    /* reference count */
    unsigned int refc;

    /* value */
    union {
        /* for boolean */
        bool        b;

        /* for number */
        double      d;

        /* for long integer */
        int64_t     i64;

        /* for unsigned long integer */
        uint64_t    u64;

        /* for long double */
        long double ld;

        /* for dynamic and native variant (two pointers) */
        void*       ptr2[2];

        /* for long string, long byte sequence, array, and object (one for size and the other for pointer). */
        uintptr_t   sz_ptr[2];

        /* for short string and byte sequence; the real space size of `bytes` is `max(sizeof(long double), sizeof(void*) * 2)` */
        uint8_t     bytes[0];
    };
};


PCA_EXTERN_C_END

#endif
