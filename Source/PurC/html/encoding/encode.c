/**
 * @file encode.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of encode.
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

#include "private/errors.h"

#include "html/encoding/encode.h"
#include "html/encoding/single.h"
#include "html/encoding/multi.h"
#include "html/encoding/range.h"


#define PCHTML_ENCODING_ENCODE_APPEND(ctx, cp)                                    \
    do {                                                                       \
        if ((ctx)->buffer_used == (ctx)->buffer_length) {                      \
            return PCHTML_STATUS_SMALL_BUFFER;                                    \
        }                                                                      \
                                                                               \
        (ctx)->buffer_out[(ctx)->buffer_used++] = (unsigned char) cp;             \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_ENCODE_APPEND_P(ctx, cp)                                  \
    do {                                                                       \
        if ((ctx)->buffer_used == (ctx)->buffer_length) {                      \
            *cps = p;                                                          \
            return PCHTML_STATUS_SMALL_BUFFER;                                    \
        }                                                                      \
                                                                               \
        (ctx)->buffer_out[(ctx)->buffer_used++] = (unsigned char) cp;             \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_ENCODE_ERROR(ctx)                                         \
    do {                                                                       \
        if (ctx->replace_to == NULL) {                                         \
            return PCHTML_STATUS_ERROR;                                           \
        }                                                                      \
                                                                               \
        if ((ctx->buffer_used + ctx->replace_len) > ctx->buffer_length) {      \
            return PCHTML_STATUS_SMALL_BUFFER;                                    \
        }                                                                      \
                                                                               \
        memcpy(&ctx->buffer_out[ctx->buffer_used], ctx->replace_to,            \
               ctx->replace_len);                                              \
                                                                               \
        ctx->buffer_used += ctx->replace_len;                                  \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_ENCODE_ERROR_P(ctx)                                       \
    do {                                                                       \
        if (ctx->replace_to == NULL) {                                         \
            *cps = p;                                                          \
            return PCHTML_STATUS_ERROR;                                           \
        }                                                                      \
                                                                               \
        if ((ctx->buffer_used + ctx->replace_len) > ctx->buffer_length) {      \
            *cps = p;                                                          \
            return PCHTML_STATUS_SMALL_BUFFER;                                    \
        }                                                                      \
                                                                               \
        memcpy(&ctx->buffer_out[ctx->buffer_used], ctx->replace_to,            \
               ctx->replace_len);                                              \
                                                                               \
        ctx->buffer_used += ctx->replace_len;                                  \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_ENCODE_SINGLE_BYTE(table, table_size)                     \
    do {                                                                       \
        uint32_t cp;                                                    \
        const uint32_t *p = *cps;                                       \
        const pchtml_shs_hash_t *hash;                                         \
                                                                               \
        for (; p < end; p++) {                                                 \
            cp = *p;                                                           \
                                                                               \
            if (cp < 0x80) {                                                   \
                PCHTML_ENCODING_ENCODE_APPEND_P(ctx, cp);                         \
                continue;                                                      \
            }                                                                  \
                                                                               \
            hash = pchtml_shs_hash_get_static(table, table_size, cp);          \
            if (hash == NULL) {                                                \
                PCHTML_ENCODING_ENCODE_ERROR_P(ctx);                              \
                continue;                                                      \
            }                                                                  \
                                                                               \
            PCHTML_ENCODING_ENCODE_APPEND_P(ctx, (uintptr_t) hash->value);        \
        }                                                                      \
                                                                               \
        return PCHTML_STATUS_OK;                                                  \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_ENCODE_BYTE_SINGLE(table, table_size)                     \
    const pchtml_shs_hash_t *hash;                                             \
                                                                               \
    if (cp < 0x80) {                                                           \
        *(*data)++ = (unsigned char) cp;                                          \
        return 1;                                                              \
    }                                                                          \
                                                                               \
    hash = pchtml_shs_hash_get_static(table, table_size, cp);                  \
    if (hash == NULL) {                                                        \
        return PCHTML_ENCODING_ENCODE_ERROR;                                      \
    }                                                                          \
                                                                               \
    *(*data)++ = (unsigned char) (uintptr_t) hash->value;                         \
    return 1


unsigned int
pchtml_encoding_encode_default(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                            const uint32_t *end)
{
    return pchtml_encoding_encode_utf_8(ctx, cps, end);
}

unsigned int
pchtml_encoding_encode_auto(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                         const uint32_t *end)
{
    UNUSED_PARAM(ctx);

    *cps = end;
    return PCHTML_STATUS_ERROR;
}

unsigned int
pchtml_encoding_encode_undefined(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                              const uint32_t *end)
{
    UNUSED_PARAM(ctx);

    *cps = end;
    return PCHTML_STATUS_ERROR;
}

unsigned int
pchtml_encoding_encode_big5(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                         const uint32_t *end)
{
    uint32_t cp;
    const pchtml_shs_hash_t *hash;

    for (; *cps < end; (*cps)++) {
        cp = **cps;

        if (cp < 0x80) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, cp);
            continue;
        }

        hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_big5,
                                          PCHTML_ENCODING_MULTI_HASH_BIG5_SIZE, cp);
        if (hash == NULL) {
            PCHTML_ENCODING_ENCODE_ERROR(ctx);
            continue;
        }

        if ((ctx->buffer_used + 2) > ctx->buffer_length) {
            return PCHTML_STATUS_SMALL_BUFFER;
        }

        ctx->buffer_out[ ctx->buffer_used++ ] = ((uint32_t) (uintptr_t) hash->value) / 157 + 0x81;

        if ((((uint32_t) (uintptr_t) hash->value) % 157) < 0x3F) {
            ctx->buffer_out[ ctx->buffer_used++ ] = (((uint32_t) (uintptr_t) hash->value) % 157) + 0x40;
        }
        else {
            ctx->buffer_out[ ctx->buffer_used++ ] = (((uint32_t) (uintptr_t) hash->value) % 157) + 0x62;
        }
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_encode_euc_jp(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                           const uint32_t *end)
{
    uint32_t cp;
    const pchtml_shs_hash_t *hash;

    for (; *cps < end; (*cps)++) {
        cp = **cps;

        if (cp < 0x80) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, cp);
            continue;
        }

        if (cp == 0x00A5) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, 0x5C);
            continue;
        }

        if (cp == 0x203E) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, 0x7E);
            continue;
        }

        if ((unsigned) (cp - 0xFF61) <= (0xFF9F - 0xFF61)) {
            if ((ctx->buffer_used + 2) > ctx->buffer_length) {
                return PCHTML_STATUS_SMALL_BUFFER;
            }

            ctx->buffer_out[ ctx->buffer_used++ ] = 0x8E;
            ctx->buffer_out[ ctx->buffer_used++ ] = cp - 0xFF61 + 0xA1;

            continue;
        }

        if (cp == 0x2212) {
            cp = 0xFF0D;
        }

        hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_jis0208,
                                          PCHTML_ENCODING_MULTI_HASH_JIS0208_SIZE, cp);
        if (hash == NULL) {
            PCHTML_ENCODING_ENCODE_ERROR(ctx);
            continue;
        }

        if ((ctx->buffer_used + 2) > ctx->buffer_length) {
            return PCHTML_STATUS_SMALL_BUFFER;
        }

        ctx->buffer_out[ ctx->buffer_used++ ] = (uint32_t) (uintptr_t) hash->value / 94 + 0xA1;
        ctx->buffer_out[ ctx->buffer_used++ ] = (uint32_t) (uintptr_t) hash->value % 94 + 0xA1;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_encode_euc_kr(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                           const uint32_t *end)
{
    uint32_t cp;
    const pchtml_shs_hash_t *hash;

    for (; *cps < end; (*cps)++) {
        cp = **cps;

        if (cp < 0x80) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, cp);
            continue;
        }

        hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_euc_kr,
                                          PCHTML_ENCODING_MULTI_HASH_EUC_KR_SIZE, cp);
        if (hash == NULL) {
            PCHTML_ENCODING_ENCODE_ERROR(ctx);
            continue;
        }

        if ((ctx->buffer_used + 2) > ctx->buffer_length) {
            return PCHTML_STATUS_SMALL_BUFFER;
        }

        ctx->buffer_out[ ctx->buffer_used++ ] = (uint32_t) (uintptr_t) hash->value / 190 + 0x81;
        ctx->buffer_out[ ctx->buffer_used++ ] = (uint32_t) (uintptr_t) hash->value % 190 + 0x41;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_encode_gbk(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                        const uint32_t *end)
{
    uint32_t cp;
    const pchtml_shs_hash_t *hash;

    for (; *cps < end; (*cps)++) {
        cp = **cps;

        if (cp < 0x80) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, cp);
            continue;
        }

        if (cp == 0xE5E5) {
            PCHTML_ENCODING_ENCODE_ERROR(ctx);
            continue;
        }

        if (cp == 0x20AC) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, 0x80);
            continue;
        }

        hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_gb18030,
                                          PCHTML_ENCODING_MULTI_HASH_GB18030_SIZE, cp);
        if (hash == NULL) {
            PCHTML_ENCODING_ENCODE_ERROR(ctx);
            continue;
        }

        if ((ctx->buffer_used + 2) > ctx->buffer_length) {
            return PCHTML_STATUS_SMALL_BUFFER;
        }

        ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (uintptr_t) hash->value / 190 + 0x81;

        if (((unsigned char) (uintptr_t) hash->value % 190) < 0x3F) {
            ctx->buffer_out[ ctx->buffer_used++ ] = ((unsigned char) (uintptr_t) hash->value % 190) + 0x40;
        }
        else {
            ctx->buffer_out[ ctx->buffer_used++ ] = ((unsigned char) (uintptr_t) hash->value % 190) + 0x41;
        }
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_encode_ibm866(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                           const uint32_t *end)
{

    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_ibm866,
                                    PCHTML_ENCODING_SINGLE_HASH_IBM866_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_2022_jp(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                const uint32_t *end)
{
    int8_t size;
    unsigned state;
    uint32_t cp;
    const pchtml_shs_hash_t *hash;

    size = 0;
    state = ctx->state;

    for (; *cps < end; (*cps)++) {
        cp = **cps;

    begin:

        switch (ctx->state) {
            case PCHTML_ENCODING_ENCODE_2022_JP_ASCII:
                if (cp == 0x000E || cp == 0x000F || cp == 0x001B) {
                    goto failed;
                }

                if (cp < 0x80) {
                    PCHTML_ENCODING_ENCODE_APPEND(ctx, cp);
                    continue;
                }

                if (cp == 0x00A5 || cp == 0x203E) {
                    /*
                     * Do not switch to the ROMAN stage with prepend code point
                     * to stream, add it immediately.
                     */
                    if ((ctx->buffer_used + 4) > ctx->buffer_length) {
                        goto small_buffer;
                    }

                    ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_ROMAN;

                    if (cp == 0x00A5) {
                        memcpy(&ctx->buffer_out[ctx->buffer_used],
                               "\x1B\x28\x4A\x5C", 4);
                        ctx->buffer_used += 4;

                        continue;
                    }

                    memcpy(&ctx->buffer_out[ctx->buffer_used],
                           "\x1B\x28\x4A\x7E", 4);
                    ctx->buffer_used += 4;

                    continue;
                }

                break;

            case PCHTML_ENCODING_ENCODE_2022_JP_ROMAN:
                if (cp == 0x000E || cp == 0x000F || cp == 0x001B) {
                    goto failed;
                }

                if (cp < 0x80) {
                    switch (cp) {
                        case 0x005C:
                        case 0x007E:
                            break;

                        case 0x00A5:
                            PCHTML_ENCODING_ENCODE_APPEND(ctx, 0x5C);
                            continue;

                        case 0x203E:
                            PCHTML_ENCODING_ENCODE_APPEND(ctx, 0x7E);
                            continue;

                        default:
                            PCHTML_ENCODING_ENCODE_APPEND(ctx, cp);
                            continue;
                    }

                    /*
                     * Do not switch to the ANSI stage with prepend code point
                     * to stream, add it immediately.
                     */
                    if ((ctx->buffer_used + 4) > ctx->buffer_length) {
                        goto small_buffer;
                    }

                    ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_ASCII;

                    memcpy(&ctx->buffer_out[ctx->buffer_used], "\x1B\x28\x42", 3);
                    ctx->buffer_used += 3;

                    ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) cp;
                    continue;
                }

                break;

            case PCHTML_ENCODING_ENCODE_2022_JP_JIS0208:
                if (cp < 0x80) {
                    if ((ctx->buffer_used + 4) > ctx->buffer_length) {
                        goto small_buffer;
                    }

                    ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_ASCII;

                    memcpy(&ctx->buffer_out[ctx->buffer_used], "\x1B\x28\x42", 3);
                    ctx->buffer_used += 3;

                    ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) cp;
                    continue;
                }

                if (cp == 0x00A5 || cp == 0x203E) {
                    if ((ctx->buffer_used + 4) > ctx->buffer_length) {
                        goto small_buffer;
                    }

                    ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_ROMAN;

                    if (cp == 0x00A5) {
                        memcpy(&ctx->buffer_out[ctx->buffer_used],
                               "\x1B\x28\x4A\x5C", 4);
                        ctx->buffer_used += 4;

                        continue;
                    }

                    memcpy(&ctx->buffer_out[ctx->buffer_used],
                           "\x1B\x28\x4A\x7E", 4);
                    ctx->buffer_used += 4;

                    continue;
                }

                break;
        }

        if ((ctx->buffer_used + 2) > ctx->buffer_length) {
            goto small_buffer;
        }

        if (cp == 0x2212) {
            cp = 0xFF0D;
        }

        if ((unsigned) (cp - 0xFF61) <= (0xFF9F - 0xFF61)) {
            cp = pchtml_encoding_multi_index_iso_2022_jp_katakana[cp - 0xFF61].codepoint;
        }

        hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_jis0208,
                                          PCHTML_ENCODING_MULTI_HASH_JIS0208_SIZE, cp);
        if (hash == NULL) {
            goto failed;
        }

        if (ctx->state != PCHTML_ENCODING_ENCODE_2022_JP_JIS0208) {
            if ((ctx->buffer_used + 3) > ctx->buffer_length) {
                goto small_buffer;
            }

            memcpy(&ctx->buffer_out[ctx->buffer_used], "\x1B\x24\x42", 3);
            ctx->buffer_used += 3;

            ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_JIS0208;
            size += 3;

            goto begin;
        }

        ctx->buffer_out[ ctx->buffer_used++ ] = (uint32_t) (uintptr_t) hash->value / 94 + 0x21;
        ctx->buffer_out[ ctx->buffer_used++ ] = (uint32_t) (uintptr_t) hash->value % 94 + 0x21;

        continue;

    small_buffer:

        ctx->state = state;
        ctx->buffer_used -= size;

        return PCHTML_STATUS_SMALL_BUFFER;

    failed:

        ctx->buffer_used -= size;
        PCHTML_ENCODING_ENCODE_ERROR(ctx);
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_encode_iso_2022_jp_eof(pchtml_encoding_encode_t *ctx)
{
    if (ctx->state != PCHTML_ENCODING_ENCODE_2022_JP_ASCII) {
        if ((ctx->buffer_used + 3) > ctx->buffer_length) {
            return PCHTML_STATUS_SMALL_BUFFER;
        }

        memcpy(&ctx->buffer_out[ctx->buffer_used], "\x1B\x28\x42", 3);
        ctx->buffer_used += 3;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_encode_iso_8859_10(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_10,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_10_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_13(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_13,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_13_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_14(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_14,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_14_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_15(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_15,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_15_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_16(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_16,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_16_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_2(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                               const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_2,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_2_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_3(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                               const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_3,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_3_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_4(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                               const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_4,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_4_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_5(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                               const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_5,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_5_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_6(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                               const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_6,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_6_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_7(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                               const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_7,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_7_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_8(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                               const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_8,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_8_SIZE);
}

unsigned int
pchtml_encoding_encode_iso_8859_8_i(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_iso_8859_8,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_8_SIZE);
}

unsigned int
pchtml_encoding_encode_koi8_r(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                           const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_koi8_r,
                                    PCHTML_ENCODING_SINGLE_HASH_KOI8_R_SIZE);
}

unsigned int
pchtml_encoding_encode_koi8_u(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                           const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_koi8_u,
                                    PCHTML_ENCODING_SINGLE_HASH_KOI8_U_SIZE);
}

static inline const pchtml_shs_hash_t *
pchtml_encoding_encode_shift_jis_index(uint32_t cp)
{
    const pchtml_shs_hash_t *entry;

    entry = &pchtml_encoding_multi_hash_jis0208[ (cp % PCHTML_ENCODING_MULTI_HASH_JIS0208_SIZE) + 1 ];

    do {
        if (entry->key == cp) {
            if ((unsigned) ((uint32_t) (uintptr_t) entry->value - 8272) > (8835 - 8272)) {
                return entry;
            }
        }

        entry = &pchtml_encoding_multi_hash_jis0208[entry->next];
    }
    while (entry != pchtml_encoding_multi_hash_jis0208);

    return NULL;
}

unsigned int
pchtml_encoding_encode_shift_jis(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                              const uint32_t *end)
{
    uint32_t lead, trail;
    uint32_t cp;
    const pchtml_shs_hash_t *hash;

    for (; *cps < end; (*cps)++) {
        cp = **cps;

        if (cp <= 0x80) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, cp);
            continue;
        }

        if ((unsigned) (cp - 0xFF61) <= (0xFF9F - 0xFF61)) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, cp - 0xFF61 + 0xA1);
            continue;
        }

        switch (cp) {
            case 0x00A5:
                PCHTML_ENCODING_ENCODE_APPEND(ctx, 0x5C);
                continue;

            case 0x203E:
                PCHTML_ENCODING_ENCODE_APPEND(ctx, 0x7E);
                continue;

            case 0x2212:
                cp = 0xFF0D;
                break;
        }

        hash = pchtml_encoding_encode_shift_jis_index(cp);
        if (hash == NULL) {
            PCHTML_ENCODING_ENCODE_ERROR(ctx);
            continue;
        }

        if ((ctx->buffer_used + 2) > ctx->buffer_length) {
            return PCHTML_STATUS_SMALL_BUFFER;
        }

        lead = (uint32_t) (uintptr_t) hash->value / 188;
        trail = (uint32_t) (uintptr_t) hash->value % 188;

        ctx->buffer_out[ctx->buffer_used++ ] = lead + ((lead < 0x1F) ? 0x81 : 0xC1);
        ctx->buffer_out[ctx->buffer_used++ ] = trail + ((trail < 0x3F) ? 0x40 : 0x41);
    }

    return PCHTML_STATUS_OK;
}

static inline void
pchtml_encoding_encode_utf_16_write(pchtml_encoding_encode_t *ctx, bool is_be,
                                 uint32_t cp)
{
    if (is_be) {
        ctx->buffer_out[ctx->buffer_used++] = cp >> 8;
        ctx->buffer_out[ctx->buffer_used++] = cp & 0x00FF;

        return;
    }

    ctx->buffer_out[ctx->buffer_used++] = cp & 0x00FF;
    ctx->buffer_out[ctx->buffer_used++] = cp >> 8;
}

static inline int8_t
pchtml_encoding_encode_utf_16(pchtml_encoding_encode_t *ctx, bool is_be,
                        const uint32_t **cps, const uint32_t *end)
{
    uint32_t cp;

    for (; *cps < end; (*cps)++) {
        cp = **cps;

        if (cp < 0x10000) {
            if ((ctx->buffer_used + 2) > ctx->buffer_length) {
                return PCHTML_STATUS_SMALL_BUFFER;
            }

            pchtml_encoding_encode_utf_16_write(ctx, is_be, cp);

            continue;
        }

        if ((ctx->buffer_used + 4) > ctx->buffer_length) {
            return PCHTML_STATUS_SMALL_BUFFER;
        }

        cp -= 0x10000;

        pchtml_encoding_encode_utf_16_write(ctx, is_be, (0xD800 | (cp >> 0x0A)));
        pchtml_encoding_encode_utf_16_write(ctx, is_be, (0xDC00 | (cp & 0x03FF)));
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_encode_utf_16be(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                             const uint32_t *end)
{
    return pchtml_encoding_encode_utf_16(ctx, true, cps, end);
}

unsigned int
pchtml_encoding_encode_utf_16le(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                             const uint32_t *end)
{
    return pchtml_encoding_encode_utf_16(ctx, false, cps, end);
}

unsigned int
pchtml_encoding_encode_utf_8(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                          const uint32_t *end)
{
    uint32_t cp;
    const uint32_t *p = *cps;

    for (; p < end; p++) {
        cp = *p;

        if (cp < 0x80) {
            if ((ctx->buffer_used + 1) > ctx->buffer_length) {
                *cps = p;

                return PCHTML_STATUS_SMALL_BUFFER;
            }

            /* 0xxxxxxx */
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) cp;
        }
        else if (cp < 0x800) {
            if ((ctx->buffer_used + 2) > ctx->buffer_length) {
                *cps = p;

                return PCHTML_STATUS_SMALL_BUFFER;
            }

            /* 110xxxxx 10xxxxxx */
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (0xC0 | (cp >> 6  ));
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000) {
            if ((ctx->buffer_used + 3) > ctx->buffer_length) {
                *cps = p;

                return PCHTML_STATUS_SMALL_BUFFER;
            }

            /* 1110xxxx 10xxxxxx 10xxxxxx */
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (0xE0 | ((cp >> 12)));
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (0x80 | ((cp >> 6 ) & 0x3F));
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (0x80 | ( cp        & 0x3F));
        }
        else if (cp < 0x110000) {
            if ((ctx->buffer_used + 4) > ctx->buffer_length) {
                *cps = p;

                return PCHTML_STATUS_SMALL_BUFFER;
            }

            /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (0xF0 | ( cp >> 18));
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (0x80 | ((cp >> 12) & 0x3F));
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (0x80 | ((cp >> 6 ) & 0x3F));
            ctx->buffer_out[ ctx->buffer_used++ ] = (unsigned char) (0x80 | ( cp        & 0x3F));
        }
        else {
            *cps = p;
            PCHTML_ENCODING_ENCODE_ERROR(ctx);
        }
    }

    *cps = p;

    return PCHTML_STATUS_OK;
}

static inline uint32_t
pchtml_encoding_encode_gb18030_range(uint32_t cp)
{
    size_t mid, left, right;
    const pchtml_encoding_range_index_t *range;

    if (cp == 0xE7C7) {
        return 7457;
    }

    left = 0;
    right = PCHTML_ENCODING_RANGE_INDEX_GB18030_SIZE;
    range = pchtml_encoding_range_index_gb18030;

    /* Some compilers say about uninitialized mid */
    mid = 0;

    while (left < right) {
        mid = left + (right - left) / 2;

        if (range[mid].codepoint < cp) {
            left = mid + 1;

            if (left < right && range[left].codepoint > cp) {
                break;
            }
        }
        else if (range[mid].codepoint > cp) {
            right = mid - 1;

            if (right > 0 && range[right].codepoint <= cp) {
                mid = right;
                break;
            }
        }
        else {
            break;
        }
    }

    return range[mid].index + cp - range[mid].codepoint;
}

unsigned int
pchtml_encoding_encode_gb18030(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                            const uint32_t *end)
{
    uint32_t index;
    uint32_t cp;
    const pchtml_shs_hash_t *hash;

    for (; *cps < end; (*cps)++) {
        cp = **cps;

        if (cp < 0x80) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, cp);
            continue;
        }

        if (cp == 0xE5E5) {
            PCHTML_ENCODING_ENCODE_ERROR(ctx);
            continue;
        }

        hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_gb18030,
                                          PCHTML_ENCODING_MULTI_HASH_GB18030_SIZE, cp);
        if (hash != NULL) {
            if ((ctx->buffer_used + 2) > ctx->buffer_length) {
                return PCHTML_STATUS_SMALL_BUFFER;
            }

            ctx->buffer_out[ ctx->buffer_used++ ] = (uint32_t) (uintptr_t) hash->value / 190 + 0x81;

            if (((uint32_t) (uintptr_t) hash->value % 190) < 0x3F) {
                ctx->buffer_out[ ctx->buffer_used++ ] = ((uint32_t) (uintptr_t) hash->value % 190) + 0x40;
            }
            else {
                ctx->buffer_out[ ctx->buffer_used++ ] = ((uint32_t) (uintptr_t) hash->value % 190) + 0x41;
            }

            continue;
        }

        if ((ctx->buffer_used + 4) > ctx->buffer_length) {
            return PCHTML_STATUS_SMALL_BUFFER;
        }

        index = pchtml_encoding_encode_gb18030_range(cp);

        ctx->buffer_out[ ctx->buffer_used++ ] = (index / (10 * 126 * 10)) + 0x81;
        ctx->buffer_out[ ctx->buffer_used++ ] = ((index % (10 * 126 * 10)) / (10 * 126)) + 0x30;

        index = (index % (10 * 126 * 10)) % (10 * 126);

        ctx->buffer_out[ ctx->buffer_used++ ] = (index / 10) + 0x81;
        ctx->buffer_out[ ctx->buffer_used++ ] = (index % 10) + 0x30;
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_encode_macintosh(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                              const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_macintosh,
                                    PCHTML_ENCODING_SINGLE_HASH_MACINTOSH_SIZE);
}

unsigned int
pchtml_encoding_encode_replacement(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                const uint32_t *end)
{
    UNUSED_PARAM(ctx);

    *cps = end;
    return PCHTML_STATUS_ERROR;
}

unsigned int
pchtml_encoding_encode_windows_1250(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    UNUSED_PARAM(ctx);

    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_1250,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1250_SIZE);
}

unsigned int
pchtml_encoding_encode_windows_1251(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_1251,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1251_SIZE);
}

unsigned int
pchtml_encoding_encode_windows_1252(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_1252,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1252_SIZE);
}

unsigned int
pchtml_encoding_encode_windows_1253(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_1253,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1253_SIZE);
}

