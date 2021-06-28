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
 * @param mode: One of "r", "w", "a", "r+", "w+", "a+". These have the same
 *        meaning as in fopen()
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
 * @param autoclose: Whether to automatically close the fp when the
 *        PURC_RWSTREAM is freed.
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
 * @param autoclose: Whether to automatically close the fp when the
 *        PURC_RWSTREAM is freed.
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

/**
 * Sets the current position in the PURC_RWSTREAM, similar to the standard
 * library function fseek().
 *
 * @param rws: pointer to PURC_RWSTREAM
 * @param offset: n offset, in bytes, which is added to the position specified
 *        by whence
 * @param whence: the position in the file, which can be PURC_RWSTREAM_SEEK_CUR (the
 *        current position), PURC_RWSTREAM_SEEK_SET (the start of the file),
 *        or PURC_RWSTREAM_SEEK_END (the end of the file)
 *
 * Returns: success returns the resulting offset location as measured in bytes from
 *        the beginning of the file,  (off_t) -1 otherwise.
 *
 * Since: 0.0.1
 */
off_t purc_rwstream_seek (PURC_RWSTREAM* rws, off_t offset, int whence);

/**
 * Obtains the current position of PURC_RWSTREAM, similar to the standard
 * library function ftell().
 *
 * @param rws: pointer to PURC_RWSTREAM
 *
 * Returns: success returns the current offset, (off_t) -1 otherwise.
 *
 * Since: 0.0.1
 */
off_t purc_rwstream_tell (PURC_RWSTREAM* rws);

/**
 * Tests the end-of-file indicator for PURC_RWSTREAM
 * library function feof().
 *
 * @param rws: pointer to PURC_RWSTREAM
 *
 * Returns: nonzero if end-of-file indicator is set.
 *
 * Since: 0.0.1
 */
int purc_rwstream_eof (PURC_RWSTREAM* rws);

/**
 * Reads data from a PURC_RWSTREAM
 *
 * @param rws: pointer to PURC_RWSTREAM
 * @param buf: a buffer to read the data into
 * @param count: the number of bytes to read
 *
 * Returns: the number of bytes actually read
 *
 * Since: 0.0.1
 */
ssize_t purc_rwstream_read (PURC_RWSTREAM* rws, void* buf, size_t count);

/**
 * Reads data to PURC_RWSTREAM
 *
 * @param rws: pointer to PURC_RWSTREAM
 * @param buf: the buffer containing the data to write
 * @param count: the number of bytes to write
 *
 * Returns: the number of bytes actually written
 *
 * Since: 0.0.1
 */
ssize_t purc_rwstream_write (PURC_RWSTREAM* rws, const void* buf, size_t count);

/**
 * Reads a character(UTF-8) from PURC_RWSTREAM and convert to wchat_t.
 * not be freed until using purc_rwstream_free.
 *
 * @param rws: pointer to PURC_RWSTREAM
 * @param buf_utf8: the buffer to read character into
 * @param buf_wc: the buffer to convert character into
 *
 * Returns: the length of character
 *
 * Since: 0.0.1
 */
int purc_rwstream_read_utf8_char (PURC_RWSTREAM* rws, char* buf_utf8, wchar_t* buf_wc);

/**
 * Close an PURC_RWSTREAM. Any pending data to be written will be flushed. The channel will
 * not be freed until using purc_rwstream_free.
 *
 * @param rws: pointer to PURC_RWSTREAM
 *
 * Returns: 0 success, non-zero otherwise.
 *
 * Since: 0.0.1
 */
int purc_rwstream_close (PURC_RWSTREAM* rws);



#endif /* PURC_MYCORE_RWSTREAM_H */

