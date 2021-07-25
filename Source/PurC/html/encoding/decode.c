/**
 * @file decode.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of decode.
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

#include "html/encoding/decode.h"
#include "html/encoding/single.h"
#include "html/encoding/multi.h"
#include "html/encoding/range.h"


#define PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY(_lower, _upper, _cont)              \
    {                                                                          \
        ch = *p;                                                               \
                                                                               \
        if (ch < _lower || ch > _upper) {                                      \
            ctx->u.utf_8.lower = 0x00;                                         \
            ctx->u.utf_8.need = 0;                                             \
                                                                               \
            PCHTML_ENCODING_DECODE_ERROR_BEGIN {                                  \
                *data = p;                                                     \
                ctx->have_error = true;                                        \
            }                                                                  \
            PCHTML_ENCODING_DECODE_ERROR_END();                                   \
                                                                               \
            _cont;                                                             \
        }                                                                      \
        else {                                                                 \
            p++;                                                               \
            need--;                                                            \
            ctx->codepoint = (ctx->codepoint << 6) | (ch & 0x3F);              \
        }                                                                      \
    }

#define PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SET(first, two, f_lower, s_upper)   \
    do {                                                                       \
        if (ch == first) {                                                     \
            ctx->u.utf_8.lower = f_lower;                                      \
            ctx->u.utf_8.upper = 0xBF;                                         \
        }                                                                      \
        else if (ch == two) {                                                  \
            ctx->u.utf_8.lower = 0x80;                                         \
            ctx->u.utf_8.upper = s_upper;                                      \
        }                                                                      \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, cp)                           \
    do {                                                                       \
        (ctx)->buffer_out[(ctx)->buffer_used++] = (cp);                        \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_APPEND(ctx, cp)                                    \
    do {                                                                       \
        if ((ctx)->buffer_used >= (ctx)->buffer_length) {                      \
            return PCHTML_STATUS_SMALL_BUFFER;                                    \
        }                                                                      \
                                                                               \
        (ctx)->buffer_out[(ctx)->buffer_used++] = (cp);                        \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_APPEND_P(ctx, cp)                                  \
    do {                                                                       \
        if ((ctx)->buffer_used >= (ctx)->buffer_length) {                      \
            *data = p;                                                         \
            return PCHTML_STATUS_SMALL_BUFFER;                                    \
        }                                                                      \
                                                                               \
        (ctx)->buffer_out[(ctx)->buffer_used++] = (cp);                        \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_CHECK_OUT(ctx)                                     \
    do {                                                                       \
        if ((ctx)->buffer_used >= (ctx)->buffer_length) {                      \
            return PCHTML_STATUS_SMALL_BUFFER;                                    \
        }                                                                      \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_ERROR_BEGIN                                        \
    do {                                                                       \
        if (ctx->replace_to == NULL) {                                         \
            return PCHTML_STATUS_ERROR;                                           \
        }                                                                      \
                                                                               \
        if ((ctx->buffer_used + ctx->replace_len) > ctx->buffer_length) {      \
            do

#define PCHTML_ENCODING_DECODE_ERROR_END()                                        \
            while (0);                                                         \
                                                                               \
            return PCHTML_STATUS_SMALL_BUFFER;                                    \
        }                                                                      \
                                                                               \
        memcpy(&ctx->buffer_out[ctx->buffer_used], ctx->replace_to,            \
               sizeof(uint32_t) * ctx->replace_len);                    \
                                                                               \
        ctx->buffer_used += ctx->replace_len;                                  \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_ERROR(ctx)                                         \
    do {                                                                       \
        PCHTML_ENCODING_DECODE_ERROR_BEGIN {                                      \
        } PCHTML_ENCODING_DECODE_ERROR_END();                                     \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_FAILED(ident)                                      \
    do {                                                                       \
        if ((byte) < (0x80)) {                                                 \
            (*data)--;                                                         \
        }                                                                      \
                                                                               \
        PCHTML_ENCODING_DECODE_ERROR_BEGIN {                                      \
            ctx->have_error = true;                                            \
            (ident) = 0x01;                                                    \
        }                                                                      \
        PCHTML_ENCODING_DECODE_ERROR_END();                                       \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_SINGLE(decode_map)                                 \
    do {                                                                       \
        const unsigned char *p = *data;                                           \
                                                                               \
        while (p < end) {                                                      \
            if (*p < 0x80) {                                                   \
                PCHTML_ENCODING_DECODE_APPEND_P(ctx, *p++);                       \
            }                                                                  \
            else {                                                             \
                ctx->codepoint = decode_map[(*p++) - 0x80].codepoint;          \
                if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {          \
                    PCHTML_ENCODING_DECODE_ERROR_BEGIN {                          \
                        *data = p - 1;                                         \
                    }                                                          \
                    PCHTML_ENCODING_DECODE_ERROR_END();                           \
                    continue;                                                  \
                }                                                              \
                                                                               \
                PCHTML_ENCODING_DECODE_APPEND_P(ctx, ctx->codepoint);             \
            }                                                                  \
                                                                               \
            *data = p;                                                         \
        }                                                                      \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SINGLE(lower, upper)                \
    do {                                                                       \
        ch = **data;                                                           \
                                                                               \
        if (ch < lower || ch > upper) {                                        \
            goto failed;                                                       \
        }                                                                      \
                                                                               \
        (*data)++;                                                             \
        needed--;                                                              \
        ctx->codepoint = (ctx->codepoint << 6) | (ch & 0x3F);                  \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SET_SINGLE(first, two, f_lower,     \
                                                      s_upper)                 \
    do {                                                                       \
        if (ch == first) {                                                     \
            ctx->u.utf_8.lower = f_lower;                                      \
            ctx->u.utf_8.upper = 0xBF;                                         \
        }                                                                      \
        else if (ch == two) {                                                  \
            ctx->u.utf_8.lower = 0x80;                                         \
            ctx->u.utf_8.upper = s_upper;                                      \
        }                                                                      \
    }                                                                          \
    while (0)


unsigned int
pchtml_encoding_decode_default(pchtml_encoding_decode_t *ctx,
                            const unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_decode_utf_8(ctx, data, end);
}

unsigned int
pchtml_encoding_decode_auto(pchtml_encoding_decode_t *ctx,
                         const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);

    *data = end;
    return PCHTML_STATUS_ERROR;
}

unsigned int
pchtml_encoding_decode_undefined(pchtml_encoding_decode_t *ctx,
                              const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);

    *data = end;
    return PCHTML_STATUS_ERROR;
}

unsigned int
pchtml_encoding_decode_big5(pchtml_encoding_decode_t *ctx,
                         const unsigned char **data, const unsigned char *end)
{
    uint32_t index;
    unsigned char lead, byte;

    ctx->status = PCHTML_STATUS_OK;

    if (ctx->u.lead != 0x00) {
        if (ctx->have_error) {
            ctx->u.lead = 0x00;
            ctx->have_error = false;

            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                ctx->u.lead = 0x01;
                ctx->have_error = true;
            } PCHTML_ENCODING_DECODE_ERROR_END();
        }
        else if (ctx->second_codepoint != 0x0000) {
            if ((ctx->buffer_used + 2) > ctx->buffer_length) {
                return PCHTML_STATUS_SMALL_BUFFER;
            }

            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->u.lead);
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->second_codepoint);

            ctx->u.lead = 0x00;
            ctx->second_codepoint = 0x0000;
        }
        else {
            if (*data >= end) {
                ctx->status = PCHTML_STATUS_CONTINUE;

                return PCHTML_STATUS_CONTINUE;
            }

            PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

            lead = (unsigned char) ctx->u.lead;
            ctx->u.lead = 0x00;

            goto lead_state;
        }
    }

    while (*data < end) {
        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        lead = *(*data)++;

        if (lead < 0x80) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, lead);
            continue;
        }

        if ((unsigned) (lead - 0x81) > (0xFE - 0x81)) {
            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                (*data)--;
            }
            PCHTML_ENCODING_DECODE_ERROR_END();

            continue;
        }

        if (*data >= end) {
            ctx->u.lead = lead;
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

    lead_state:

        index = 0;
        byte = *(*data)++;

        if ((unsigned) (byte - 0x40) <= (0x7E - 0x40)
            || (unsigned) (byte - 0xA1) <= (0xFE - 0xA1))
        {
            if (byte < 0x7F) {
                /* Max index == (0xFE - 0x81) * 157 + (0x7E - 0x62) == 19653 */
                index = (lead - 0x81) * 157 + (byte - 0x40);
            }
            else {
                /* Max index == (0xFE - 0x81) * 157 + (0xFE - 0x62) == 19781 */
                index = (lead - 0x81) * 157 + (byte - 0x62);
            }
        }

        /*
         * 1133 U+00CA U+0304  Ê̄ (LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND MACRON)
         * 1135 U+00CA U+030C  Ê̌ (LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND CARON)
         * 1164 U+00EA U+0304  ê̄ (LATIN SMALL LETTER E WITH CIRCUMFLEX AND MACRON)
         * 1166 U+00EA U+030C  ê̌ (LATIN SMALL LETTER E WITH CIRCUMFLEX AND CARON)
         */
        switch (index) {
            case 1133:
                if ((ctx->buffer_used + 2) > ctx->buffer_length) {
                    ctx->u.lead = 0x00CA;
                    ctx->second_codepoint = 0x0304;

                    return PCHTML_STATUS_SMALL_BUFFER;
                }

                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x00CA);
                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x0304);

                continue;

            case 1135:
                if ((ctx->buffer_used + 2) > ctx->buffer_length) {
                    ctx->u.lead = 0x00CA;
                    ctx->second_codepoint = 0x030C;

                    return PCHTML_STATUS_SMALL_BUFFER;
                }

                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x00CA);
                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x030C);

                continue;

            case 1164:
                if ((ctx->buffer_used + 2) > ctx->buffer_length) {
                    ctx->u.lead = 0x00EA;
                    ctx->second_codepoint = 0x0304;

                    return PCHTML_STATUS_SMALL_BUFFER;
                }

                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x00EA);
                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x0304);

                continue;

            case 1166:
                if ((ctx->buffer_used + 2) > ctx->buffer_length) {
                    ctx->u.lead = 0x00EA;
                    ctx->second_codepoint = 0x030C;

                    return PCHTML_STATUS_SMALL_BUFFER;
                }

                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x00EA);
                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x030C);

                continue;

            case 0:
                PCHTML_ENCODING_DECODE_FAILED(ctx->u.lead);
                continue;
        }

        ctx->codepoint = pchtml_encoding_multi_index_big5[index].codepoint;
        if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
            PCHTML_ENCODING_DECODE_FAILED(ctx->u.lead);
            continue;
        }

        PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->codepoint);
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_euc_jp(pchtml_encoding_decode_t *ctx,
                           const unsigned char **data, const unsigned char *end)
{
    bool is_jis0212;
    unsigned char byte, lead;

    ctx->status = PCHTML_STATUS_OK;

    if (ctx->u.euc_jp.lead != 0x00) {
        if (ctx->have_error) {
            ctx->have_error = false;
            ctx->u.euc_jp.lead = 0x00;

            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                ctx->have_error = true;
                ctx->u.euc_jp.lead = 0x01;
            } PCHTML_ENCODING_DECODE_ERROR_END();
        }
        else {
            if (*data >= end) {
                ctx->status = PCHTML_STATUS_CONTINUE;

                return PCHTML_STATUS_CONTINUE;
            }

            PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

            lead = ctx->u.euc_jp.lead;
            byte = *(*data)++;

            ctx->u.euc_jp.lead = 0x00;

            if (ctx->u.euc_jp.is_jis0212) {
                is_jis0212 = true;
                ctx->u.euc_jp.is_jis0212 = false;

                goto lead_jis_state;
            }

            goto lead_state;
        }
    }

    while (*data < end) {
        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        lead = *(*data)++;

        if (lead < 0x80) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, lead);
            continue;
        }

        if ((unsigned) (lead - 0xA1) > (0xFE - 0xA1)
            && (lead != 0x8E && lead != 0x8F))
        {
            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                (*data)--;
            }
            PCHTML_ENCODING_DECODE_ERROR_END();

            continue;
        }

        if (*data >= end) {
            ctx->u.euc_jp.lead = lead;
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

        byte = *(*data)++;

    lead_state:

        if (lead == 0x8E && (unsigned) (byte - 0xA1) <= (0xDF - 0xA1)) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0xFF61 - 0xA1 + byte);
            continue;
        }

        is_jis0212 = false;

        if (lead == 0x8F && (unsigned) (byte - 0xA1) <= (0xFE - 0xA1)) {
            if (*data >= end) {
                ctx->u.euc_jp.lead = byte;
                ctx->u.euc_jp.is_jis0212 = true;

                ctx->status = PCHTML_STATUS_CONTINUE;

                return PCHTML_STATUS_CONTINUE;
            }

            lead = byte;
            byte = *(*data)++;
            is_jis0212 = true;
        }

    lead_jis_state:

        if ((unsigned) (lead - 0xA1) > (0xFE - 0xA1)
            || (unsigned) (byte - 0xA1) > (0xFE - 0xA1))
        {
            PCHTML_ENCODING_DECODE_FAILED(ctx->u.euc_jp.lead);
            continue;
        }

        /* Max index == (0xFE - 0xA1) * 94 + 0xFE - 0xA1 == 8835 */
        ctx->codepoint = (lead - 0xA1) * 94 + byte - 0xA1;

        if (is_jis0212) {
            if ((sizeof(pchtml_encoding_multi_index_jis0212)
                 / sizeof(pchtml_encoding_multi_index_t)) <= ctx->codepoint)
            {
                PCHTML_ENCODING_DECODE_FAILED(ctx->u.euc_jp.lead);
                continue;
            }

            ctx->codepoint = pchtml_encoding_multi_index_jis0212[ctx->codepoint].codepoint;
        }
        else {
            if ((sizeof(pchtml_encoding_multi_index_jis0208)
                 / sizeof(pchtml_encoding_multi_index_t)) <= ctx->codepoint)
            {
                PCHTML_ENCODING_DECODE_FAILED(ctx->u.euc_jp.lead);
                continue;
            }

            ctx->codepoint = pchtml_encoding_multi_index_jis0208[ctx->codepoint].codepoint;
        }

        if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
            PCHTML_ENCODING_DECODE_FAILED(ctx->u.euc_jp.lead);
            continue;
        }

        PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->codepoint);
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_euc_kr(pchtml_encoding_decode_t *ctx,
                           const unsigned char **data, const unsigned char *end)
{
    unsigned char lead, byte;

    ctx->status = PCHTML_STATUS_OK;

    if (ctx->u.lead != 0x00) {
        if (ctx->have_error) {
            ctx->have_error = false;
            ctx->u.lead = 0x00;

            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                ctx->have_error = true;
                ctx->u.lead = 0x01;
            } PCHTML_ENCODING_DECODE_ERROR_END();
        }
        else {
            if (*data >= end) {
                ctx->status = PCHTML_STATUS_CONTINUE;

                return PCHTML_STATUS_CONTINUE;
            }

            PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

            lead = (unsigned char) ctx->u.lead;
            ctx->u.lead = 0x00;

            goto lead_state;
        }
    }

    while (*data < end) {
        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        lead = *(*data)++;

        if (lead < 0x80) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, lead);
            continue;
        }

        if ((unsigned) (lead - 0x81) > (0xFE - 0x81)) {
            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                (*data)--;
            }
            PCHTML_ENCODING_DECODE_ERROR_END();

            continue;
        }

        if (*data == end) {
            ctx->u.lead = lead;
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

    lead_state:

        byte = *(*data)++;

        if ((unsigned) (byte - 0x41) > (0xFE - 0x41)) {
            PCHTML_ENCODING_DECODE_FAILED(ctx->u.lead);
            continue;
        }

        /* Max index == (0xFE - 0x81) * 190 + (0xFE - 0x41) == 23939 */
        ctx->codepoint = (lead - 0x81) * 190 + (byte - 0x41);

        if (ctx->codepoint >= sizeof(pchtml_encoding_multi_index_euc_kr)
                              / sizeof(pchtml_encoding_multi_index_t))
        {
            PCHTML_ENCODING_DECODE_FAILED(ctx->u.lead);
            continue;
        }

        ctx->codepoint = pchtml_encoding_multi_index_euc_kr[ctx->codepoint].codepoint;
        if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
            PCHTML_ENCODING_DECODE_FAILED(ctx->u.lead);
            continue;
        }

        PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->codepoint);
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_gbk(pchtml_encoding_decode_t *ctx,
                        const unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_decode_gb18030(ctx, data, end);
}