unsigned int
pchtml_encoding_encode_windows_1254(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_1254,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1254_SIZE);
}

unsigned int
pchtml_encoding_encode_windows_1255(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_1255,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1255_SIZE);
}

unsigned int
pchtml_encoding_encode_windows_1256(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_1256,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1256_SIZE);
}

unsigned int
pchtml_encoding_encode_windows_1257(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_1257,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1257_SIZE);
}

unsigned int
pchtml_encoding_encode_windows_1258(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                 const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_1258,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1258_SIZE);
}

unsigned int
pchtml_encoding_encode_windows_874(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_windows_874,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_874_SIZE);
}

unsigned int
pchtml_encoding_encode_x_mac_cyrillic(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                   const uint32_t *end)
{
    PCHTML_ENCODING_ENCODE_SINGLE_BYTE(pchtml_encoding_single_hash_x_mac_cyrillic,
                                  PCHTML_ENCODING_SINGLE_HASH_X_MAC_CYRILLIC_SIZE);
}

unsigned int
pchtml_encoding_encode_x_user_defined(pchtml_encoding_encode_t *ctx, const uint32_t **cps,
                                   const uint32_t *end)
{
    uint32_t cp;

    for (; *cps < end; (*cps)++) {
        cp = **cps;

        if (cp < 0x80) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, cp);
        }
        else if (cp >= 0xF780 && cp <= 0xF7FF) {
            PCHTML_ENCODING_ENCODE_APPEND(ctx, (cp - 0xF780 + 0x80));
        }
        else {
            PCHTML_ENCODING_ENCODE_ERROR(ctx);
        }
    }

    return PCHTML_STATUS_OK;
}

