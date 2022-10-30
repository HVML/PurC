/*
** @file unicode.c
** @author Vincent Wei
** @date 2022/10/21
** @brief Implemetation of Unicode-related interface.
**  Note that we copied most of code from GPL'd MiniGUI:
**
**      <https://github.com/VincentWei/MiniGUI/>
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <glib.h>

#include "foil.h"
#include "unicode.h"

#define MIN_LEN_UCHARS      4
#define INC_LEN_UCHARS      8

bool foil_uchar_is_letter(uint32_t uc)
{
    GUnicodeType gc = g_unichar_type(uc);
    GUnicodeBreakType bt = g_unichar_break_type(uc);

    if ((gc >= G_UNICODE_LOWERCASE_LETTER
                && gc <= G_UNICODE_UPPERCASE_LETTER)
            || (gc >= G_UNICODE_DECIMAL_NUMBER
                && gc <= G_UNICODE_OTHER_NUMBER))
        return TRUE;

    if (bt == G_UNICODE_BREAK_NUMERIC
            || bt == G_UNICODE_BREAK_ALPHABETIC
            || bt == G_UNICODE_BREAK_IDEOGRAPHIC
            || bt == G_UNICODE_BREAK_AMBIGUOUS)
        return TRUE;

    return FALSE;
}

/** Converts a unichar to full-width. */
uint32_t foil_uchar_to_fullwidth(uint32_t uc)
{
    if (uc == 0x20)
        return 0x3000;

    if (uc >= 0x21 && uc <= 0x7E) {
        return uc + (0xFF01 - 0x21);
    }

    return uc;
}

/** Converts a chv to single-width. */
uint32_t foil_uchar_to_singlewidth(uint32_t uc)
{
    if (uc == 0x3000)
        return 0x20;

    if (uc >= 0xFF01 && uc <= 0xFF5E) {
        return uc - (0xFF01 - 0x21);
    }

    return uc;
}

struct uchar_map {
    gunichar one, other;
};

static const struct uchar_map kana_small_to_full_size_table [] = {
    { /*ぁ*/ 0x3041, /*あ*/ 0x3042},
    { /*ぃ*/ 0x3043, /*い*/ 0x3044},
    { /*ぅ*/ 0x3045, /*う*/ 0x3046},
    { /*ぇ*/ 0x3047, /*え*/ 0x3048},
    { /*ぉ*/ 0x3049, /*お*/ 0x304A},
    { /*ゕ*/ 0x3095, /*か*/ 0x304B},
    { /*ゖ*/ 0x3096, /*け*/ 0x3051},
    { /*っ*/ 0x3063, /*つ*/ 0x3064},
    { /*ゃ*/ 0x3083, /*や*/ 0x3084},
    { /*ゅ*/ 0x3085, /*ゆ*/ 0x3086},
    { /*ょ*/ 0x3087, /*よ*/ 0x3088},
    { /*ゎ*/ 0x308E, /*わ*/ 0x308F},
    { /*ァ*/ 0x30A1, /*ア*/ 0x30A2},
    { /*ィ*/ 0x30A3, /*イ*/ 0x30A4},
    { /*ゥ*/ 0x30A5, /*ウ*/ 0x30A6},
    { /*ェ*/ 0x30A7, /*エ*/ 0x30A8},
    { /*ォ*/ 0x30A9, /*オ*/ 0x30AA},
    { /*ヵ*/ 0x30F5, /*カ*/ 0x30AB},
    { /*ㇰ*/ 0x31F0, /*ク*/ 0x30AF},
    { /*ヶ*/ 0x30F6, /*ケ*/ 0x30B1},
    { /*ㇱ*/ 0x31F1, /*シ*/ 0x30B7},
    { /*ㇲ*/ 0x31F2, /*ス*/ 0x30B9},
    { /*ッ*/ 0x30C3, /*ツ*/ 0x30C4},
    { /*ㇳ*/ 0x31F3, /*ト*/ 0x30C8},
    { /*ㇴ*/ 0x31F4, /*ヌ*/ 0x30CC},
    { /*ㇵ*/ 0x31F5, /*ハ*/ 0x30CF},
    { /*ㇶ*/ 0x31F6, /*ヒ*/ 0x30D2},
    { /*ㇷ*/ 0x31F7, /*フ*/ 0x30D5},
    { /*ㇸ*/ 0x31F8, /*ヘ*/ 0x30D8},
    { /*ㇹ*/ 0x31F9, /*ホ*/ 0x30DB},
    { /*ㇺ*/ 0x31FA, /*ム*/ 0x30E0},
    { /*ャ*/ 0x30E3, /*ヤ*/ 0x30E4},
    { /*ュ*/ 0x30E5, /*ユ*/ 0x30E6},
    { /*ョ*/ 0x30E7, /*ヨ*/ 0x30E8},
    { /*ㇻ*/ 0x31FB, /*ラ*/ 0x30E9},
    { /*ㇼ*/ 0x31FC, /*リ*/ 0x30EA},
    { /*ㇽ*/ 0x31FD, /*ル*/ 0x30EB},
    { /*ㇾ*/ 0x31FE, /*レ*/ 0x30EC},
    { /*ㇿ*/ 0x31FF, /*ロ*/ 0x30ED},
    { /*ヮ*/ 0x30EE, /*ワ*/ 0x30EF},
    { /*ｧ*/ 0xFF67, /*ｱ*/ 0xFF71},
    { /*ｨ*/ 0xFF68, /*ｲ*/ 0xFF72},
    { /*ｩ*/ 0xFF69, /*ｳ*/ 0xFF73},
    { /*ｪ*/ 0xFF6A, /*ｴ*/ 0xFF74},
    { /*ｫ*/ 0xFF6B, /*ｵ*/ 0xFF75},
    { /*ｯ*/ 0xFF6F, /*ﾂ*/ 0xFF82},
    { /*ｬ*/ 0xFF6C, /*ﾔ*/ 0xFF94},
    { /*ｭ*/ 0xFF6D, /*ﾕ*/ 0xFF95},
    { /*ｮ*/ 0xFF6E, /*ﾖ*/ 0xFF96},
};