unsigned int
pchtml_encoding_decode_ibm866(pchtml_encoding_decode_t *ctx,
                           const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_ibm866);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_2022_jp(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
#define PCHTML_ENCODING_DECODE_ISO_2022_JP_OK()                                   \
    do {                                                                       \
        if (*data >= end) {                                                    \
            return PCHTML_STATUS_OK;                                              \
        }                                                                      \
    }                                                                          \
    while (0)

#define PCHTML_ENCODING_DECODE_ISO_2022_JP_CONTINUE()                             \
    do {                                                                       \
        if (*data >= end) {                                                    \
            ctx->status = PCHTML_STATUS_CONTINUE;                                 \
            return PCHTML_STATUS_CONTINUE;                                        \
        }                                                                      \
    }                                                                          \
    while (0)


    unsigned char byte;
    pchtml_encoding_ctx_2022_jp_t *iso = &ctx->u.iso_2022_jp;

    ctx->status = PCHTML_STATUS_OK;

    if (ctx->have_error) {
        ctx->have_error = false;

        PCHTML_ENCODING_DECODE_ERROR_BEGIN {
            ctx->have_error = true;
        }
        PCHTML_ENCODING_DECODE_ERROR_END();
    }

    if (iso->prepand != 0x00) {
        if (*data >= end) {
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        byte = iso->prepand;
        iso->prepand = 0x00;

        goto prepand;
    }

    if (*data >= end) {
        return PCHTML_STATUS_OK;
    }

    do {
        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        byte = *(*data)++;

    prepand:

        switch (iso->state) {
            case PCHTML_ENCODING_DECODE_2022_JP_ASCII:
                if (byte == 0x1B) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                    PCHTML_ENCODING_DECODE_ISO_2022_JP_CONTINUE();
                    break;
                }

                /* 0x00 to 0x7F, excluding 0x0E, 0x0F, and 0x1B */
                if ((unsigned) (byte - 0x00) <= (0x7F - 0x00)
                    && byte != 0x0E && byte != 0x0F)
                {
                    iso->out_flag = false;

                    PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, byte);
                    PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();
                    break;
                }

                iso->out_flag = false;

                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();
                break;

            case PCHTML_ENCODING_DECODE_2022_JP_ROMAN:
                switch (byte) {
                    case 0x1B:
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                        PCHTML_ENCODING_DECODE_ISO_2022_JP_CONTINUE();
                        continue;

                    case 0x5C:
                        iso->out_flag = false;

                        PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x00A5);
                        PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();

                        continue;

                    case 0x7E:
                        iso->out_flag = false;

                        PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x203E);
                        PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();

                        continue;

                    case 0x0E:
                    case 0x0F:
                        break;

                    default:
                        /* 0x00 to 0x7F */
                        if ((unsigned) (byte - 0x00) <= (0x7F - 0x00)) {
                            iso->out_flag = false;

                            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, byte);
                            PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();

                            continue;
                        }

                        break;
                }

                iso->out_flag = false;

                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();
                break;

            case PCHTML_ENCODING_DECODE_2022_JP_KATAKANA:
                if (byte == 0x1B) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                    PCHTML_ENCODING_DECODE_ISO_2022_JP_CONTINUE();
                    break;
                }

                /* 0x21 to 0x5F */
                if ((unsigned) (byte - 0x21) <= (0x5F - 0x21)) {
                    iso->out_flag = false;

                    PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx,
                                                        0xFF61 - 0x21 + byte);
                    PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();
                    break;
                }

                iso->out_flag = false;

                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();
                break;

            case PCHTML_ENCODING_DECODE_2022_JP_LEAD:
                if (byte == 0x1B) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                    PCHTML_ENCODING_DECODE_ISO_2022_JP_CONTINUE();
                    break;
                }

                /* 0x21 to 0x7E */
                if ((unsigned) (byte - 0x21) <= (0x7E - 0x21)) {
                    iso->out_flag = false;
                    iso->lead = byte;
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_TRAIL;

                    PCHTML_ENCODING_DECODE_ISO_2022_JP_CONTINUE();
                    break;
                }

                iso->out_flag = false;

                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();
                break;

            case PCHTML_ENCODING_DECODE_2022_JP_TRAIL:
                if (byte == 0x1B) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                    PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                        ctx->have_error = true;
                    }
                    PCHTML_ENCODING_DECODE_ERROR_END();

                    PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();
                    break;
                }

                iso->state = PCHTML_ENCODING_DECODE_2022_JP_LEAD;

                /* 0x21 to 0x7E */
                if ((unsigned) (byte - 0x21) <= (0x7E - 0x21)) {
                    /* Max index == (0x7E - 0x21) * 94 + 0x7E - 0x21 == 8835 */
                    ctx->codepoint = (iso->lead - 0x21) * 94 + byte - 0x21;

                    ctx->codepoint = pchtml_encoding_multi_index_jis0208[ctx->codepoint].codepoint;

                    if (ctx->codepoint != PCHTML_ENCODING_ERROR_CODEPOINT) {
                        PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->codepoint);
                        PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();

                        break;
                    }
                }

                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    iso->prepand = 0x01;
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();
                break;

            case PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START:
                if (byte == 0x24 || byte == 0x28) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE;
                    iso->lead = byte;

                    PCHTML_ENCODING_DECODE_ISO_2022_JP_CONTINUE();
                    break;
                }

                (*data)--;

                iso->out_flag = false;
                iso->state = ctx->u.iso_2022_jp.out_state;

                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    iso->prepand = 0x01;
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                break;

            case PCHTML_ENCODING_DECODE_2022_JP_ESCAPE:
                iso->state = PCHTML_ENCODING_DECODE_2022_JP_UNSET;

                if (iso->lead == 0x28) {
                    if (byte == 0x42) {
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_ASCII;
                    }
                    else if (byte == 0x4A) {
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_ROMAN;
                    }
                    else if (byte == 0x49) {
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_KATAKANA;
                    }
                }
                else if (iso->lead == 0x24) {
                    if (byte == 0x40 || byte == 0x42) {
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_LEAD;
                    }
                }

                if (iso->state == PCHTML_ENCODING_DECODE_2022_JP_UNSET) {
                    (*data)--;

                    iso->out_flag = false;
                    iso->state = iso->out_state;

                    PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                        iso->prepand = iso->lead;
                        iso->lead = 0x00;

                        ctx->have_error = true;
                    }
                    PCHTML_ENCODING_DECODE_ERROR_END();

                    byte = iso->lead;
                    iso->lead = 0x00;

                    goto prepand;
                }

                iso->lead = 0x00;
                iso->out_state = iso->state;

                if (iso->out_flag) {
                    PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                        ctx->have_error = true;
                    }
                    PCHTML_ENCODING_DECODE_ERROR_END();

                    PCHTML_ENCODING_DECODE_ISO_2022_JP_OK();
                    break;
                }

                iso->out_flag = true;

                PCHTML_ENCODING_DECODE_ISO_2022_JP_CONTINUE();
                break;
        }
    }
    while (true);

    return PCHTML_STATUS_OK;