/*
 * Single
 */
int8_t
pchtml_encoding_encode_default_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                   const unsigned char *end, uint32_t cp)
{
    return pchtml_encoding_encode_utf_8_single(ctx, data, end, cp);
}

int8_t
pchtml_encoding_encode_auto_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(data);
    UNUSED_PARAM(end);
    UNUSED_PARAM(cp);

    return PCHTML_ENCODING_ENCODE_ERROR;
}

int8_t
pchtml_encoding_encode_undefined_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                     const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(data);
    UNUSED_PARAM(end);
    UNUSED_PARAM(cp);

    return PCHTML_ENCODING_ENCODE_ERROR;
}

int8_t
pchtml_encoding_encode_big5_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);

    const pchtml_shs_hash_t *hash;

    if (cp < 0x80) {
        *(*data)++ = (unsigned char) cp;

        return 1;
    }

    hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_big5,
                                      PCHTML_ENCODING_MULTI_HASH_BIG5_SIZE, cp);
    if (hash == NULL) {
        return PCHTML_ENCODING_ENCODE_ERROR;
    }

    if ((*data + 2) > end) {
        return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
    }

    *(*data)++ = ((uint32_t) (uintptr_t) hash->value) / 157 + 0x81;

    if ((((uint32_t) (uintptr_t) hash->value) % 157) < 0x3F) {
        *(*data)++ = (((uint32_t) (uintptr_t) hash->value) % 157) + 0x40;
    }
    else {
        *(*data)++ = (((uint32_t) (uintptr_t) hash->value) % 157) + 0x62;
    }

    return 2;
}

