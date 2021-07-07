/**
 * @file purc-utils.h
 * @author Vincent Wei
 * @date 2021/07/05
 * @brief The API for utilities.
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

#ifndef PURC_PURC_UTILS_H
#define PURC_PURC_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "purc-macros.h"

/**
 * SECTION:atom
 * @title: Atom String
 * @short_description: a 2-way association between a string and a
 *     unique integer identifier
 *
 * Atoms are associations between strings and integer identifiers.
 * Given either the string or the #purc_atom_t identifier it is possible to
 * retrieve the other.
 *
 * To create a new atom from a string, use purc_atom_from_string() or
 * purc_atom_from_static_string().
 *
 * To find the string corresponding to a given #purc_atom_t, use
 * g_atom_to_string().
 *
 * To find the #purc_atom_t corresponding to a given string, use
 * g_atom_try_string().
 */

/**
 * purc_atom_t:
 *
 * A purc_atom_t is a non-zero integer which uniquely identifies a
 * particular string. A purc_atom_t value of zero is associated to %NULL.
 */
typedef uintptr_t purc_atom_t;

PCA_EXTERN_C_BEGIN

/**
 * purc_atom_from_string:
 * @string: (nullable): a string
 *
 * Gets the #purc_atom_t identifying the given string. If the string does
 * not currently have an associated #purc_atom_t, a new #purc_atom_t is created,
 * using a copy of the string.
 *
 * This function must not be used before library constructors have finished
 * running. In particular, this means it cannot be used to initialize global
 * variables in C++.
 *
 * Returns: the #purc_atom_t identifying the string, or 0 if @string is %NULL
 */
PCA_EXPORT purc_atom_t
purc_atom_make_from_string(const char* string);

/**
 * purc_atom_from_static_string:
 * @string: (nullable): a string
 *
 * Gets the #purc_atom_t identifying the given (static) string. If the
 * string does not currently have an associated #purc_atom_t, a new #purc_atom_t
 * is created, linked to the given string.
 *
 * Note that this function is identical to purc_atom_from_string() except
 * that if a new #purc_atom_t is created the string itself is used rather
 * than a copy. This saves memory, but can only be used if the string
 * will continue to exist until the program terminates. It can be used
 * with statically allocated strings in the main program, but not with
 * statically allocated memory in dynamically loaded modules, if you
 * expect to ever unload the module again (e.g. do not use this
 * function in GTK+ theme engines).
 *
 * This function must not be used before library constructors have finished
 * running. In particular, this means it cannot be used to initialize global
 * variables in C++.
 *
 * Returns: the #purc_atom_t identifying the string, or 0 if @string is %NULL
 */
PCA_EXPORT purc_atom_t
purc_atom_make_from_static_string(const char* string);
 
/**
 * purc_atom_try_string:
 * @string: (nullable): a string
 *
 * Gets the #purc_atom_t associated with the given string, or 0 if string is
 * %NULL or it has no associated #purc_atom_t.
 *
 * If you want the purc_atom_t to be created if it doesn't already exist,
 * use purc_atom_from_string() or purc_atom_from_static_string().
 *
 * This function must not be used before library constructors have finished
 * running.
 *
 * Returns: the #purc_atom_t associated with the string, or 0 if @string is
 *     %NULL or there is no #purc_atom_t associated with it
 */
PCA_EXPORT purc_atom_t
purc_atom_try_string(const char* string);

/**
 * purc_atom_to_string:
 * @quark: a #purc_atom_t.
 *
 * Gets the string associated with the given #purc_atom_t.
 *
 * Returns: the string associated with the #purc_atom_t
 */
PCA_EXPORT const char*
purc_atom_to_string(purc_atom_t atom);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_UTILS_H */