#undef PCHTML_ENCODING_DECODE_ISO_2022_JP_OK
#undef PCHTML_ENCODING_DECODE_ISO_2022_JP_CONTINUE
}

unsigned int
pchtml_encoding_decode_iso_8859_10(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_10);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_13(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_13);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_14(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_14);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_15(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_15);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_16(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_16);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_2(pchtml_encoding_decode_t *ctx,
                               const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_2);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_3(pchtml_encoding_decode_t *ctx,
                               const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_3);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_4(pchtml_encoding_decode_t *ctx,
                               const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_4);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_5(pchtml_encoding_decode_t *ctx,
                               const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_5);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_6(pchtml_encoding_decode_t *ctx,
                               const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_6);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_7(pchtml_encoding_decode_t *ctx,
                               const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_7);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_8(pchtml_encoding_decode_t *ctx,
                               const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_8);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_iso_8859_8_i(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_iso_8859_8);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_koi8_r(pchtml_encoding_decode_t *ctx,
                           const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_koi8_r);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_koi8_u(pchtml_encoding_decode_t *ctx,
                           const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_koi8_u);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_shift_jis(pchtml_encoding_decode_t *ctx,
                              const unsigned char **data, const unsigned char *end)
{
    unsigned char byte, lead;

    ctx->status = PCHTML_STATUS_OK;

    if (ctx->u.lead != 0x00) {
        if (ctx->have_error) {
            ctx->have_error = false;
            ctx->u.lead = 0x00;

            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                ctx->have_error = true;
                ctx->u.lead = 0x01;
            } PCHTML_ENCODING_DECODE_ERROR_END();
        }
        else {
            if (*data >= end) {
                ctx->status = PCHTML_STATUS_CONTINUE;

                return PCHTML_STATUS_CONTINUE;
            }

            PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

            lead = (unsigned char) ctx->u.lead;
            ctx->u.lead = 0x00;

            goto lead_state;
        }
    }

    while (*data < end) {
        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        lead = *(*data)++;

        if (lead <= 0x80) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, lead);
            continue;
        }

        if ((unsigned) (lead - 0xA1) <= (0xDF - 0xA1)) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0xFF61 - 0xA1 + lead);
            continue;
        }

        if ((unsigned) (lead - 0x81) > (0x9F - 0x81)
            && lead != 0xE0 && lead != 0xFC)
        {
            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                (*data)--;
            }
            PCHTML_ENCODING_DECODE_ERROR_END();

            continue;
        }

        if (*data >= end) {
            ctx->u.lead = lead;
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

    lead_state:

        byte = *(*data)++;

        if (byte < 0x7F) {
            ctx->codepoint = 0x40;
        }
        else {
            ctx->codepoint = 0x41;
        }

        if (lead < 0xA0) {
            ctx->second_codepoint = 0x81;
        }
        else {
            ctx->second_codepoint = 0xC1;
        }

        if ((unsigned) (byte - 0x40) > (0x7E - 0x40)
            && (unsigned) (byte - 0x80) > (0xFC - 0x80))
        {
            PCHTML_ENCODING_DECODE_FAILED(ctx->u.lead);
            continue;
        }

        /* Max index == (0xFC - 0xC1) * 188 + 0xFC - 0x41 = 11279 */
        ctx->codepoint = (lead - ctx->second_codepoint) * 188
                          + byte - ctx->codepoint;

        if (ctx->codepoint >= (sizeof(pchtml_encoding_multi_index_jis0208)
                               / sizeof(pchtml_encoding_multi_index_t)))
        {
            PCHTML_ENCODING_DECODE_FAILED(ctx->u.lead);
            continue;
        }

        if ((unsigned) (ctx->codepoint - 8836) <= (10715 - 8836)) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0xE000 - 8836 + ctx->codepoint);
            continue;
        }

        ctx->codepoint = pchtml_encoding_multi_index_jis0208[ctx->codepoint].codepoint;
        if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
            PCHTML_ENCODING_DECODE_FAILED(ctx->u.lead);
            continue;
        }

        PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->codepoint);
    }

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_encoding_decode_utf_16(pchtml_encoding_decode_t *ctx, bool is_be,
                           const unsigned char **data, const unsigned char *end)
{
    unsigned lead;
    uint32_t unit;

    ctx->status = PCHTML_STATUS_OK;

    if (ctx->have_error) {
        ctx->have_error = false;

        PCHTML_ENCODING_DECODE_ERROR_BEGIN {
            ctx->have_error = true;
        }
        PCHTML_ENCODING_DECODE_ERROR_END();
    }

    if (ctx->u.lead != 0x00) {
        if (*data >= end) {
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        lead = ctx->u.lead - 0x01;
        ctx->u.lead = 0x00;

        goto lead_state;
    }

    while (*data < end) {
        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

    pair_state:

        lead = *(*data)++;

        if (*data >= end) {
            ctx->u.lead = lead + 0x01;
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

    lead_state:

        /* For UTF-16BE or UTF-16LE */
        if (is_be) {
            unit = (lead << 8) + *(*data)++;
        }
        else {
            unit = (*(*data)++ << 8) + lead;
        }

        if (ctx->second_codepoint != 0x00) {
            if ((unsigned) (unit - 0xDC00) <= (0xDFFF - 0xDC00)) {
                ctx->codepoint = 0x10000 + ((ctx->second_codepoint - 0xD800) << 10)
                                 + (unit - 0xDC00);

                ctx->second_codepoint = 0x00;

                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->codepoint);
                continue;
            }

            (*data)--;

            ctx->second_codepoint = 0x00;

            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                ctx->have_error = true;

                ctx->u.lead = lead + 0x01;
            }
            PCHTML_ENCODING_DECODE_ERROR_END();

            goto lead_state;
        }

        /* Surrogate pair */
        if ((unsigned) (unit - 0xD800) <= (0xDFFF - 0xD800)) {
            if ((unsigned) (unit - 0xDC00) <= (0xDFFF - 0xDC00)) {
                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                continue;
            }

            ctx->second_codepoint = unit;

            if (*data >= end) {
                ctx->status = PCHTML_STATUS_CONTINUE;

                return PCHTML_STATUS_CONTINUE;
            }

            goto pair_state;
        }

        PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, unit);
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_utf_16be(pchtml_encoding_decode_t *ctx,
                             const unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_decode_utf_16(ctx, true, data, end);
}

unsigned int
pchtml_encoding_decode_utf_16le(pchtml_encoding_decode_t *ctx,
                             const unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_decode_utf_16(ctx, false, data, end);
}

unsigned int
pchtml_encoding_decode_utf_8(pchtml_encoding_decode_t *ctx,
                          const unsigned char **data, const unsigned char *end)
{
    unsigned need;
    unsigned char ch;
    const unsigned char *p = *data;

    ctx->status = PCHTML_STATUS_OK;

    if (ctx->have_error) {
        ctx->have_error = false;

        PCHTML_ENCODING_DECODE_ERROR_BEGIN {
            ctx->have_error = true;
        }
        PCHTML_ENCODING_DECODE_ERROR_END();
    }

    if (ctx->u.utf_8.need != 0) {
        if (p >= end) {
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        need = ctx->u.utf_8.need;
        ctx->u.utf_8.need = 0;

        if (ctx->u.utf_8.lower != 0x00) {
            PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY(ctx->u.utf_8.lower,
                                               ctx->u.utf_8.upper, goto begin);
            ctx->u.utf_8.lower = 0x00;
        }

        goto decode;
    }

begin:

    while (p < end) {
        if (ctx->buffer_used >= ctx->buffer_length) {
            *data = p;

            return PCHTML_STATUS_SMALL_BUFFER;
        }

        ch = *p++;

        if (ch < 0x80) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ch);
            continue;
        }
        else if (ch <= 0xDF) {
            if (ch < 0xC2) {
                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    *data = p - 1;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                continue;
            }

            need = 1;
            ctx->codepoint = ch & 0x1F;
        }
        else if (ch < 0xF0) {
            need = 2;
            ctx->codepoint = ch & 0x0F;

            if (p == end) {
                PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SET(0xE0, 0xED, 0xA0, 0x9F);

                *data = p;

                ctx->u.utf_8.need = need;
                ctx->status = PCHTML_STATUS_CONTINUE;

                return PCHTML_STATUS_CONTINUE;
            }

            if (ch == 0xE0) {
                PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY(0xA0, 0xBF, continue);
            }
            else if (ch == 0xED) {
                PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY(0x80, 0x9F, continue);
            }
        }
        else if (ch < 0xF5) {
            need = 3;
            ctx->codepoint = ch & 0x07;

            if (p == end) {
                PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SET(0xF0, 0xF4, 0x90, 0x8F);

                *data = p;

                ctx->u.utf_8.need = need;
                ctx->status = PCHTML_STATUS_CONTINUE;

                return PCHTML_STATUS_CONTINUE;
            }

            if (ch == 0xF0) {
                PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY(0x90, 0xBF, continue);
            }
            else if (ch == 0xF4) {
                PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY(0x80, 0x8F, continue);
            }
        }
        else {
            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                *data = p - 1;
            }
            PCHTML_ENCODING_DECODE_ERROR_END();

            continue;
        }

    decode:

        do {
            if (p >= end) {
                *data = p;

                ctx->u.utf_8.need = need;
                ctx->status = PCHTML_STATUS_CONTINUE;

                return PCHTML_STATUS_CONTINUE;
            }

            ch = *p++;

            if (ch < 0x80 || ch > 0xBF) {
                p--;

                ctx->u.utf_8.need = 0;

                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    *data = p;
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                break;
            }

            ctx->codepoint = (ctx->codepoint << 6) | (ch & 0x3F);

            if (--need == 0) {
                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->codepoint);

                break;
            }
        }
        while (true);
    }

    *data = p;

    return PCHTML_STATUS_OK;
}