/** Converts a chv to full-size Kana. */
uint32_t foil_uchar_to_fullsize_kana(uint32_t uc)
{
    unsigned int lower = 0;
    unsigned int upper = PCA_TABLESIZE (kana_small_to_full_size_table) - 1;
    int mid = PCA_TABLESIZE (kana_small_to_full_size_table) / 2;

    if (uc < kana_small_to_full_size_table[lower].one
            || uc > kana_small_to_full_size_table[upper].one)
        return uc;

    do {
        if (uc < kana_small_to_full_size_table[mid].one)
            upper = mid - 1;
        else if (uc > kana_small_to_full_size_table[mid].one)
            lower = mid + 1;
        else
            return kana_small_to_full_size_table[mid].other;

        mid = (lower + upper) / 2;

    } while (lower <= upper);

    return uc;
}

/** Converts a chv to small Kana. */
uint32_t foil_uchar_to_small_kana (uint32_t uc)
{
    unsigned int lower = 0;
    unsigned int upper = PCA_TABLESIZE (kana_small_to_full_size_table) - 1;
    int mid = PCA_TABLESIZE (kana_small_to_full_size_table) / 2;

    if (uc < kana_small_to_full_size_table[lower].other
            || uc > kana_small_to_full_size_table[upper].other)
        return uc;

    do {
        if (uc < kana_small_to_full_size_table[mid].other)
            upper = mid - 1;
        else if (uc > kana_small_to_full_size_table[mid].other)
            lower = mid + 1;
        else
            return kana_small_to_full_size_table[mid].one;

        mid = (lower + upper) / 2;

    } while (lower <= upper);

    return uc;
}

struct ustr_ctxt {
    uint32_t*   ucs;
    size_t      len_buff;
    size_t      n;
    uint8_t     wsr;
};

static size_t usctxt_init_spaces(struct ustr_ctxt* ctxt, size_t size)
{
    // pre-allocate buffers
    ctxt->len_buff = size;
    if (ctxt->len_buff < MIN_LEN_UCHARS)
        ctxt->len_buff = MIN_LEN_UCHARS;

    ctxt->ucs = (uint32_t*)malloc(sizeof(uint32_t) * ctxt->len_buff);
    if (ctxt->ucs == NULL)
        return 0;

    ctxt->n = 0;
    return ctxt->len_buff;
}

static size_t usctxt_push_back(struct ustr_ctxt* ctxt, uint32_t uc)
{
    /* realloc buffers if it needs */
    if ((ctxt->n + 2) >= ctxt->len_buff) {
        ctxt->len_buff += INC_LEN_UCHARS;
        ctxt->ucs = (uint32_t*)realloc(ctxt->ucs,
            sizeof(uint32_t) * ctxt->len_buff);

        if (ctxt->ucs == NULL)
            return 0;
    }

    ctxt->ucs[ctxt->n] = uc;
    ctxt->n++;
    return ctxt->n;
}

static size_t get_next_uchar(const char *mstr, size_t mstr_len,
        uint32_t* uc)
{
    const char *next = pcutils_utf8_next_char(mstr);
    size_t mclen = next - mstr;

    if (mclen > mstr_len) {
        return 0;
    }

    if (mclen > 0 && mstr_len >= mclen) {
        gunichar uch = g_utf8_get_char(mstr);
        *uc = uch;
    }

    return mclen;
}