int8_t
pchtml_encoding_encode_euc_jp_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                  const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);

    const pchtml_shs_hash_t *hash;

    if (cp < 0x80) {
        *(*data)++ = (unsigned char) cp;

        return 1;
    }

    if (cp == 0x00A5) {
        *(*data)++ = 0x5C;

        return 1;
    }

    if (cp == 0x203E) {
        *(*data)++ = 0x7E;

        return 1;
    }

    if ((*data + 2) > end) {
        return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
    }

    if ((unsigned) (cp - 0xFF61) <= (0xFF9F - 0xFF61)) {
        *(*data)++ = 0x8E;
        *(*data)++ = cp - 0xFF61 + 0xA1;

        return 2;
    }

    if (cp == 0x2212) {
        cp = 0xFF0D;
    }

    hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_jis0208,
                                      PCHTML_ENCODING_MULTI_HASH_JIS0208_SIZE, cp);
    if (hash == NULL) {
        return PCHTML_ENCODING_ENCODE_ERROR;
    }

    *(*data)++ = (uint32_t) (uintptr_t) hash->value / 94 + 0xA1;
    *(*data)++ = (uint32_t) (uintptr_t) hash->value % 94 + 0xA1;

    return 2;
}

int8_t
pchtml_encoding_encode_euc_kr_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                  const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);

    const pchtml_shs_hash_t *hash;

    if (cp < 0x80) {
        *(*data)++ = (unsigned char) cp;

        return 1;
    }

    if ((*data + 2) > end) {
        return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
    }

    hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_euc_kr,
                                      PCHTML_ENCODING_MULTI_HASH_EUC_KR_SIZE, cp);
    if (hash == NULL) {
        return PCHTML_ENCODING_ENCODE_ERROR;
    }

    *(*data)++ = (uint32_t) (uintptr_t) hash->value / 190 + 0x81;
    *(*data)++ = (uint32_t) (uintptr_t) hash->value % 190 + 0x41;

    return 2;
}