static inline uint32_t
pchtml_encoding_decode_gb18030_range(uint32_t index)
{
    size_t mid, left, right;
    const pchtml_encoding_range_index_t *range;

    /*
     * Pointer greater than 39419 and less than 189000,
     * or pointer is greater than 1237575
     */
    if ((unsigned) (index - 39419) < (189000 - 39419)
        || index > 1237575)
    {
        return PCHTML_ENCODING_ERROR_CODEPOINT;
    }

    if (index == 7457) {
        return 0xE7C7;
    }

    left = 0;
    right = PCHTML_ENCODING_RANGE_INDEX_GB18030_SIZE;
    range = pchtml_encoding_range_index_gb18030;

    /* Some compilers say about uninitialized mid */
    mid = 0;

    while (left < right) {
        mid = left + (right - left) / 2;

        if (range[mid].index < index) {
            left = mid + 1;

            if (left < right && range[ left ].index > index) {
                break;
            }
        }
        else if (range[mid].index > index) {
            right = mid - 1;

            if (right > 0 && range[right].index <= index) {
                mid = right;
                break;
            }
        }
        else {
            break;
        }
    }

    return range[mid].codepoint + index - range[mid].index;
}

unsigned int
pchtml_encoding_decode_gb18030(pchtml_encoding_decode_t *ctx,
                            const unsigned char **data, const unsigned char *end)
{
    uint32_t pointer;
    unsigned char first, second, third, offset;

    /* Make compiler happy */
    second = 0x00;

    ctx->status = PCHTML_STATUS_OK;

    if (ctx->have_error) {
        ctx->have_error = false;

        PCHTML_ENCODING_DECODE_ERROR_BEGIN {
            ctx->have_error = true;
        }
        PCHTML_ENCODING_DECODE_ERROR_END();
    }

    if (ctx->u.gb18030.first != 0) {
        if (*data >= end) {
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        if (ctx->u.gb18030.third != 0x00) {
            first = ctx->u.gb18030.first;
            second = ctx->u.gb18030.second;
            third = ctx->u.gb18030.third;

            memset(&ctx->u.gb18030, 0, sizeof(pchtml_encoding_ctx_gb18030_t));

            if (ctx->prepend) {
                /* The first is always < 0x80 */
                PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, second);

                if (ctx->buffer_used == ctx->buffer_length) {
                    ctx->u.gb18030.first = third;

                    return PCHTML_STATUS_SMALL_BUFFER;
                }

                first = third;
                ctx->prepend = false;

                goto prepend_first;
            }

            goto third_state;
        }
        else if (ctx->u.gb18030.second != 0x00) {
            first = ctx->u.gb18030.first;
            second = ctx->u.gb18030.second;

            memset(&ctx->u.gb18030, 0, sizeof(pchtml_encoding_ctx_gb18030_t));

            goto second_state;
        }

        first = ctx->u.gb18030.first;
        ctx->u.gb18030.first = 0x00;

        if (ctx->prepend) {
            ctx->prepend = false;
            goto prepend_first;
        }

        goto first_state;
    }

    while (*data < end) {
        PCHTML_ENCODING_DECODE_CHECK_OUT(ctx);

        first = *(*data)++;

    prepend_first:

        if (first < 0x80) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, first);
            continue;
        }

        if (first == 0x80) {
            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, 0x20AC);
            continue;
        }

        /* Range 0x81 to 0xFE, inclusive */
        if ((unsigned) (first - 0x81) > (0xFE - 0x81)) {
            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                (*data)--;
            }
            PCHTML_ENCODING_DECODE_ERROR_END();

            continue;
        }

        if (*data == end) {
            ctx->u.gb18030.first = first;
            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

        /* First */
    first_state:

        second = *(*data)++;

        /* Range 0x30 to 0x39, inclusive */
        if ((unsigned) (second - 0x30) > (0x39 - 0x30)) {
            offset = (second < 0x7F) ? 0x40 : 0x41;

            /* Range 0x40 to 0x7E, inclusive, or 0x80 to 0xFE, inclusive */
            if ((unsigned) (second - 0x40) <= (0x7E - 0x40)
                || (unsigned) (second - 0x80) <= (0xFE - 0x80))
            {
                pointer = (first - 0x81) * 190 + (second - offset);
            }
            else {
                if (second < 0x80) {
                    (*data)--;
                }

                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                continue;
            }

            /* Max pointer value == (0xFE - 0x81) * 190 + (0xFE - 0x41) == 23939 */
            ctx->codepoint = pchtml_encoding_multi_index_gb18030[pointer].codepoint;
            if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
                if (second < 0x80) {
                    (*data)--;
                }

                PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                    ctx->have_error = true;
                }
                PCHTML_ENCODING_DECODE_ERROR_END();

                continue;
            }

            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->codepoint);
            continue;
        }

        if (*data == end) {
            ctx->u.gb18030.first = first;
            ctx->u.gb18030.second = second;

            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

        /* Second */
    second_state:

        third = *(*data)++;

        /* Range 0x81 to 0xFE, inclusive */
        if ((unsigned) (third - 0x81) > (0xFE - 0x81)) {
            (*data)--;

            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                ctx->prepend = true;
                ctx->have_error = true;
                ctx->u.gb18030.first = second;
            }
            PCHTML_ENCODING_DECODE_ERROR_END();

            first = second;

            goto prepend_first;
        }

        if (*data == end) {
            ctx->u.gb18030.first = first;
            ctx->u.gb18030.second = second;
            ctx->u.gb18030.third = third;

            ctx->status = PCHTML_STATUS_CONTINUE;

            return PCHTML_STATUS_CONTINUE;
        }

        /* Third */
    third_state:

        /* Range 0x30 to 0x39, inclusive */
        if ((unsigned) (**data - 0x30) > (0x39 - 0x30)) {
            ctx->prepend = true;

            PCHTML_ENCODING_DECODE_ERROR_BEGIN {
                ctx->prepend = true;
                ctx->have_error = true;

                /* First is a fake for trigger */
                ctx->u.gb18030.first = 0x01;
                ctx->u.gb18030.second = second;
                ctx->u.gb18030.third = third;
            }
            PCHTML_ENCODING_DECODE_ERROR_END();

            PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, second);

            if (ctx->buffer_used == ctx->buffer_length) {
                ctx->prepend = true;
                ctx->have_error = true;

                /* First is a fake for trigger */
                ctx->u.gb18030.first = 0x01;
                ctx->u.gb18030.second = second;
                ctx->u.gb18030.third = third;

                return PCHTML_STATUS_SMALL_BUFFER;
            }

            first = third;

            goto prepend_first;
        }

        pointer = ((first  - 0x81) * (10 * 126 * 10))
                + ((second - 0x30) * (10 * 126))
                + ((third  - 0x81) * 10) + (*(*data)++) - 0x30;

        ctx->codepoint =  pchtml_encoding_decode_gb18030_range(pointer);

        if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
            PCHTML_ENCODING_DECODE_ERROR_BEGIN {}
            PCHTML_ENCODING_DECODE_ERROR_END();

            continue;
        }

        PCHTML_ENCODING_DECODE_APPEND_WO_CHECK(ctx, ctx->codepoint);
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_macintosh(pchtml_encoding_decode_t *ctx,
                              const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_macintosh);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_replacement(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);

    *data = end;
    return PCHTML_STATUS_ERROR;
}

