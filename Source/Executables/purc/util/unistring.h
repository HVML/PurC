/*
 * unistring.h - simple Unicode string.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Author: Vincent Wei <https://github.com/VincentWei>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __foil_util_unistring_h
#define __foil_util_unistring_h

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <sys/types.h>

/**
 * The foil_unistr struct contains the public fields of a foil_unistr.
 */
typedef struct foil_unistr {
    uint32_t *ucs;
    size_t    len;
    size_t    sz;
} foil_unistr;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new %foil_unistr with @len bytes in UTF-8 encoding
 * of the @init buffer. Because a length is provided, @init need not be
 * nul-terminated, and can contain embedded nul bytes.
 *
 * Since this function does not stop at nul bytes, it is the caller’s
 * responsibility to ensure that @init has at least @len addressable bytes.
 */
foil_unistr *foil_unistr_new_len(const char *str_utf8, ssize_t len);

/**
 * Creates a new %foil_unistr, initialized with the given null-terminated
 * UTF-8 string.
 */
static inline foil_unistr *foil_unistr_new(const char *str_utf8)
{
    return foil_unistr_new_len(str_utf8, -1);
}

/**
 * Creates a new %foil_unistr, with enough space for @dfl_size characters.
 * This is useful if you are going to add a lot of text to the string and
 * don’t want it to be reallocated too often.
 */
foil_unistr *foil_unistr_sized_new(size_t dfl_size);

/**
 * Clones a new %foil_unistr, with an existing %foil_unistr.
 */
foil_unistr *foil_unistr_clone(foil_unistr *unistr);

/**
 * Creates a new %foil_unistr, with an array of Unicode characters @ucs
 * which is @len long.
 */
foil_unistr *foil_unistr_new_ucs(const char *ucs, size_t len);

/**
 * Creates a new %foil_unistr by taking the ownership of the buffer
 * given by @ucs and @len.
 */
foil_unistr *foil_unistr_new_moving_in(uint32_t *ucs, size_t len);

/**
 * Sets the length of a foil_unistr. If the length is less than the current
 * length, the string will be truncated. If the length is greater than the
 * current length, the contents of the newly added area are undefined.
 */
foil_unistr *foil_unistr_set_size(foil_unistr *unistr, size_t len);

/**
 * Frees the memory allocated for the foil_unistr. If @free_segment is %true
 * it also frees the character data. If it’s %false, the caller gains ownership
 * of the buffer and must free it after use with free().
 */
uint32_t *foil_unistr_free(foil_unistr *unistr, bool free_segment);

/**
 * Deletes a foil_unistr and free the character data
 */
static inline void foil_unistr_delete(foil_unistr *unistr)
{
    foil_unistr_free(unistr, true);
}

/**
 * Inserts all Unicode characters in a UTF-8 string into a foil_unistr
 * at the given position, expanding it if necessary.
 */
foil_unistr *foil_unistr_insert_len(foil_unistr *unistr, ssize_t pos,
        const char *str_utf8, ssize_t len);

/**
 * Inserts all Unicode characters in a null-terminated UTF-8 string into a
 * foil_unistr at the given position, expanding it if necessary.
 */
static inline foil_unistr *
foil_unistr_insert(foil_unistr *unistr, ssize_t pos, const char *str_utf8)
{
    return foil_unistr_insert_len(unistr, pos, str_utf8, -1);
}

/**
 * Inserts a Unicode character into a foil_unistr at the given position,
 * expanding it if necessary.
 */
foil_unistr *foil_unistr_insert_unichar(foil_unistr *unistr, ssize_t pos,
        uint32_t unichar);

/**
 * Prepends all Unicode characters in a UTF-8 string into a foil_unistr,
 * expanding it if necessary.
 */
static inline foil_unistr *
foil_unistr_prepend(foil_unistr *unistr, const char *str_utf8)
{
    return foil_unistr_insert(unistr, 0, str_utf8);
}

/**
 * Prepends a Unicode character into a foil_unistr,
 * expanding it if necessary.
 */
static inline foil_unistr *
foil_unistr_prepend_unichar(foil_unistr *unistr, uint32_t unichar)
{
    return foil_unistr_insert_unichar(unistr, 0, unichar);
}

/**
 * Appends all Unicode characters in a UTF-8 string into a foil_unistr,
 * expanding it if necessary.
 */
static inline foil_unistr *
foil_unistr_append(foil_unistr *unistr, const char *str_utf8)
{
    return foil_unistr_insert(unistr, -1, str_utf8);
}

/**
 * Appends a Unicode character into a foil_unistr,
 * expanding it if necessary.
 */
static inline foil_unistr *
foil_unistr_append_unichar(foil_unistr *unistr, uint32_t unichar)
{
    return foil_unistr_insert_unichar(unistr, -1, unichar);
}

/**
 * Removes len bytes from a foil_unistr, starting at position @pos.
 * The rest of the foil_unistr is shifted down to fill the gap.
 */
foil_unistr *foil_unistr_erase(foil_unistr *unistr, size_t pos, ssize_t len);

/**
 * Cuts off the end of the foil_unistr, leaving the first @len characters.
 */
foil_unistr *foil_unistr_truncate(foil_unistr *unistr, size_t len);

/**
 * Copies all Unicode characters from a UTF-8 string into a foil_unistr,
 * destroying any previous contents.
 * It is rather like the standard strncpy() function,
 * except that you do not have to worry about having enough space to copy
 * the string.
 */
foil_unistr *foil_unistr_assign_len(foil_unistr *unistr,
        const char *str_utf8, ssize_t len);

/**
 * Copies all Unicode characters from a UTF-8 string into a foil_unistr,
 * destroying any previous contents.
 * It is rather like the standard strcpy() function,
 * except that you do not have to worry about having enough space to copy
 * the string.
 */
static inline foil_unistr *
foil_unistr_assign(foil_unistr *unistr, const char *str_utf8)
{
    return foil_unistr_assign_len(unistr, str_utf8, -1);
}

#ifdef __cplusplus
}
#endif

#endif  /* __foil_util_unistring_h */
