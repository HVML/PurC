/**
 * @file encoding.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for encoding.
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


#ifndef PCHTML_ENCODING_ENCODE_H
#define PCHTML_ENCODING_ENCODE_H

#ifdef __cplusplus
extern "C" {
#endif


#include "config.h"
#include "html/encoding/base.h"


unsigned int
pchtml_encoding_encode_default(pchtml_encoding_encode_t *ctx,
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_auto(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_undefined(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_big5(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_euc_jp(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_euc_kr(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_gbk(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_ibm866(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_2022_jp(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_2022_jp_eof(
            pchtml_encoding_encode_t *ctx) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_10(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_13(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_14(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_15(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_16(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_2(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_3(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_4(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_5(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_6(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_7(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_8(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_iso_8859_8_i(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_koi8_r(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_koi8_u(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_shift_jis(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_utf_16be(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_utf_16le(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_utf_8(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_gb18030(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_macintosh(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_replacement(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_1250(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_1251(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_1252(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_1253(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_1254(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_1255(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_1256(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_1257(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_1258(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_windows_874(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_x_mac_cyrillic(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_x_user_defined(pchtml_encoding_encode_t *ctx, 
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

/*
 * Single
 */
int8_t
pchtml_encoding_encode_default_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_auto_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_undefined_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_big5_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_euc_jp_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_euc_kr_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_gbk_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_ibm866_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_2022_jp_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_2022_jp_eof_single(pchtml_encoding_encode_t *ctx,
            unsigned char **data, const unsigned char *end) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_10_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_13_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_14_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_15_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_16_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_2_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_3_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_4_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_5_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_6_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_7_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_8_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_iso_8859_8_i_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_koi8_r_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_koi8_u_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_shift_jis_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_utf_16be_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_utf_16le_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_utf_8_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_gb18030_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_macintosh_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_replacement_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_1250_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_1251_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_1252_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_1253_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_1254_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_1255_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_1256_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_1257_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_1258_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_windows_874_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_x_mac_cyrillic_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_x_user_defined_single(pchtml_encoding_encode_t *ctx, 
            unsigned char **data, const unsigned char *end, 
            uint32_t cp) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_ENCODING_ENCODE_H */