int8_t
pchtml_encoding_encode_gbk_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                               const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);

    const pchtml_shs_hash_t *hash;

    if (cp < 0x80) {
        *(*data)++ = (unsigned char) cp;

        return 1;
    }

    if (cp == 0xE5E5) {
        return PCHTML_ENCODING_ENCODE_ERROR;
    }

    if (cp == 0x20AC) {
        *(*data)++ = 0x80;

        return 1;
    }

    hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_gb18030,
                                      PCHTML_ENCODING_MULTI_HASH_GB18030_SIZE, cp);
    if (hash != NULL) {
        if ((*data + 2) > end) {
            return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
        }

        *(*data)++ = (unsigned char) (uintptr_t) hash->value / 190 + 0x81;

        if (((unsigned char) (uintptr_t) hash->value % 190) < 0x3F) {
            *(*data)++ = ((unsigned char) (uintptr_t) hash->value % 190) + 0x40;
        }
        else {
            *(*data)++ = ((unsigned char) (uintptr_t) hash->value % 190) + 0x41;
        }

        return 2;
    }

    return PCHTML_ENCODING_ENCODE_ERROR;
}

int8_t
pchtml_encoding_encode_ibm866_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                  const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_ibm866,
                                    PCHTML_ENCODING_SINGLE_HASH_IBM866_SIZE);
}

