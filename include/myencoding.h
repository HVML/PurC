/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
** Copyright (C) 2015-2017 Alexander Borisov
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
** Author: Vincent Wei <https://github.com/VincentWei>
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#ifndef PCAT2_MyENCODING_H
#define PCAT2_MyENCODING_H

#pragma once

/**
 * @file myencoding.h
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "pcat2_version.h"
#include "pcat2_macros.h"
#include "myencoding/myosi.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************
 *
 * MyENCODING
 *
 ***********************************************************************************/

/**
 * Convert Unicode Codepoint to UTF-16LE
 *
 * I advise not to use UTF-16! Use UTF-8 and be happy!
 *
 * @param[in] Codepoint
 * @param[in] Data to set characters. Data length is 2 or 4 bytes
 *   data length must be always available 4 bytes
 *
 * @return size character set
 */
size_t
myencoding_codepoint_to_ascii_utf_16(size_t codepoint, char *data);

/**
 * Detect character encoding
 *
 * Now available for detect UTF-8, UTF-16LE, UTF-16BE
 * and Russians: windows-1251,  koi8-r, iso-8859-5, x-mac-cyrillic, ibm866
 * Other in progress
 *
 * @param[in]  text
 * @param[in]  text length
 * @param[out] detected encoding
 *
 * @return true if encoding found, otherwise false
 */
bool
myencoding_detect(const char *text, size_t length, myencoding_t *encoding);

/**
 * Detect Russian character encoding
 *
 * Now available for detect windows-1251,  koi8-r, iso-8859-5, x-mac-cyrillic, ibm866
 *
 * @param[in]  text
 * @param[in]  text length
 * @param[out] detected encoding
 *
 * @return true if encoding found, otherwise false
 */
bool
myencoding_detect_russian(const char *text, size_t length, myencoding_t *encoding);

/**
 * Detect Unicode character encoding
 *
 * Now available for detect UTF-8, UTF-16LE, UTF-16BE
 *
 * @param[in]  text
 * @param[in]  text length
 * @param[out] detected encoding
 *
 * @return true if encoding found, otherwise false
 */
bool
myencoding_detect_unicode(const char *text, size_t length, myencoding_t *encoding);

/**
 * Detect Unicode character encoding by BOM
 *
 * Now available for detect UTF-8, UTF-16LE, UTF-16BE
 *
 * @param[in]  text
 * @param[in]  text length
 * @param[out] detected encoding
 *
 * @return true if encoding found, otherwise false
 */
bool
myencoding_detect_bom(const char *text, size_t length, myencoding_t *encoding);

/**
 * Detect Unicode character encoding by BOM. Cut BOM if will be found
 *
 * Now available for detect UTF-8, UTF-16LE, UTF-16BE
 *
 * @param[in]  text
 * @param[in]  text length
 * @param[out] detected encoding
 * @param[out] new text position
 * @param[out] new size position
 *
 * @return true if encoding found, otherwise false
 */
bool
myencoding_detect_and_cut_bom(const char *text, size_t length, myencoding_t *encoding,
                                   const char **new_text, size_t *new_size);

/**
 * Detect encoding by name
 * Names like: windows-1258 return MyENCODING_WINDOWS_1258
 *             cp1251 or windows-1251 return MyENCODING_WINDOWS_1251
 *
 * See https://encoding.spec.whatwg.org/#names-and-labels
 *
 * @param[in]  name
 * @param[in]  name length
 * @param[out] detected encoding
 *
 * @return true if encoding found, otherwise false
 */
bool
myencoding_by_name(const char *name, size_t length, myencoding_t *encoding);

/**
 * Get Encoding name by myencoding_t (by id)
 *
 * @param[in]  myencoding_t, encoding id
 * @param[out] return name length
 *
 * @return encoding name, otherwise NULL value
 */
const char*
myencoding_name_by_id(myencoding_t encoding, size_t *length);

/**
 * Detect encoding in meta tag (<meta ...>) before start parsing
 *
 * See https://html.spec.whatwg.org/multipage/syntax.html#prescan-a-byte-stream-to-determine-its-encoding
 *
 * @param[in]  html data bytes
 * @param[in]  html data length
 *
 * @return detected encoding if encoding found, otherwise MyENCODING_NOT_DETERMINED
 */
myencoding_t
myencoding_prescan_stream_to_determine_encoding(const char *data, size_t data_size);

/**
 * Extracting character encoding from string. Find "charset=" and see encoding. 
 * For example: "text/html; charset=windows-1251". Return MyENCODING_WINDOWS_1251
 *
 *
 * See https://html.spec.whatwg.org/multipage/infrastructure.html#algorithm-for-extracting-a-character-encoding-from-a-meta-element
 *
 * @param[in]  data
 * @param[in]  data length
 * @param[out] return encoding
 *
 * @return true if encoding found
 */
bool
myencoding_extracting_character_encoding_from_charset(const char *data, size_t data_size,
                                                           myencoding_t *encoding);

/**
 * Detect encoding in meta tag (<meta ...>) before start parsing and return found raw data
 *
 * See https://html.spec.whatwg.org/multipage/syntax.html#prescan-a-byte-stream-to-determine-its-encoding
 *
 * @param[in]  html data bytes
 * @param[in]  html data length
 * @param[out] return raw char data point for find encoding
 * @param[out] return raw char length
 *
 * @return detected encoding if encoding found, otherwise MyENCODING_NOT_DETERMINED
 */
myencoding_t
myencoding_prescan_stream_to_determine_encoding_with_found(const char *data, size_t data_size,
                                                           const char **found, size_t *found_length);

/**
 * Extracting character encoding from string. Find "charset=" and see encoding. Return found raw data.
 * For example: "text/html; charset=windows-1251". Return MyENCODING_WINDOWS_1251
 *
 *
 * See https://html.spec.whatwg.org/multipage/infrastructure.html#algorithm-for-extracting-a-character-encoding-from-a-meta-element
 *
 * @param[in]  data
 * @param[in]  data length
 * @param[out] return encoding
 * @param[out] return raw char data point for find encoding
 * @param[out] return raw char length
 *
 * @return true if encoding found
 */
bool
myencoding_extracting_character_encoding_from_charset_with_found(const char *data, size_t data_size,
                                                                 myencoding_t *encoding,
                                                                 const char **found, size_t *found_length);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCAT2_MyENCODING_H */