unsigned int
pchtml_encoding_decode_windows_1250(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_1250);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_windows_1251(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_1251);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_windows_1252(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_1252);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_windows_1253(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_1253);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_windows_1254(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_1254);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_windows_1255(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_1255);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_windows_1256(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_1256);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_windows_1257(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_1257);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_windows_1258(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_1258);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_windows_874(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_windows_874);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_x_mac_cyrillic(pchtml_encoding_decode_t *ctx,
                                   const unsigned char **data, const unsigned char *end)
{
    PCHTML_ENCODING_DECODE_SINGLE(pchtml_encoding_single_index_x_mac_cyrillic);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_encoding_decode_x_user_defined(pchtml_encoding_decode_t *ctx,
                                   const unsigned char **data, const unsigned char *end)
{
    while (*data < end) {
        if (**data < 0x80) {
            PCHTML_ENCODING_DECODE_APPEND(ctx,  *(*data)++);
        }
        else {
            PCHTML_ENCODING_DECODE_APPEND(ctx,  0xF780 + (*(*data)++) - 0x80);
        }
    }

    return PCHTML_STATUS_OK;
}

/*
 * Single
 */
uint32_t
pchtml_encoding_decode_default_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_decode_utf_8_single(ctx, data, end);
}

uint32_t
pchtml_encoding_decode_auto_single(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(data);
    UNUSED_PARAM(end);

    return PCHTML_ENCODING_DECODE_ERROR;
}

