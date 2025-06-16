/**
 * @file purc-rwstream.h
 * @author XueShuming
 * @date 2021/07/02
 * @brief The API for RWStream.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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
#include <string.h>
#include <wchar.h>

#include "purc-macros.h"

struct purc_rwstream;
typedef struct purc_rwstream purc_rwstream;
typedef struct purc_rwstream* purc_rwstream_t;

PCA_EXTERN_C_BEGIN


/**
 * Creates a new purc_rwstream_t for the automatic memory buffer.
 *
 * @param sz_init: the init size of the memory buffer
 * @param sz_max: the max size of the memory buffer
 *
 * @return A purc_rwstream_t on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *
 * Since: 0.0.1
 */
PCA_EXPORT
purc_rwstream_t purc_rwstream_new_buffer (size_t sz_init, size_t sz_max);

/**
 * Creates a new purc_rwstream_t for the given memory buffer.
 *
 * @param mem: pointer to memory buffer
 * @param sz:  size of memory buffer
 *
 * @return A purc_rwstream_t on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
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
 * @return A purc_rwstream_t on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_BAD_SYSTEM_CALL: Bad system call
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
 * @return A purc_rwstream_t on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_BAD_SYSTEM_CALL: Bad system call
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t purc_rwstream_new_from_fp (FILE* fp);

/**
 * Creates a new purc_rwstream_t for the given file descriptor (Unix && GLIB).
 * The fd must be in blocking mode, otherwise the fd will be set in blocking
 * mode.
 *
 * @param fd: file descriptor
 *
 * @return A purc_rwstream_t on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_BAD_SYSTEM_CALL: Bad system call
 *  - @PURC_ERROR_OUT_OF_MEMORY: Out of memory
 *  - @PURC_ERROR_NOT_IMPLEMENTED: Not implemented
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t
purc_rwstream_new_from_unix_fd (int fd);

/**
 * Creates a new purc_rwstream_t for the given socket on Windows (Win32 && GLIB).
 * The socket must be in blocking mode, otherwise the socket will be set in
 * blocking mode.
 *
 * @param socket:  sockets created by Winsock
 * @param sz_buf: buffer size
 *
 * @return A purc_rwstream_t on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_BAD_SYSTEM_CALL: Bad system call
 *  - @PURC_ERROR_OUT_OF_MEMORY: Out of memory
 *  - @PURC_ERROR_NOT_IMPLEMENTED: Not implemented
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t
purc_rwstream_new_from_win32_socket (int socket, size_t sz_buf);

typedef ssize_t (*pcrws_cb_write)(void *ctxt, const void *buf, size_t count);

/**
 * Creates a new purc_rwstream_t which is dedicated for dump only,
 * that is, the new purc_rwstream_t is write-only and not seekable.
 *
 * @param ctxt: the buffer
 *
 * @return A purc_rwstream_t on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_OUT_OF_MEMORY: Out of memory
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t
purc_rwstream_new_for_dump (void *ctxt, pcrws_cb_write fn);

typedef ssize_t (*pcrws_cb_read)(void *ctxt, void *buf, size_t count);

/**
 * Creates a new purc_rwstream_t which is dedicated for read only,
 * that is, the new purc_rwstream_t is read-only and not seekable.
 *
 * @param ctxt: the buffer
 *
 * @return A purc_rwstream_t on success, @NULL on failure and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_OUT_OF_MEMORY: Out of memory
 *
 * Since: 0.0.1
 */
PCA_EXPORT purc_rwstream_t
purc_rwstream_new_for_read (void *ctxt, pcrws_cb_read fn);

/**
 * Release the purc_rwstream_t
 *
 * @param rws: purc_rwstream_t
 *
 * @return 0 success, -1 otherwise and the error code is set to indicate
 *         the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_BAD_SYSTEM_CALL: Bad system call
 *  - @PURC_ERROR_OUT_OF_MEMORY: Out of memory
 *  - @PURC_ERROR_NOT_IMPLEMENTED: Not implemented
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
 * @param whence: the position in the file, which can be
 *        SEEK_CUR (the current position),
 *        SEEK_SET (the start of the file),
 *        SEEK_END (the end of the file)
 *
 * @return success returns the resulting offset location as measured in bytes
 *         from the beginning of the file,  (off_t) -1 otherwise and the error
 *         code is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_BAD_SYSTEM_CALL: Bad system call
 *  - @PURC_ERROR_NOT_IMPLEMENTED: Not implemented
 *  - @PCRWSTREAM_ERROR_FILE_TOO_BIG: File too large
 *  - @PCRWSTREAM_ERROR_IO: IO error
 *  - @PCRWSTREAM_ERROR_IS_DIR: File is a directory
 *  - @PCRWSTREAM_ERROR_NO_SPACE: No space left on device
 *  - @PCRWSTREAM_ERROR_NO_DEVICE_OR_ADDRESS: No such device or address
 *  - @PCRWSTREAM_ERROR_OVERFLOW: Value too large for defined datatype
 *  - @PCRWSTREAM_ERROR_FAILED: Rwstream failed with some other error
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
 * @return success returns the current offset, (off_t) -1 otherwise and
 *         the error code is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_NOT_IMPLEMENTED: Not implemented
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
 * @return the number of bytes actually read and the error code is set to
 *         indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PCRWSTREAM_ERROR_FILE_TOO_BIG: File too large"
 *  - @PCRWSTREAM_ERROR_IO: IO error
 *  - @PCRWSTREAM_ERROR_IS_DIR: File is a directory
 *  - @PCRWSTREAM_ERROR_NO_SPACE: No space left on device
 *  - @PCRWSTREAM_ERROR_NO_DEVICE_OR_ADDRESS: No such device or address
 *  - @PCRWSTREAM_ERROR_OVERFLOW: Value too large for defined datatype
 *  - @PCRWSTREAM_ERROR_FAILED: Rwstream failed with some other error
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t
purc_rwstream_read (purc_rwstream_t rws, void* buf, size_t count);

/**
 * Reads a character(UTF-8) from purc_rwstream_t and convert to wchat_t.
 *
 * @param rws: purc_rwstream_t
 * @param buf_utf8: the buffer to read character into
 * @param buf_wc (nullable): the buffer to convert character into
 *
 * @return the length of character and the error code is set to indicate the
 *         error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PCRWSTREAM_ERROR_FILE_TOO_BIG: File too large"
 *  - @PCRWSTREAM_ERROR_IO: IO error
 *  - @PCRWSTREAM_ERROR_IS_DIR: File is a directory
 *  - @PCRWSTREAM_ERROR_NO_SPACE: No space left on device
 *  - @PCRWSTREAM_ERROR_NO_DEVICE_OR_ADDRESS: No such device or address
 *  - @PCRWSTREAM_ERROR_OVERFLOW: Value too large for defined datatype
 *  - @PCRWSTREAM_ERROR_FAILED: Rwstream failed with some other error
 *
 * Since: 0.0.1
 */
