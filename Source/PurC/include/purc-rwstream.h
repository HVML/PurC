/**
 * @file purc-rwstream.h
 * @author XueShuming
 * @date 2021/07/02
 * @brief The API for RWStream.
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

#ifndef PURC_PURC_RWSTREAM_H
#define PURC_PURC_RWSTREAM_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
#include <sys/types.h>

#include "purc-macros.h"
#include "purc-errors.h"

struct purc_rwstream;
typedef struct purc_rwstream purc_rwstream;
typedef struct purc_rwstream* purc_rwstream_t;

PCA_EXTERN_C_BEGIN

/**
 * Creates a new purc_rwstream_t for the given memory buffer.
 *
 * @param mem: pointer to memory buffer
 * @param sz:  size of memory buffer
 *
 * Returns: A purc_rwstream_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t purc_rwstream_new_from_mem (void* mem, size_t sz);

/**
 * Creates a new purc_rwstream_t for the given file and mode.
 *
 * @param file: the file will be opened
 * @param mode: One of "r", "w", "a", "r+", "w+", "a+". These have the same
 *        meaning as in fopen()
 *
 * Returns: A purc_rwstream_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t
purc_rwstream_new_from_file (const char* file, const char* mode);

/**
 * Creates a new purc_rwstream_t for the given FILE pointer.
 *
 * @param fp: FILE pointer
 *
 * Returns: A purc_rwstream_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t purc_rwstream_new_from_fp (FILE* fp);

/**
 * Creates a new purc_rwstream_t for the given file descriptor (Unix && GLIB).
 *
 * @param fd: file descriptor
 * @param sz_buf: buffer size
 *
 * Returns: A purc_rwstream_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t
purc_rwstream_new_from_unix_fd (int fd, size_t sz_buf);

/**
 * Creates a new purc_rwstream_t for the given socket on Windows (Win32 && GLIB).
 *
 * @param socket:  sockets created by Winsock
 * @param sz_buf: buffer size
 *
 * Returns: A purc_rwstream_t on success, NULL on failure.
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t
purc_rwstream_new_from_win32_socket (int socket, size_t sz_buf);

/**
 * Release the purc_rwstream_t
 *
 * @param rws: purc_rwstream_t
 *
 * Returns: 0 success, non-zero otherwise.
 *
 * Since: 0.0.1
 */
PCA_EXPORT int purc_rwstream_destroy (purc_rwstream_t rws);

/**
 * Sets the current position in the purc_rwstream_t, similar to the standard
 * library function fseek().
 *
 * @param rws: purc_rwstream_t
 * @param offset: n offset, in bytes, which is added to the position specified
 *        by whence
 * @param whence: the position in the file, which can be purc_rwstream_t_SEEK_CUR (the
 *        current position), purc_rwstream_t_SEEK_SET (the start of the file),
 *        or purc_rwstream_t_SEEK_END (the end of the file)
 *
 * Returns: success returns the resulting offset location as measured in bytes from
 *        the beginning of the file,  (off_t) -1 otherwise.
 *
 * Since: 0.0.1
 */
PCA_EXPORT off_t
purc_rwstream_seek (purc_rwstream_t rws, off_t offset, int whence);

/**
 * Obtains the current position of purc_rwstream_t, similar to the standard
 * library function ftell().
 *
 * @param rws: pointer to purc_rwstream_t
 *
 * Returns: success returns the current offset, (off_t) -1 not support.
 *
 * Since: 0.0.1
 */
PCA_EXPORT off_t purc_rwstream_tell (purc_rwstream_t rws);

/**
 * Reads data from a purc_rwstream_t
 *
 * @param rws: purc_rwstream_t
 * @param buf: a buffer to read the data into
 * @param count: the number of bytes to read
 *
 * Returns: the number of bytes actually read
 *
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t
purc_rwstream_read (purc_rwstream_t rws, void* buf, size_t count);

/**
 * Reads a character(UTF-8) from purc_rwstream_t and convert to wchat_t.
 * not be freed until using purc_rwstream_free.
 *
 * @param rws: purc_rwstream_t
 * @param buf_utf8: the buffer to read character into
 * @param buf_wc: the buffer to convert character into
 *
 * Returns: the length of character
 *
 * Since: 0.0.1
 */
PCA_EXPORT int
purc_rwstream_read_utf8_char (purc_rwstream_t rws, char* buf_utf8, wchar_t* buf_wc);


/**
 * Write data to purc_rwstream_t
 *
 * @param rws: purc_rwstream_t
 * @param buf: the buffer containing the data to write
 * @param count: the number of bytes to write
 *
 * Returns: the number of bytes actually written
 *
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t
purc_rwstream_write (purc_rwstream_t rws, const void* buf, size_t count);


/**
 * Flushes the write buffer for the purc_rwstream_t.
 *
 * @param rws: pointer to purc_rwstream_t
 *
 * Returns: 0 success, non-zero otherwise.
 *
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t purc_rwstream_flush (purc_rwstream_t rws);

/**
 * Close an purc_rwstream_t. Any pending data to be written will be flushed. The channel will
 * not be freed until using purc_rwstream_free.
 *
 * @param rws: pointer to purc_rwstream_t
 *
 * Returns: 0 success, non-zero otherwise.
 *
 * Since: 0.0.1
 */
PCA_EXPORT int purc_rwstream_close (purc_rwstream_t rws);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_RWSTREAM_H */