uint32_t
pchtml_encoding_decode_undefined_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(data);
    UNUSED_PARAM(end);

    return PCHTML_ENCODING_DECODE_ERROR;
}

uint32_t
pchtml_encoding_decode_big5_single(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end)
{
    uint32_t index;
    unsigned char lead, byte;

    if (ctx->u.lead != 0x00) {
        if (ctx->second_codepoint != 0x00) {
            (*data)++;

            ctx->u.lead = 0x00;

            ctx->codepoint = ctx->second_codepoint;
            ctx->second_codepoint = 0x00;

            return ctx->codepoint;
        }

        lead = (unsigned char) ctx->u.lead;
        ctx->u.lead = 0x00;

        goto lead_state;
    }

    lead = *(*data)++;

    if (lead < 0x80) {
        return lead;
    }

    if ((unsigned) (lead - 0x81) > (0xFE - 0x81)) {
        return PCHTML_ENCODING_DECODE_ERROR;
    }

    if (*data >= end) {
        ctx->u.lead = lead;

        return PCHTML_ENCODING_DECODE_CONTINUE;
    }

lead_state:

    index = 0;
    byte = **data;

    if ((unsigned) (byte - 0x40) <= (0x7E - 0x40)
        || (unsigned) (byte - 0xA1) <= (0xFE - 0xA1))
    {
        if (byte < 0x7F) {
            /* Max index == (0xFE - 0x81) * 157 + (0x7E - 0x62) == 19653 */
            index = (lead - 0x81) * 157 + (byte - 0x40);
        }
        else {
            /* Max index == (0xFE - 0x81) * 157 + (0xFE - 0x62) == 19781 */
            index = (lead - 0x81) * 157 + (byte - 0x62);
        }
    }

    /*
     * 1133 U+00CA U+0304  Ê̄ (LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND MACRON)
     * 1135 U+00CA U+030C  Ê̌ (LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND CARON)
     * 1164 U+00EA U+0304  ê̄ (LATIN SMALL LETTER E WITH CIRCUMFLEX AND MACRON)
     * 1166 U+00EA U+030C  ê̌ (LATIN SMALL LETTER E WITH CIRCUMFLEX AND CARON)
     */
    switch (index) {
        case 1133:
            ctx->u.lead = lead;
            ctx->second_codepoint = 0x0304;
            return 0x00CA;

        case 1135:
            ctx->u.lead = lead;
            ctx->second_codepoint = 0x030C;
            return 0x00CA;

        case 1164:
            ctx->u.lead = lead;
            ctx->second_codepoint = 0x0304;
            return 0x00EA;

        case 1166:
            ctx->u.lead = lead;
            ctx->second_codepoint = 0x030C;
            return 0x00EA;

        case 0:
            goto failed;
    }

    ctx->codepoint = pchtml_encoding_multi_index_big5[index].codepoint;
    if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
        goto failed;
    }

    (*data)++;

    return ctx->codepoint;

failed:

    if (byte >= 0x80) {
        (*data)++;
    }

    return PCHTML_ENCODING_DECODE_ERROR;
}

uint32_t
pchtml_encoding_decode_euc_jp_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    bool is_jis0212;
    unsigned char byte, lead;

    if (ctx->u.euc_jp.lead != 0x00) {
        lead = ctx->u.euc_jp.lead;
        byte = *(*data)++;

        ctx->u.euc_jp.lead = 0x00;

        if (ctx->u.euc_jp.is_jis0212) {
            is_jis0212 = true;
            ctx->u.euc_jp.is_jis0212 = false;

            goto lead_jis_state;
        }

        goto lead_state;
    }

    lead = *(*data)++;

    if (lead < 0x80) {
        return lead;
    }

    if ((unsigned) (lead - 0xA1) > (0xFE - 0xA1)
        && (lead != 0x8E && lead != 0x8F))
    {
        return PCHTML_ENCODING_DECODE_ERROR;
    }

    if (*data >= end) {
        ctx->u.euc_jp.lead = lead;
        return PCHTML_ENCODING_DECODE_CONTINUE;
    }

    byte = *(*data)++;

lead_state:

    if (lead == 0x8E && (unsigned) (byte - 0xA1) <= (0xDF - 0xA1)) {
        return 0xFF61 - 0xA1 + byte;
    }

    is_jis0212 = false;

    if (lead == 0x8F && (unsigned) (byte - 0xA1) <= (0xFE - 0xA1)) {
        if (*data >= end) {
            ctx->u.euc_jp.lead = byte;
            ctx->u.euc_jp.is_jis0212 = true;

            return PCHTML_ENCODING_DECODE_CONTINUE;
        }

        lead = byte;
        byte = *(*data)++;
        is_jis0212 = true;
    }

lead_jis_state:

    if ((unsigned) (lead - 0xA1) > (0xFE - 0xA1)
        || (unsigned) (byte - 0xA1) > (0xFE - 0xA1))
    {
        goto failed;
    }

    /* Max index == (0xFE - 0xA1) * 94 + 0xFE - 0xA1 == 8835 */
    ctx->codepoint = (lead - 0xA1) * 94 + byte - 0xA1;

    if (is_jis0212) {
        if ((sizeof(pchtml_encoding_multi_index_jis0212)
             / sizeof(pchtml_encoding_multi_index_t)) <= ctx->codepoint)
        {
            goto failed;
        }

        ctx->codepoint = pchtml_encoding_multi_index_jis0212[ctx->codepoint].codepoint;
    }
    else {
        if ((sizeof(pchtml_encoding_multi_index_jis0208)
             / sizeof(pchtml_encoding_multi_index_t)) <= ctx->codepoint)
        {
            goto failed;
        }

        ctx->codepoint = pchtml_encoding_multi_index_jis0208[ctx->codepoint].codepoint;
    }

    if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
        goto failed;
    }

    return ctx->codepoint;

failed:

    if (byte < 0x80) {
        (*data)--;
    }

    return PCHTML_ENCODING_DECODE_ERROR;
}

uint32_t
pchtml_encoding_decode_euc_kr_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    unsigned char lead, byte;

    if (ctx->u.lead != 0x00) {
        lead = (unsigned char) ctx->u.lead;
        ctx->u.lead = 0x00;

        goto lead_state;
    }

    lead = *(*data)++;

    if (lead < 0x80) {
        return lead;
    }

    if ((unsigned) (lead - 0x81) > (0xFE - 0x81)) {
        return PCHTML_ENCODING_DECODE_ERROR;
    }

    if (*data == end) {
        ctx->u.lead = lead;
        return PCHTML_ENCODING_DECODE_CONTINUE;
    }

lead_state:

    byte = *(*data)++;

    if ((unsigned) (byte - 0x41) > (0xFE - 0x41)) {
        goto failed;
    }

    /* Max index == (0xFE - 0x81) * 190 + (0xFE - 0x41) == 23939 */
    ctx->codepoint = (lead - 0x81) * 190 + (byte - 0x41);

    if (ctx->codepoint >= sizeof(pchtml_encoding_multi_index_euc_kr)
                          / sizeof(pchtml_encoding_multi_index_t))
    {
        goto failed;
    }

    ctx->codepoint = pchtml_encoding_multi_index_euc_kr[ctx->codepoint].codepoint;
    if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
        goto failed;
    }

    return ctx->codepoint;

failed:

    if (byte < 0x80) {
        (*data)--;
    }

    return PCHTML_ENCODING_DECODE_ERROR;
}

uint32_t
pchtml_encoding_decode_gbk_single(pchtml_encoding_decode_t *ctx,
                               const unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_decode_gb18030_single(ctx, data, end);
}