int8_t
pchtml_encoding_encode_iso_2022_jp_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                       const unsigned char *end, uint32_t cp)
{
    int8_t size;
    unsigned state;
    const pchtml_shs_hash_t *hash;

    size = 0;
    state = ctx->state;

begin:

    switch (ctx->state) {
        case PCHTML_ENCODING_ENCODE_2022_JP_ASCII:
            if (cp == 0x000E || cp == 0x000F || cp == 0x001B) {
                goto failed;
            }

            if (cp < 0x80) {
                *(*data)++ = (unsigned char) cp;

                return size + 1;
            }

            if (cp == 0x00A5 || cp == 0x203E) {
                /*
                 * Do not switch to the ROMAN stage with prepend code point
                 * to stream, add it immediately.
                 */
                if ((*data + 4) > end) {
                    goto small_buffer;
                }

                ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_ROMAN;

                if (cp == 0x00A5) {
                    memcpy(*data, "\x1B\x28\x4A\x5C", 4);
                    *data = *data + 4;

                    return size + 4;
                }

                memcpy(*data, "\x1B\x28\x4A\x7E", 4);
                *data = *data + 4;

                return size + 4;
            }

            break;

        case PCHTML_ENCODING_ENCODE_2022_JP_ROMAN:
            if (cp == 0x000E || cp == 0x000F || cp == 0x001B) {
                goto failed;
            }

            if (cp < 0x80) {
                switch (cp) {
                    case 0x005C:
                    case 0x007E:
                        break;

                    case 0x00A5:
                        *(*data)++ = 0x5C;
                        return size + 1;

                    case 0x203E:
                        *(*data)++ = 0x7E;
                        return size + 1;

                    default:
                        *(*data)++ = (unsigned char) cp;
                        return size + 1;
                }

                /*
                 * Do not switch to the ANSI stage with prepend code point
                 * to stream, add it immediately.
                 */
                if ((*data + 4) > end) {
                    goto small_buffer;
                }

                ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_ASCII;

                memcpy(*data, "\x1B\x28\x42", 3);
                *data = *data + 3;

                *(*data)++ = (unsigned char) cp;

                return size + 4;
            }

            break;

        case PCHTML_ENCODING_ENCODE_2022_JP_JIS0208:
            if (cp < 0x80) {
                if ((*data + 4) > end) {
                    goto small_buffer;
                }

                ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_ASCII;

                memcpy(*data, "\x1B\x28\x42", 3);
                *data = *data + 3;

                *(*data)++ = (unsigned char) cp;

                return size + 4;
            }

            if (cp == 0x00A5 || cp == 0x203E) {
                if ((*data + 4) > end) {
                    goto small_buffer;
                }

                ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_ROMAN;

                if (cp == 0x00A5) {
                    memcpy(*data, "\x1B\x28\x4A\x5C", 4);
                    *data = *data + 4;

                    return size + 4;
                }

                memcpy(*data, "\x1B\x28\x4A\x7E", 4);
                *data = *data + 4;

                return size + 4;
            }

            break;
    }

    if ((*data + 2) > end) {
        goto small_buffer;
    }

    if (cp == 0x2212) {
        cp = 0xFF0D;
    }

    if ((unsigned) (cp - 0xFF61) <= (0xFF9F - 0xFF61)) {
        cp = pchtml_encoding_multi_index_iso_2022_jp_katakana[cp - 0xFF61].codepoint;
    }

    hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_jis0208,
                                      PCHTML_ENCODING_MULTI_HASH_JIS0208_SIZE, cp);
    if (hash == NULL) {
        goto failed;
    }

    if (ctx->state != PCHTML_ENCODING_ENCODE_2022_JP_JIS0208) {
        if ((*data + 3) > end) {
            goto small_buffer;
        }

        memcpy(*data, "\x1B\x24\x42", 3);
        *data = *data + 3;

        ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_JIS0208;
        size += 3;

        goto begin;
    }

    *(*data)++ = (uint32_t) (uintptr_t) hash->value / 94 + 0x21;
    *(*data)++ = (uint32_t) (uintptr_t) hash->value % 94 + 0x21;

    return size + 2;