PCA_EXPORT int
purc_rwstream_read_utf8_char (purc_rwstream_t rws,
        char* buf_utf8, uint32_t* buf_wc);

/**
 * Write data to purc_rwstream_t
 *
 * @param rws: purc_rwstream_t
 * @param buf: the buffer containing the data to write
 * @param count: the number of bytes to write
 *
 * @return the number of bytes actually written and the error code is set to
 *         indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PCRWSTREAM_ERROR_FILE_TOO_BIG: File too large"
 *  - @PCRWSTREAM_ERROR_IO: IO error
 *  - @PCRWSTREAM_ERROR_IS_DIR: File is a directory
 *  - @PCRWSTREAM_ERROR_NO_SPACE: No space left on device
 *  - @PCRWSTREAM_ERROR_NO_DEVICE_OR_ADDRESS: No such device or address
 *  - @PCRWSTREAM_ERROR_OVERFLOW: Value too large for defined datatype
 *  - @PCRWSTREAM_ERROR_FAILED: Rwstream failed with some other error
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
 * @return 0 success, non-zero otherwise and the error code is set to indicate
 *         the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_NOT_IMPLEMENTED: Not implemented
 *
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t purc_rwstream_flush (purc_rwstream_t rws);

/**
 * Read the count bytes from the in rwstream, write to out rwstream,
 * and return the number of bytes written
 *
 * @param in: pointer to purc_rwstream_t to be read
 * @param out: pointer to purc_rwstream_t to be write
 * @param count: the read/write count. -1 means until EOF
 *
 * @return success, the number of bytes written. -1 otherwise and the error code
 *         is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PCRWSTREAM_ERROR_FILE_TOO_BIG: File too large"
 *  - @PCRWSTREAM_ERROR_IO: IO error
 *  - @PCRWSTREAM_ERROR_IS_DIR: File is a directory
 *  - @PCRWSTREAM_ERROR_NO_SPACE: No space left on device
 *  - @PCRWSTREAM_ERROR_NO_DEVICE_OR_ADDRESS: No such device or address
 *  - @PCRWSTREAM_ERROR_OVERFLOW: Value too large for defined datatype
 *  - @PCRWSTREAM_ERROR_FAILED: Rwstream failed with some other error
 *
 * Since: 0.0.1
 */
PCA_EXPORT ssize_t purc_rwstream_dump_to_another (purc_rwstream_t in,
        purc_rwstream_t out, ssize_t count);

/**
 * Get the pointer and size of the rwstream whose type is memory (Created by
 * purc_rwstream_new_buffer or purc_rwstream_new_from_mem).
 * This is the extended version of @purc_rwstream_get_mem_buffer.
 *
 * @param rw_mem: the purc_rwstream_t object.
 * @param sz_content: (nullable): pointer to receive the size of content.
 * @param sz_buffer: (nullable): pointer to receive the size of buffer.
 * @param res_buff: whether to reserve the buffer for reuse (do not call
 *      free() when destroying the rwstream object).
 *
 * @return success returns the pointer of the memory, @NULL not support and
 *         the error code is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_NOT_IMPLEMENTED: Not implemented
 *
 * Since: 0.0.2
 */
PCA_EXPORT void* purc_rwstream_get_mem_buffer_ex (purc_rwstream_t rw_mem,
        size_t *sz_content, size_t *sz_buffer, bool res_buff);

/**
 * Get the pointer and size of the rwstream whose type is memory (Created by
 * purc_rwstream_new_buffer or purc_rwstream_new_from_mem).
 *
 * @param rw_mem: the purc_rwstream_t object.
 * @param sz_content: (nullable): pointer to receive the size of content.
 *
 * @return success returns the pointer of the memory, @NULL not support and
 *         the error code is set to indicate the error. The error code:
 *  - @PURC_ERROR_INVALID_VALUE: Invalid value
 *  - @PURC_ERROR_NOT_IMPLEMENTED: Not implemented
 *
 * @note: API changed since 0.0.2 (return the size of the buffer).
 *
 * Since: 0.0.1
 */
static inline void* purc_rwstream_get_mem_buffer (purc_rwstream_t rw_mem,
        size_t *sz_content)
{
    return purc_rwstream_get_mem_buffer_ex (rw_mem, sz_content,
            NULL, false);
}

static inline ssize_t purc_rwstream_write_str (purc_rwstream_t rws,
        const char *str)
{
    return purc_rwstream_write(rws, str, strlen(str));
}


PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_RWSTREAM_H */