static size_t is_next_mchar_bt(const char* mstr, size_t mstr_len,
        uint32_t* uc, GUnicodeBreakType bt)
{
    size_t mclen;

    mclen = get_next_uchar(mstr, mstr_len, uc);
    if (mclen > 0 && g_unichar_break_type(*uc) == bt)
        return mclen;

    return 0;
}

static size_t is_next_mchar_lf(const char* mstr, size_t mstr_len,
        uint32_t* uc)
{
    return is_next_mchar_bt(mstr, mstr_len, uc,
            G_UNICODE_BREAK_LINE_FEED);
}

static size_t collapse_space(const char* mstr, size_t mstr_len)
{
    uint32_t uc;
    GUnicodeBreakType bt;
    size_t consumed = 0;

    do {
        size_t mclen;

        mclen = get_next_uchar(mstr, mstr_len, &uc);
        if (mclen == 0)
            break;

        bt = g_unichar_break_type(uc);
        if (bt != G_UNICODE_BREAK_SPACE && uc != FOIL_UCHAR_TAB)
            break;

        mstr += mclen;
        mstr_len -= mclen;
        consumed += mclen;
    } while (1);

    return consumed;
}

/*
    Reference:
    [CSS Text Module Level 3](https://www.w3.org/TR/css-text-3/)
 */
size_t foil_ustr_from_utf8_until_paragraph_boundary(const char* mstr,
        size_t mstr_len, uint8_t wsr, uint32_t** uchars, size_t* nr_uchars)
{
    struct ustr_ctxt ctxt;
    size_t consumed = 0;
    bool col_sp = false;
    bool col_nl = false;

    // CSS: collapses space according to space rule
    if (wsr == FOIL_WSR_NORMAL || wsr == FOIL_WSR_NOWRAP || wsr == FOIL_WSR_PRE_LINE)
        col_sp = true;
    // CSS: collapses new lines acoording to space rule
    if (wsr == FOIL_WSR_NORMAL || wsr == FOIL_WSR_NOWRAP)
        col_nl = true;

    *uchars = NULL;
    *nr_uchars = 0;

    if (mstr_len == 0)
        return 0;

    ctxt.wsr = wsr;
    if (usctxt_init_spaces(&ctxt, mstr_len >> 1) <= 0) {
        goto error;
    }

    while (true) {
        uint32_t uc, next_uc;
        GUnicodeBreakType bt;

        size_t mclen = 0;
        size_t next_mclen;
        size_t cosumed_one_loop = 0;

        mclen = get_next_uchar(mstr, mstr_len, &uc);
        if (mclen == 0) {
            // badly encoded or end of text
            break;
        }

        mstr += mclen;
        mstr_len -= mclen;
        consumed += mclen;

        if ((wsr == FOIL_WSR_NORMAL || wsr == FOIL_WSR_NOWRAP
                || wsr == FOIL_WSR_PRE_LINE) && uc == FOIL_UCHAR_TAB) {
            LOG_DEBUG ("CSS: Every tab is converted to a space (U+0020)\n");
            uc = FOIL_UCHAR_SPACE;
        }

        bt = g_unichar_break_type(uc);
        if (usctxt_push_back(&ctxt, uc) == 0)
            goto error;

        /* Check mandatory breaks */
        if (bt == G_UNICODE_BREAK_MANDATORY) {
            break;
        }
        else if (bt == G_UNICODE_BREAK_CARRIAGE_RETURN
                && (next_mclen = is_next_mchar_lf(mstr, mstr_len,
                        &next_uc)) > 0) {
            cosumed_one_loop += next_mclen;

            if (col_nl) {
                assert(ctxt.n > 0);
                ctxt.n--;
            }
            else {
                if (usctxt_push_back(&ctxt, next_uc) == 0)
                    goto error;
                break;
            }
        }
        else if (bt == G_UNICODE_BREAK_CARRIAGE_RETURN
                || bt == G_UNICODE_BREAK_LINE_FEED
                || bt == G_UNICODE_BREAK_NEXT_LINE) {

            if (col_nl) {
                assert(ctxt.n > 0);
                ctxt.n--;
            }
            else {
                break;
            }
        }
        /* collapse spaces */
        else if (col_sp && (bt == G_UNICODE_BREAK_SPACE
                || bt == G_UNICODE_BREAK_ZERO_WIDTH_SPACE)) {
            cosumed_one_loop += collapse_space(mstr, mstr_len);
        }

        mstr_len -= cosumed_one_loop;
        mstr += cosumed_one_loop;
        consumed += cosumed_one_loop;
    }

    if (ctxt.n > 0) {
        *uchars = ctxt.ucs;
        *nr_uchars = ctxt.n;
    }
    else
        goto error;

    return consumed;

error:
    if (ctxt.ucs) free(ctxt.ucs);
    return 0;
}

