/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of Purring Cat 2, a HVML parser and interpreter.
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

#ifndef PURC_MYCORE_RWSTREAM_H
#define PURC_MYCORE_RWSTREAM_H

#pragma once

struct _PURC_RWSTREAM;
typedef struct _PURC_RWSTREAM PURC_RWSTREAM;

/**
 * Creates a new PURC_RWSTREAM for the given file and mode.
 *
 * @param file: the file will be opened
 * @param mode: One of "r", "w", "a", "r+", "w+", "a+". These have the same meaning as in fopen()
 *
 * Returns: A PURC_RWSTREAM on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PURC_RWSTREAM* purc_rwstream_from_file (const char* file, const char* mode);

/**
 * Creates a new PURC_RWSTREAM for the given FILE pointer.
 *
 * @param fp: FILE pointer
 * @param autoclose: Whether to automatically close the fp when the PURC_RWSTREAM is freed.
 *
 * Returns: A PURC_RWSTREAM on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PURC_RWSTREAM* purc_rwstream_from_fp (FILE* fp, bool autoclose);

/**
 * Creates a new PURC_RWSTREAM for the given file descriptor.
 *
 * @param fd: file descriptor
 * @param autoclose: Whether to automatically close the fp when the PURC_RWSTREAM is freed.
 *
 * Returns: A PURC_RWSTREAM on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PURC_RWSTREAM* purc_rwstream_from_fd (int fd, bool autoclose);

/**
 * Creates a new PURC_RWSTREAM for the given memory buffer.
 *
 * @param mem: pointer to memory buffer
 * @param sz:  size of memory buffer
 *
 * Returns: A PURC_RWSTREAM on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PURC_RWSTREAM* purc_rwstream_from_mem (void* mem, size_t sz);

/**
 * Release the PURC_RWSTREAM
 *
 * @param rws: pointer to PURC_RWSTREAM
 *
 * Returns: 0 success, non-zero otherwise.
 *
 * Since: 0.0.1
 */
int purc_rwstream_free (PURC_RWSTREAM* rws);



#endif /* PURC_MYCORE_RWSTREAM_H */