uint32_t
pchtml_encoding_decode_ibm866_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_ibm866[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_2022_jp_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    unsigned char byte;
    pchtml_encoding_ctx_2022_jp_t *iso = &ctx->u.iso_2022_jp;

    if (iso->prepand != 0x00) {
        byte = iso->prepand;
        iso->prepand = 0x00;

        goto prepand;
    }

    do {
        byte = *(*data)++;

    prepand:

        switch (iso->state) {
            case PCHTML_ENCODING_DECODE_2022_JP_ASCII:
                if (byte == 0x1B) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                    break;
                }

                /* 0x00 to 0x7F, excluding 0x0E, 0x0F, and 0x1B */
                if ((unsigned) (byte - 0x00) <= (0x7F - 0x00)
                    && byte != 0x0E && byte != 0x0F)
                {
                    iso->out_flag = false;

                    return byte;
                }

                iso->out_flag = false;

                return PCHTML_ENCODING_DECODE_ERROR;

            case PCHTML_ENCODING_DECODE_2022_JP_ROMAN:
                switch (byte) {
                    case 0x1B:
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                        continue;

                    case 0x5C:
                        iso->out_flag = false;

                        return 0x00A5;

                    case 0x7E:
                        iso->out_flag = false;

                        return 0x203E;

                    case 0x0E:
                    case 0x0F:
                        break;

                    default:
                        /* 0x00 to 0x7F */
                        if ((unsigned) (byte - 0x00) <= (0x7F - 0x00)) {
                            iso->out_flag = false;

                            return byte;
                        }

                        break;
                }

                iso->out_flag = false;

                return PCHTML_ENCODING_DECODE_ERROR;

            case PCHTML_ENCODING_DECODE_2022_JP_KATAKANA:
                if (byte == 0x1B) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                    break;
                }

                /* 0x21 to 0x5F */
                if ((unsigned) (byte - 0x21) <= (0x5F - 0x21)) {
                    iso->out_flag = false;

                    return 0xFF61 - 0x21 + byte;
                }

                iso->out_flag = false;

                return PCHTML_ENCODING_DECODE_ERROR;

            case PCHTML_ENCODING_DECODE_2022_JP_LEAD:
                if (byte == 0x1B) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                    break;
                }

                /* 0x21 to 0x7E */
                if ((unsigned) (byte - 0x21) <= (0x7E - 0x21)) {
                    iso->out_flag = false;
                    iso->lead = byte;
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_TRAIL;

                    break;
                }

                iso->out_flag = false;

                return PCHTML_ENCODING_DECODE_ERROR;

            case PCHTML_ENCODING_DECODE_2022_JP_TRAIL:
                if (byte == 0x1B) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START;

                    return PCHTML_ENCODING_DECODE_ERROR;
                }

                iso->state = PCHTML_ENCODING_DECODE_2022_JP_LEAD;

                /* 0x21 to 0x7E */
                if ((unsigned) (byte - 0x21) <= (0x7E - 0x21)) {
                    /* Max index == (0x7E - 0x21) * 94 + 0x7E - 0x21 == 8835 */
                    ctx->codepoint = (iso->lead - 0x21) * 94 + byte - 0x21;

                    return pchtml_encoding_multi_index_jis0208[ctx->codepoint].codepoint;
                }

                return PCHTML_ENCODING_DECODE_ERROR;

            case PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START:
                if (byte == 0x24 || byte == 0x28) {
                    iso->state = PCHTML_ENCODING_DECODE_2022_JP_ESCAPE;
                    iso->lead = byte;

                    break;
                }

                (*data)--;

                iso->out_flag = false;
                iso->state = ctx->u.iso_2022_jp.out_state;

                return PCHTML_ENCODING_DECODE_ERROR;

            case PCHTML_ENCODING_DECODE_2022_JP_ESCAPE:
                iso->state = PCHTML_ENCODING_DECODE_2022_JP_UNSET;

                if (iso->lead == 0x28) {
                    if (byte == 0x42) {
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_ASCII;
                    }
                    else if (byte == 0x4A) {
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_ROMAN;
                    }
                    else if (byte == 0x49) {
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_KATAKANA;
                    }
                }
                else if (iso->lead == 0x24) {
                    if (byte == 0x40 || byte == 0x42) {
                        iso->state = PCHTML_ENCODING_DECODE_2022_JP_LEAD;
                    }
                }

                if (iso->state == PCHTML_ENCODING_DECODE_2022_JP_UNSET) {
                    iso->prepand = iso->lead;
                    iso->lead = 0x00;

                    (*data)--;

                    iso->out_flag = false;
                    iso->state = iso->out_state;

                    return PCHTML_ENCODING_DECODE_ERROR;
                }

                iso->lead = 0x00;
                iso->out_state = iso->state;

                if (iso->out_flag) {
                    return PCHTML_ENCODING_DECODE_ERROR;
                }

                iso->out_flag = true;

                break;
        }
    }
    while (*data < end);

    return PCHTML_ENCODING_DECODE_CONTINUE;
}