small_buffer:

    ctx->state = state;
    *data = *data - size;

    return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;

failed:

    *data = *data - size;

    return PCHTML_ENCODING_ENCODE_ERROR;
}

int8_t
pchtml_encoding_encode_iso_2022_jp_eof_single(pchtml_encoding_encode_t *ctx,
                                       unsigned char **data, const unsigned char *end)
{
    if (ctx->state != PCHTML_ENCODING_ENCODE_2022_JP_ASCII) {
        if ((*data + 3) > end) {
            return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
        }

        memcpy(*data, "\x1B\x28\x42", 3);
        *data = *data + 3;

        ctx->state = PCHTML_ENCODING_ENCODE_2022_JP_ASCII;

        return 3;
    }

    return 0;
}

int8_t
pchtml_encoding_encode_iso_8859_10_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                       const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_10,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_10_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_13_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                       const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_13,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_13_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_14_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                       const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_14,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_14_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_15_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                       const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_15,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_15_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_16_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                       const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_16,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_16_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_2_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                      const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_2,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_2_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_3_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                      const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_3,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_3_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_4_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                      const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_4,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_4_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_5_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                      const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_5,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_5_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_6_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                      const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_6,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_6_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_7_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                      const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_7,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_7_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_8_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                      const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_8,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_8_SIZE);
}

int8_t
pchtml_encoding_encode_iso_8859_8_i_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_iso_8859_8,
                                    PCHTML_ENCODING_SINGLE_HASH_ISO_8859_8_SIZE);
}

int8_t
pchtml_encoding_encode_koi8_r_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                  const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_koi8_r,
                                    PCHTML_ENCODING_SINGLE_HASH_KOI8_R_SIZE);
}

int8_t
pchtml_encoding_encode_koi8_u_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                  const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_koi8_u,
                                    PCHTML_ENCODING_SINGLE_HASH_KOI8_U_SIZE);
}

int8_t
pchtml_encoding_encode_shift_jis_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                     const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);

    uint32_t lead, trail;
    const pchtml_shs_hash_t *hash;

    if (cp <= 0x80) {
        *(*data)++ = (unsigned char) cp;

        return 1;
    }

    if ((unsigned) (cp - 0xFF61) <= (0xFF9F - 0xFF61)) {
        *(*data)++ = cp - 0xFF61 + 0xA1;

        return 1;
    }

    switch (cp) {
        case 0x00A5:
            *(*data)++ = 0x5C;
            return 1;

        case 0x203E:
            *(*data)++ = 0x7E;
            return 1;

        case 0x2212:
            cp = 0xFF0D;
            break;
    }

    hash = pchtml_encoding_encode_shift_jis_index(cp);
    if (hash == NULL) {
        return PCHTML_ENCODING_ENCODE_ERROR;
    }

    if ((*data + 2) > end) {
        return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
    }

    lead = (uint32_t) (uintptr_t) hash->value / 188;
    trail = (uint32_t) (uintptr_t) hash->value % 188;

    *(*data)++ = lead + ((lead < 0x1F) ? 0x81 : 0xC1);
    *(*data)++ = trail + ((trail < 0x3F) ? 0x40 : 0x41);

    return 2;
}

static inline void
pchtml_encoding_encode_utf_16_write_single(bool is_be, unsigned char **data,
                                        uint32_t cp)
{
    if (is_be) {
        *(*data)++ = cp >> 8;
        *(*data)++ = cp & 0x00FF;

        return;
    }

    *(*data)++ = cp & 0x00FF;
    *(*data)++ = cp >> 8;
}

static inline int8_t
pchtml_encoding_encode_utf_16_single(pchtml_encoding_encode_t *ctx, bool is_be,
                   unsigned char **data, const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);

    if ((*data + 2) > end) {
        return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
    }

    if (cp < 0x10000) {
        pchtml_encoding_encode_utf_16_write_single(is_be, data, cp);

        return 2;
    }

    if ((*data + 4) > end) {
        return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
    }

    cp -= 0x10000;

    pchtml_encoding_encode_utf_16_write_single(is_be, data, (0xD800 | (cp >> 0x0A)));
    pchtml_encoding_encode_utf_16_write_single(is_be, data, (0xDC00 | (cp & 0x03FF)));

    return 4;
}

