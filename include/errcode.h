/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML parser
** and interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
*/

#ifndef PURC_ERRCODE_H
#define PURC_ERRCODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int purc_error_t;

#define PURC_ERROR_OK                   0
#define PURC_ERROR_BAD_SYSTEM_CALL      1
#define PURC_ERROR_OUT_OF_MEMORY        2
#define PURC_ERROR_INVALID_VALUE        3
#define PURC_ERROR_NOT_IMPLEMENTED      4

// the first error codes for various modules:
#define PURC_ERROR_FIRST_VARIANT        100
#define PURC_ERROR_FIRST_RWSTREAM       200

#define PURC_ERROR_FIRST_EJSON          1100
#define PURC_ERROR_FIRST_HVML           1200
#define PURC_ERROR_FIRST_HTML           1300
#define PURC_ERROR_FIRST_XGML           1400
#define PURC_ERROR_FIRST_XML            1500

#define PURC_ERROR_FIRST_VDOM           2100
#define PURC_ERROR_FIRST_EDOM           2200
#define PURC_ERROR_FIRST_VCM            2300


/**
 * Set last error code
 *
 * @param err_code:  error code
 *
 * Since: 0.0.1
 */
void purc_set_last_error (purc_error_t err_code);

/**
 * Get last error code
 *
 * Returns: last error code
 *
 * Since: 0.0.1
 */
purc_error_t purc_get_last_error (void);

/**
 * Get message of the error code
 *
 * @param err_code:  error code
 *
 * Returns:  error code message
 *
 * Since: 0.0.1
 */
const char* purc_get_error_message (purc_error_t err_code);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PURC_ERRCODE_H */