uint32_t
pchtml_encoding_decode_iso_8859_10_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_10[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_13_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_13[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_14_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_14[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_15_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_15[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_16_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_16[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_2_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_2[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_3_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_3[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_4_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_4[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_5_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_5[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_6_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_6[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_7_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_7[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_8_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_8[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_iso_8859_8_i_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_iso_8859_8[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_koi8_r_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_koi8_r[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_koi8_u_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_koi8_u[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_shift_jis_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    unsigned char byte, lead;

    if (ctx->u.lead != 0x00) {
        lead = (unsigned char) ctx->u.lead;
        ctx->u.lead = 0x00;

        goto lead_state;
    }

    lead = *(*data)++;

    if (lead <= 0x80) {
        return lead;
    }

    if ((unsigned) (lead - 0xA1) <= (0xDF - 0xA1)) {
        return 0xFF61 - 0xA1 + lead;
    }

    if ((unsigned) (lead - 0x81) > (0x9F - 0x81)
        && lead != 0xE0 && lead != 0xFC)
    {
        return PCHTML_ENCODING_DECODE_ERROR;
    }

    if (*data >= end) {
        ctx->u.lead = lead;

        return PCHTML_ENCODING_DECODE_CONTINUE;
    }

lead_state:

    byte = *(*data)++;

    if (byte < 0x7F) {
        ctx->codepoint = 0x40;
    }
    else {
        ctx->codepoint = 0x41;
    }

    if (lead < 0xA0) {
        ctx->second_codepoint = 0x81;
    }
    else {
        ctx->second_codepoint = 0xC1;
    }

    if ((unsigned) (byte - 0x40) <= (0x7E - 0x40)
        || (unsigned) (byte - 0x80) <= (0xFC - 0x80))
    {
        /* Max index == (0xFC - 0xC1) * 188 + 0xFC - 0x41 = 11279 */
        ctx->codepoint = (lead - ctx->second_codepoint) * 188
                          + byte - ctx->codepoint;

        if (ctx->codepoint >= (sizeof(pchtml_encoding_multi_index_jis0208)
            / sizeof(pchtml_encoding_multi_index_t)))
        {
            goto failed;
        }

        if ((unsigned) (ctx->codepoint - 8836) <= (10715 - 8836)) {
            return 0xE000 - 8836 + ctx->codepoint;
        }

        ctx->codepoint = pchtml_encoding_multi_index_jis0208[ctx->codepoint].codepoint;
        if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
            goto failed;
        }

        return ctx->codepoint;
    }

failed:

    if (byte < 0x80) {
        (*data)--;
    }

    return PCHTML_ENCODING_DECODE_ERROR;
}

static inline uint32_t
pchtml_encoding_decode_utf_16_single(pchtml_encoding_decode_t *ctx, bool is_be,
                                 const unsigned char **data, const unsigned char *end)
{
    unsigned lead;
    uint32_t unit;

    if (ctx->u.lead != 0x00) {
        lead = ctx->u.lead - 0x01;
        ctx->u.lead = 0x00;

        goto lead_state;
    }

pair_state:

    lead = *(*data)++;

    if (*data >= end) {
        ctx->u.lead = lead + 0x01;
        return PCHTML_ENCODING_DECODE_CONTINUE;
    }

lead_state:

    /* For UTF-16BE or UTF-16LE */
    if (is_be) {
        unit = (lead << 8) + *(*data)++;
    }
    else {
        unit = (*(*data)++ << 8) + lead;
    }

    if (ctx->second_codepoint != 0x00) {
        if ((unsigned) (unit - 0xDC00) <= (0xDFFF - 0xDC00)) {
            ctx->codepoint = 0x10000 + ((ctx->second_codepoint - 0xD800) << 10)
                             + (unit - 0xDC00);

            ctx->second_codepoint = 0x00;
            return ctx->codepoint;
        }

        (*data)--;

        ctx->u.lead = lead + 0x01;
        ctx->second_codepoint = 0x00;

        return PCHTML_ENCODING_DECODE_ERROR;
    }

    /* Surrogate pair */
    if ((unsigned) (unit - 0xD800) <= (0xDFFF - 0xD800)) {
        if ((unsigned) (unit - 0xDC00) <= (0xDFFF - 0xDC00)) {
            return PCHTML_ENCODING_DECODE_ERROR;
        }

        ctx->second_codepoint = unit;

        if (*data >= end) {
            return PCHTML_ENCODING_DECODE_CONTINUE;
        }

        goto pair_state;
    }

    return unit;
}

uint32_t
pchtml_encoding_decode_utf_16be_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_decode_utf_16_single(ctx, true, data, end);
}

uint32_t
pchtml_encoding_decode_utf_16le_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_decode_utf_16_single(ctx, false, data, end);
}

uint32_t
pchtml_encoding_decode_utf_8_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    unsigned needed;
    unsigned char ch;
    const unsigned char *p;

    if (ctx->u.utf_8.need != 0) {
        needed = ctx->u.utf_8.need;
        ctx->u.utf_8.need = 0;

        if (ctx->u.utf_8.lower != 0x00) {
            PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SINGLE(ctx->u.utf_8.lower,
                                                      ctx->u.utf_8.upper);
            ctx->u.utf_8.lower = 0x00;
        }

        goto decode;
    }

    ch = *(*data)++;

    if (ch < 0x80) {
        return ch;
    }
    else if (ch <= 0xDF) {
        if (ch < 0xC2) {
            return PCHTML_ENCODING_DECODE_ERROR;
        }

        needed = 1;
        ctx->codepoint = ch & 0x1F;
    }
    else if (ch < 0xF0) {
        needed = 2;
        ctx->codepoint = ch & 0x0F;

        if (*data == end) {
            PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SET_SINGLE(0xE0, 0xED,
                                                          0xA0, 0x9F);
            goto next;
        }

        if (ch == 0xE0) {
            PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SINGLE(0xA0, 0xBF);
        }
        else if (ch == 0xED) {
            PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SINGLE(0x80, 0x9F);
        }
    }
    else if (ch < 0xF5) {
        needed = 3;
        ctx->codepoint = ch & 0x07;

        if (*data == end) {
            PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SET_SINGLE(0xF0, 0xF4,
                                                          0x90, 0x8F);

            goto next;
        }

        if (ch == 0xF0) {
            PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SINGLE(0x90, 0xBF);
        }
        else if (ch == 0xF4) {
            PCHTML_ENCODING_DECODE_UTF_8_BOUNDARY_SINGLE(0x80, 0x8F);
        }
    }
    else {
        return PCHTML_ENCODING_DECODE_ERROR;
    }

decode:

    for (p = *data; p < end; p++) {
        ch = *p;

        if (ch < 0x80 || ch > 0xBF) {
            *data = p;

            goto failed;
        }

        ctx->codepoint = (ctx->codepoint << 6) | (ch & 0x3F);

        if (--needed == 0) {
            *data = p + 1;

            return ctx->codepoint;
        }
    }

    *data = p;

next:

    ctx->u.utf_8.need = needed;

    return PCHTML_ENCODING_DECODE_CONTINUE;

failed:

    ctx->u.utf_8.lower = 0x00;
    ctx->u.utf_8.need = 0;

    return PCHTML_ENCODING_DECODE_ERROR;
}

uint32_t
pchtml_encoding_decode_gb18030_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    uint32_t pointer;
    unsigned char first, second, third, offset;

    /* Make compiler happy */
    second = 0x00;

    if (ctx->u.gb18030.first != 0) {
        if (ctx->u.gb18030.third != 0x00) {
            first = ctx->u.gb18030.first;
            second = ctx->u.gb18030.second;
            third = ctx->u.gb18030.third;

            memset(&ctx->u.gb18030, 0, sizeof(pchtml_encoding_ctx_gb18030_t));

            if (ctx->prepend) {
                /* The first is always < 0x80 */
                ctx->u.gb18030.first = third;

                return second;
            }

            goto third_state;
        }
        else if (ctx->u.gb18030.second != 0x00) {
            first = ctx->u.gb18030.first;
            second = ctx->u.gb18030.second;

            memset(&ctx->u.gb18030, 0, sizeof(pchtml_encoding_ctx_gb18030_t));

            goto second_state;
        }

        first = ctx->u.gb18030.first;
        ctx->u.gb18030.first = 0x00;

        if (ctx->prepend) {
            ctx->prepend = false;
            goto prepend_first;
        }

        goto first_state;
    }

    first = *(*data)++;

prepend_first:

    if (first < 0x80) {
        return first;
    }

    if (first == 0x80) {
        return 0x20AC;
    }

    /* Range 0x81 to 0xFE, inclusive */
    if ((unsigned) (first - 0x81) > (0xFE - 0x81)) {
        return PCHTML_ENCODING_DECODE_ERROR;
    }

    if (*data == end) {
        ctx->u.gb18030.first = first;
        return PCHTML_ENCODING_DECODE_CONTINUE;
    }

    /* First */
first_state:

    second = *(*data)++;

    /* Range 0x30 to 0x39, inclusive */
    if ((unsigned) (second - 0x30) > (0x39 - 0x30)) {
        offset = (second < 0x7F) ? 0x40 : 0x41;

        /* Range 0x40 to 0x7E, inclusive, or 0x80 to 0xFE, inclusive */
        if ((unsigned) (second - 0x40) <= (0x7E - 0x40)
            || (unsigned) (second - 0x80) <= (0xFE - 0x80))
        {
            pointer = (first - 0x81) * 190 + (second - offset);
        }
        else {
            goto failed;
        }

        /* Max pointer value == (0xFE - 0x81) * 190 + (0xFE - 0x41) == 23939 */
        ctx->codepoint = pchtml_encoding_multi_index_gb18030[pointer].codepoint;
        if (ctx->codepoint == PCHTML_ENCODING_ERROR_CODEPOINT) {
            goto failed;
        }

        return ctx->codepoint;
    }

    if (*data == end) {
        ctx->u.gb18030.first = first;
        ctx->u.gb18030.second = second;

        return PCHTML_ENCODING_DECODE_CONTINUE;
    }

    /* Second */
second_state:

    third = *(*data)++;

    /* Range 0x81 to 0xFE, inclusive */
    if ((unsigned) (third - 0x81) > (0xFE - 0x81)) {
        (*data)--;

        ctx->prepend = true;
        ctx->u.gb18030.first = second;

        return PCHTML_ENCODING_DECODE_ERROR;
    }

    if (*data == end) {
        ctx->u.gb18030.first = first;
        ctx->u.gb18030.second = second;
        ctx->u.gb18030.third = third;

        return PCHTML_ENCODING_DECODE_CONTINUE;
    }

    /* Third */
third_state:

    /* Range 0x30 to 0x39, inclusive */
    if ((unsigned) (**data - 0x30) > (0x39 - 0x30)) {
        ctx->prepend = true;

        /* First is a fake for trigger */
        ctx->u.gb18030.first = 0x01;
        ctx->u.gb18030.second = second;
        ctx->u.gb18030.third = third;

        return PCHTML_ENCODING_DECODE_ERROR;
    }

    pointer = ((first  - 0x81) * (10 * 126 * 10))
            + ((second - 0x30) * (10 * 126))
            + ((third  - 0x81) * 10) + (*(*data)++) - 0x30;

    return pchtml_encoding_decode_gb18030_range(pointer);

failed:

    if (second < 0x80) {
        (*data)--;
    }

    return PCHTML_ENCODING_DECODE_ERROR;
}

uint32_t
pchtml_encoding_decode_macintosh_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_macintosh[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_replacement_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(data);
    UNUSED_PARAM(end);

    return PCHTML_ENCODING_DECODE_ERROR;
}

uint32_t
pchtml_encoding_decode_windows_1250_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_1250[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_windows_1251_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_1251[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_windows_1252_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_1252[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_windows_1253_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_1253[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_windows_1254_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_1254[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_windows_1255_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_1255[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_windows_1256_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_1256[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_windows_1257_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_1257[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_windows_1258_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_1258[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_windows_874_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_windows_874[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_x_mac_cyrillic_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return pchtml_encoding_single_index_x_mac_cyrillic[*(*data)++ - 0x80].codepoint;
}

uint32_t
pchtml_encoding_decode_x_user_defined_single(pchtml_encoding_decode_t *ctx,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(end);

    if (**data < 0x80) {
        return *(*data)++;
    }

    return 0xF780 + (*(*data)++) - 0x80;
}