int8_t
pchtml_encoding_encode_utf_16be_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                    const unsigned char *end, uint32_t cp)
{
    return pchtml_encoding_encode_utf_16_single(ctx, true, data, end, cp);
}

int8_t
pchtml_encoding_encode_utf_16le_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                    const unsigned char *end, uint32_t cp)
{
    return pchtml_encoding_encode_utf_16_single(ctx, false, data, end, cp);
}

int8_t
pchtml_encoding_encode_utf_8_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                 const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);

    if (cp < 0x80) {
        /* 0xxxxxxx */
        *(*data)++ = (unsigned char) cp;

        return 1;
    }

    if (cp < 0x800) {
        if ((*data + 2) > end) {
            return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
        }

        /* 110xxxxx 10xxxxxx */
        *(*data)++ = (unsigned char) (0xC0 | (cp >> 6  ));
        *(*data)++ = (unsigned char) (0x80 | (cp & 0x3F));

        return 2;
    }

    if (cp < 0x10000) {
        if ((*data + 3) > end) {
            return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
        }

        /* 1110xxxx 10xxxxxx 10xxxxxx */
        *(*data)++ = (unsigned char) (0xE0 | ((cp >> 12)));
        *(*data)++ = (unsigned char) (0x80 | ((cp >> 6 ) & 0x3F));
        *(*data)++ = (unsigned char) (0x80 | ( cp        & 0x3F));

        return 3;
    }

    if (cp < 0x110000) {
        if ((*data + 4) > end) {
            return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
        }

        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        *(*data)++ = (unsigned char) (0xF0 | ( cp >> 18));
        *(*data)++ = (unsigned char) (0x80 | ((cp >> 12) & 0x3F));
        *(*data)++ = (unsigned char) (0x80 | ((cp >> 6 ) & 0x3F));
        *(*data)++ = (unsigned char) (0x80 | ( cp        & 0x3F));

        return 4;
    }

    return PCHTML_ENCODING_ENCODE_ERROR;
}

int8_t
pchtml_encoding_encode_gb18030_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                   const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);

    uint32_t index;
    const pchtml_shs_hash_t *hash;

    if (cp < 0x80) {
        *(*data)++ = (unsigned char) cp;

        return 1;
    }

    if (cp == 0xE5E5) {
        return PCHTML_ENCODING_ENCODE_ERROR;
    }

    hash = pchtml_shs_hash_get_static(pchtml_encoding_multi_hash_gb18030,
                                      PCHTML_ENCODING_MULTI_HASH_GB18030_SIZE, cp);
    if (hash != NULL) {
        if ((*data + 2) > end) {
            return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
        }

        *(*data)++ = (uint32_t) (uintptr_t) hash->value / 190 + 0x81;

        if (((uint32_t) (uintptr_t) hash->value % 190) < 0x3F) {
            *(*data)++ = ((uint32_t) (uintptr_t) hash->value % 190) + 0x40;
        }
        else {
            *(*data)++ = ((uint32_t) (uintptr_t) hash->value % 190) + 0x41;
        }

        return 2;
    }

    if ((*data + 4) > end) {
        return PCHTML_ENCODING_ENCODE_SMALL_BUFFER;
    }

    index = pchtml_encoding_encode_gb18030_range(cp);

    *(*data)++ = (index / (10 * 126 * 10)) + 0x81;
    *(*data)++ = ((index % (10 * 126 * 10)) / (10 * 126)) + 0x30;

    index = (index % (10 * 126 * 10)) % (10 * 126);

    *(*data)++ = (index / 10) + 0x81;
    *(*data)++ = (index % 10) + 0x30;

    return 4;
}

int8_t
pchtml_encoding_encode_macintosh_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                     const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_macintosh,
                                    PCHTML_ENCODING_SINGLE_HASH_MACINTOSH_SIZE);
}

int8_t
pchtml_encoding_encode_replacement_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                       const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);
    UNUSED_PARAM(cp);

    (*data)++;
    return PCHTML_ENCODING_ENCODE_ERROR;
}

int8_t
pchtml_encoding_encode_windows_1250_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_1250,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1250_SIZE);
}

int8_t
pchtml_encoding_encode_windows_1251_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_1251,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1251_SIZE);
}

int8_t
pchtml_encoding_encode_windows_1252_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_1252,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1252_SIZE);
}

int8_t
pchtml_encoding_encode_windows_1253_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_1253,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1253_SIZE);
}

int8_t
pchtml_encoding_encode_windows_1254_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_1254,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1254_SIZE);
}

int8_t
pchtml_encoding_encode_windows_1255_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_1255,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1255_SIZE);
}

int8_t
pchtml_encoding_encode_windows_1256_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_1256,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1256_SIZE);
}

int8_t
pchtml_encoding_encode_windows_1257_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_1257,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1257_SIZE);
}

int8_t
pchtml_encoding_encode_windows_1258_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                        const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_1258,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_1258_SIZE);
}

int8_t
pchtml_encoding_encode_windows_874_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                       const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_windows_874,
                                    PCHTML_ENCODING_SINGLE_HASH_WINDOWS_874_SIZE);
}

int8_t
pchtml_encoding_encode_x_mac_cyrillic_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                          const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    PCHTML_ENCODING_ENCODE_BYTE_SINGLE(pchtml_encoding_single_hash_x_mac_cyrillic,
                                  PCHTML_ENCODING_SINGLE_HASH_X_MAC_CYRILLIC_SIZE);
}

int8_t
pchtml_encoding_encode_x_user_defined_single(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                          const unsigned char *end, uint32_t cp)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (cp < 0x80) {
        *(*data)++ = (unsigned char) cp;

        return 1;
    }

    if (cp >= 0xF780 && cp <= 0xF7FF) {
        *(*data)++ = (unsigned char) (cp - 0xF780 + 0x80);

        return 1;
    }

    return PCHTML_ENCODING_ENCODE_ERROR;
}
