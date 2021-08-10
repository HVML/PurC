/**
 * @file purc-dynamic.h
 * @author 
 * @date 2021/08/08
 * @brief The API for system dynamic variant.
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

#ifndef _VARIANT_DYNAMIC_H_ 
#define _VARIANT_DYNAMIC_H_ 

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pruce-variant.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
Usage:

HVML statement: $_SYSTEM.uname('kernel-name kernel-release kernel-version')

After HVML Parser:

purc_variant_t root: ?
unsinged int nr_args: 4
purc_variant_t* argv: string variant array, with size 4: 
                                [0]: "kernel-name",
                                [1]: "kernel-release",
                                [2]: "kernel-version",
                                [3]: NULL

purc_variant_t uname = purc_variant_object_get_c (sys, "uname");
purc_dvariant_method fn = uname.getter();
purc_variant_t result = fn (root? , 3, get_array);

purc_variant_t name = purc_variant_object_get_c (result, "kernel-name");
purc_variant_t release = purc_variant_object_get_c (result, "kernel-release");
purc_variant_t version = purc_variant_object_get_c (result, "kernel-version");
*/


// For SYSTEM
/**
 * Get the system information accroding to argv.
 *
 * @param root: 
 * @param inr_args: the element number in argv, the last NULL is includsive
 * @param argv: the string variant array, the name of information
 *
 * Returns: An ojbect purc_variant_t with "name:info".
 *
 * Since: 0.0.1
 */
purc_variant_t
get_uname (purc_variant_t root, unsinged int nr_args, purc_variant_t* argv);


/**
 * Get the locale information accroding to argv.
 *
 * @param root: 
 * @param inr_args: the element number in argv, the last NULL is includsive
 * @param argv: the string variant array, the locale type 
 * Returns: An ojbect purc_variant_t with "locale type:value".
 *
 * Since: 0.0.1
 */
purc_variant_t
get_locale (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


/**
 * Set the locale information.
 *
 * @param root: 
 * @param inr_args: the element number in argv, the last NULL is includsive
 * @param argv: the object variant array, the locale type: value
 *
 * Returns: An boolean variant, true if success, otherwise false.
 *
 * Since: 0.0.1
 */
purc_variant_t
set_locale (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


/**
 * Get the random 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 3 
 * @param argv: number variant, minimize number
                number variant, maxmize number
                NULL
 * Returns: An double variant of random.
 *
 * Since: 0.0.1
 */
purc_variant_t
get_random (purc_variant_t root, size_t nr_args, purc_variant_t* argv);

/**
 * Get the system time 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 4 
 * @param argv: string variant, ISO standard name
 *              number variant, secons since epoch
 *              string variant, locale
 *              NULL
 * Returns: An double variant of random.
 *
 * Since: 0.0.1
 */
purc_variant_t
get_time (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


// For MATH
/**
 * Get the number of PI 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 2 
 * @param argv: number variant, the digits after point 
 *              NULL
 * Returns: An double variant of PI.
 *
 * Since: 0.0.1
 */
purc_variant_t
get_pi (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


/**
 * Get the result of an statement 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 2 
 * @param argv: string variant, the statement 
 *              NULL
 * Returns: An double variant of statement value.
 *
 * Since: 0.0.1
 */
purc_variant_t
get_eval (purc_variant_t root, size_t nr_args, purc_variant_t* argv);

/**
 * Get the math function, for example: sin 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 3 
 * @param argv: number variant, the degree
 *              string variant, unit
 * Returns: An double variant of sin value.
 *
 * Since: 0.0.1
 */
purc_variant_t
get_sin (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


// For Filesystem
/**
 * Get the file list in a directory 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 4
 * @param argv: string variant, the directory
 *              string variant, the filter
                boolean variant, whether recursive, ???
                NULL
 * Returns: An array variant of list.
                [
                    {
                        path:xxxxxx,
                        attribute:xxxxx
                    },
                    .....,
                    undefined.
                ]
 *
 * Since: 0.0.1
 */
purc_variant_t
get_list (purc_variant_t root, size_t nr_args, purc_variant_t* argv);

/**
 * make a new directory 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 3 
 * @param argv: string variant, the absolute path of new directory
 *              integer variant for the mode, accroding to c lib
 *              NULL
 * Returns: An boolean variant, true for success, otherwiase false.
 *          You can get error number with purc_get_last_error ()
 *
 * Since: 0.0.1
 */
purc_variant_t
mkdir (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


/**
 * remove a directory or a file 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 3 
 * @param argv: string variant, the absolute path of new directory
                boolean variant, whether recursive
 *              NULL
 * Returns: An boolean variant, true for success, otherwiase false.
 *          You can get error number with purc_get_last_error ()
 *
 * Since: 0.0.1
 */
purc_variant_t
rmdir (purc_variant_t root, size_t nr_args, purc_variant_t* argv);

/**
 * update the timestamp of a file if the file exists, or create new file 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 2 
 * @param argv: string variant, the absolute path of new directory
 *              NULL
 * Returns: An boolean variant, true for success, otherwiase false.
 *          You can get error number with purc_get_last_error ()
 *
 * Since: 0.0.1
 */
purc_variant_t
touch (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


/**
 * remove the file 
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 2 
 * @param argv: string variant, the absolute path of new directory
 *              NULL
 * Returns: An boolean variant, true for success, otherwiase false.
 *          You can get error number with purc_get_last_error ()
 *
 * Since: 0.0.1
 */
purc_variant_t
unlink (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


// For File
/**
 * Get the text file string for head
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 4
 * @param argv: string variant, absolute path of file 
 *              integer variant, line number
                NULL
 * Returns: An string variant, content the text of first lines.
 *          You can get error number with purc_get_last_error ()
 *
 * Since: 0.0.1
 */
purc_variant_t
file_text_head (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


/**
 * Get the text file string from tail
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 4
 * @param argv: string variant, absolute path of file 
 *              integer variant, line number
                NULL
 * Returns: An string variant, content the text of last lines.
 *          You can get error number with purc_get_last_error ()
 *
 * Since: 0.0.1
 */
purc_variant_t
file_text_tail (purc_variant_t root, size_t nr_args, purc_variant_t* argv);


/**
 * Get the first bytes from binary file
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 4
 * @param argv: string variant, absolute path of file 
 *              integer variant, bytes number
                NULL
 * Returns: An sequency variant, content the first bytes.
 *          You can get error number with purc_get_last_error ()
 *
 * Since: 0.0.1
 */
purc_variant_t
file_bin_head (purc_variant_t root, size_t nr_args, purc_variant_t* argv);

/**
 * Get the last bytes from binary file
 *
 * @param root: 
 * @param inr_args: the element number in argv, it must be 4
 * @param argv: string variant, absolute path of file 
 *              integer variant, bytes number
                NULL
 * Returns: An sequency variant, content the last bytes.
 *          You can get error number with purc_get_last_error ()
 *
 * Since: 0.0.1
 */
purc_variant_t
file_bin_tail (purc_variant_t root, size_t nr_args, purc_variant_t* argv);




#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif /* not defined _VARIANT_DYNAMIC_H_ */
