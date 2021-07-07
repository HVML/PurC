/*
 * @file json_object.h
 * @author
 * @date 2021/07/07
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

/*
 * $Id: json_object.h,v 1.12 2006/01/30 23:07:57 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

/**
 * @file
 * @brief Core json-c API.  Start here, or with json_tokener.h
 */
#ifndef _pcutils_json_object_h_
#define _pcutils_json_object_h_

#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#define PCUTILS_JSON_OBJECT_DEF_HASH_ENTRIES 16

/**
 * A flag for the json_object_to_json_string_ext() and
 * json_object_to_file_ext() functions which causes the output
 * to have no extra whitespace or formatting applied.
 */
#define PCUTILS_JSON_C_TO_STRING_PLAIN 0
/**
 * A flag for the json_object_to_json_string_ext() and
 * json_object_to_file_ext() functions which causes the output to have
 * minimal whitespace inserted to make things slightly more readable.
 */
#define PCUTILS_JSON_C_TO_STRING_SPACED (1 << 0)
/**
 * A flag for the json_object_to_json_string_ext() and
 * json_object_to_file_ext() functions which causes
 * the output to be formatted.
 *
 * See the "Two Space Tab" option at http://jsonformatter.curiousconcept.com/
 * for an example of the format.
 */
#define PCUTILS_JSON_C_TO_STRING_PRETTY (1 << 1)
/**
 * A flag for the json_object_to_json_string_ext() and
 * json_object_to_file_ext() functions which causes
 * the output to be formatted.
 *
 * Instead of a "Two Space Tab" this gives a single tab character.
 */
#define PCUTILS_JSON_C_TO_STRING_PRETTY_TAB (1 << 3)
/**
 * A flag to drop trailing zero for float values
 */
#define PCUTILS_JSON_C_TO_STRING_NOZERO (1 << 2)

/**
 * Don't escape forward slashes.
 */
#define PCUTILS_JSON_C_TO_STRING_NOSLASHESCAPE (1 << 4)

/**
 * A flag for the json_object_object_add_ex function which
 * causes the value to be added without a check if it already exists.
 * Note: it is the responsibility of the caller to ensure that no
 * key is added multiple times. If this is done, results are
 * unpredictable. While this option is somewhat dangerous, it
 * permits potentially large performance savings in code that
 * knows for sure the key values are unique (e.g. because the
 * code adds a well-known set of constant key values).
 */
#define PCUTILS_JSON_C_OBJECT_ADD_KEY_IS_NEW (1 << 1)
/**
 * A flag for the json_object_object_add_ex function which
 * flags the key as being constant memory. This means that
 * the key will NOT be copied via strdup(), resulting in a
 * potentially huge performance win (malloc, strdup and
 * free are usually performance hogs). It is acceptable to
 * use this flag for keys in non-constant memory blocks if
 * the caller ensure that the memory holding the key lives
 * longer than the corresponding json object. However, this
 * is somewhat dangerous and should only be done if really
 * justified.
 * The general use-case for this flag is cases where the
 * key is given as a real constant value in the function
 * call, e.g. as in
 *   json_object_object_add_ex(obj, "ip", json,
 *       PCUTILS_JSON_C_OBJECT_KEY_IS_CONSTANT);
 */
#define PCUTILS_JSON_C_OBJECT_KEY_IS_CONSTANT (1 << 2)

/**
 * Set the global value of an option, which will apply to all
 * current and future threads that have not set a thread-local value.
 *
 * @see json_c_set_serialization_double_format
 */
#define PCUTILS_JSON_C_OPTION_GLOBAL (0)
/**
 * Set a thread-local value of an option, overriding the global value.
 * This will fail if json-c is not compiled with threading enabled, and
 * with the __thread specifier (or equivalent) available.
 *
 * @see json_c_set_serialization_double_format
 */
#define PCUTILS_JSON_C_OPTION_THREAD (1)

#ifdef __clang__
/*
 * Clang doesn't pay attention to the parameters defined in the
 * function typedefs used here, so turn off spurious doc warnings.
 * {
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#endif

#ifdef __clang__
/* } */
#pragma clang diagnostic pop
#endif

#ifdef __cplusplus
}
#endif

#endif
