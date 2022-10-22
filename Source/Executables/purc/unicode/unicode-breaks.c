/*
** @file unicode-breaks.c
** @author Vincent Wei
** @date 2022/10/22
** @brief Implemetation of foil_ustr_get_breaks().
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

#define G_UNICODE_BREAK_UNSET   0xFF

enum _LBOrder {
    LB1, LB2, LB3, LB4, LB5, LB6, LB7, LB8, LBPRE, LB8a, LB9, LB10,
    LB11, LB12, LB12a, LB13, LB14, LB15, LB16, LB17, LB18, LB19,
    LB20, LB21, LB21a, LB21b, LB22, LB23, LB23a, LB24, LB25, LB26,
    LB27, LB28, LB29, LB30, LB30a, LB30b, LB31,
    LBLAST = 0xFF,
};

/* See Grapheme_Cluster_Break Property Values table of UAX#29 */
typedef enum {
    GB_Other,
    GB_ControlCRLF,
    GB_Extend,
    GB_ZWJ,
    GB_Prepend,
    GB_SpacingMark,
    GB_InHangulSyllable, /* Handles all of L, V, T, LV, LVT rules */
    /* Use state machine to handle emoji sequence */
    /* Rule GB12 and GB13 */
    GB_RI_Odd, /* Meets odd number of RI */
    GB_RI_Even, /* Meets even number of RI */
} _GBType;

/* See Word_Break Property Values table of UAX#29 */
typedef enum {
    WB_Other,
    WB_NewlineCRLF,
    WB_ExtendFormat,
    WB_Katakana,
    WB_Hebrew_Letter,
    WB_ALetter,
    WB_MidNumLet,
    WB_MidLetter,
    WB_MidNum,
    WB_Numeric,
    WB_ExtendNumLet,
    WB_RI_Odd,
    WB_RI_Even,
    WB_WSegSpace,
} _WBType;

/* See Sentence_Break Property Values table of UAX#29 */
typedef enum {
    SB_Other,
    SB_ExtendFormat,
    SB_ParaSep,
    SB_Sp,
    SB_Lower,
    SB_Upper,
    SB_OLetter,
    SB_Numeric,
    SB_ATerm,
    SB_SContinue,
    SB_STerm,
    SB_Close,
    /* Rules SB8 and SB8a */
    SB_ATerm_Close_Sp,
    SB_STerm_Close_Sp,
} _SBType;

/* An enum that works as the states of the Hangul syllables system.
 */
typedef enum {
    JAMO_L,     /* G_UNICODE_BREAK_HANGUL_L_JAMO */
    JAMO_V,     /* G_UNICODE_BREAK_HANGUL_V_JAMO */
    JAMO_T,     /* G_UNICODE_BREAK_HANGUL_T_JAMO */
    JAMO_LV,    /* G_UNICODE_BREAK_HANGUL_LV_SYLLABLE */
    JAMO_LVT,   /* G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE */
    NO_JAMO     /* Other */
} _JamoType;

/* Previously "123foo" was two words. But in UAX#29,
 * we know don't break words between consecutive letters and numbers.
 */
typedef enum {
    WordNone,
    WordLetters,
    WordNumbers
} _WordType;

struct break_ctxt {
    uint32_t* ucs;
    uint8_t*   bts;
    uint8_t*   ods;
    uint16_t*  bos;

    size_t     nr_ucs;
    ssize_t    n;

    foil_langcode_t lc;
    uint8_t   ctr;
    uint8_t   wbr;
    uint8_t   lbp;

    uint8_t   base_bt;
    uint8_t   curr_gc;
    uint8_t   curr_od;

    /* UAX#29 boundaries */
    uint32_t prev_uc;
    uint32_t base_uc;
    uint32_t last_word_letter;

    ssize_t  last_stc_start;
    ssize_t  last_non_space;
    ssize_t  prev_wb_index;
    ssize_t  prev_sb_index;

    _GBType prev_gbt;
    _WBType prev_wbt, prev_prev_wbt;
    _SBType prev_sbt, prev_prev_sbt;
    _JamoType prev_jamo;
    _WordType curr_wt;

    uint8_t   makes_hangul_syllable:1;
    uint8_t   met_extended_pictographic:1;
    uint8_t   is_extended_pictographic:1;

    uint8_t   is_grapheme_boundary:1;
    uint8_t   is_word_boundary:1;
    uint8_t   is_sentence_boundary:1;
};

static bool is_letter(GUnicodeType gc, GUnicodeBreakType bt)
{
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

static bool break_change_lbo_last(struct break_ctxt* ctxt,
        uint16_t lbo)
{
    if (ctxt->n == 0)
        return false;

    size_t i = ctxt->n - 1;

    if ((ctxt->bos[i] & FOIL_BOV_LB_MASK) == FOIL_BOV_UNKNOWN) {
         ctxt->bos[i] &= ~FOIL_BOV_LB_MASK;
         ctxt->bos[i] |= lbo;
         ctxt->ods[i] = ctxt->curr_od;
    }
    else if (ctxt->bos[i] & FOIL_BOV_LB_MANDATORY_FLAG) {
        LOG_DEBUG("%s: ignore the change: old one is mandatory\n",
            __FUNCTION__);
    }
    else if (ctxt->curr_od <= ctxt->ods[i]) {
        LOG_DEBUG("%s: changed: curr_od(%d), org_od(%d)\n",
            __FUNCTION__, ctxt->curr_od, ctxt->ods[i]);
         ctxt->bos[i] &= ~FOIL_BOV_LB_MASK;
         ctxt->bos[i] |= lbo;
         ctxt->ods[i] = ctxt->curr_od;
    }
    else {
        LOG_DEBUG("%s: ignore the change: curr_od(%d), org_od(%d)\n",
            __FUNCTION__, ctxt->curr_od, ctxt->ods[i]);
    }

    return true;
}

static bool break_change_lbo_before_last(struct break_ctxt* ctxt,
        uint16_t lbo)
{
    size_t i = ctxt->n - 2;

    // do not allow to change the first break value
    if (i < 1) {
        return false;
    }

    if ((ctxt->bos[i] & FOIL_BOV_LB_MASK) == FOIL_BOV_UNKNOWN) {
         ctxt->bos[i] &= ~FOIL_BOV_LB_MASK;
         ctxt->bos[i] |= lbo;
         ctxt->ods[i] = ctxt->curr_od;
    }
    else if (ctxt->bos[i] & FOIL_BOV_LB_MANDATORY_FLAG) {
        LOG_DEBUG("%s: ignore the change: old one is mandatory\n",
            __FUNCTION__);
    }
    else if (ctxt->curr_od <= ctxt->ods[i]) {
        LOG_DEBUG("%s: changed: curr_od(%d), org_od(%d)\n",
            __FUNCTION__, ctxt->curr_od, ctxt->ods[ctxt->n - 2]);
         ctxt->bos[i] &= ~FOIL_BOV_LB_MASK;
         ctxt->bos[i] |= lbo;
         ctxt->ods[i] = ctxt->curr_od;
    }
    else {
        LOG_DEBUG("%s: ignore the change\n", __FUNCTION__);
    }

    return true;
}

static GUnicodeBreakType resolve_lbc(struct break_ctxt* ctxt, uint32_t uc)
{
    GUnicodeBreakType bt;

    bt = g_unichar_break_type(uc);
    ctxt->curr_gc = g_unichar_type(uc);

    /*
     * TODO: according to the content language and the writing system
     * to resolve AI, CB, CJ, SA, SG, and XX into other line breaking classes.
     */

    // default handling.
    switch (bt) {
    case G_UNICODE_BREAK_AMBIGUOUS:
    case G_UNICODE_BREAK_SURROGATE:
    case G_UNICODE_BREAK_UNKNOWN:
        bt = G_UNICODE_BREAK_ALPHABETIC;
        break;

    case G_UNICODE_BREAK_COMPLEX_CONTEXT:
        if (ctxt->curr_gc == G_UNICODE_NON_SPACING_MARK
                || ctxt->curr_gc == G_UNICODE_SPACING_MARK) {
            bt = G_UNICODE_BREAK_COMBINING_MARK;
        }
        else {
            bt = G_UNICODE_BREAK_ALPHABETIC;
        }
        break;

    case G_UNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER:
        bt = G_UNICODE_BREAK_NON_STARTER;
        break;

    case G_UNICODE_BREAK_COMBINING_MARK:
    case G_UNICODE_BREAK_ZERO_WIDTH_JOINER:
        if (ctxt && ctxt->base_bt != G_UNICODE_BREAK_UNSET) {
            LOG_DEBUG ("CM as if it has the line breaking class of the base character(%d)\n", ctxt->base_bt);
            // CM/ZWJ should have the same break class as
            // its base character.
            bt = ctxt->base_bt;
        }
        break;

    default:
        break;
    }

    /*
     * Breaking is allowed within “words”: specifically, in addition to
     * soft wrap opportunities allowed for normal, any typographic character
     * units resolving to the NU (“numeric”), AL (“alphabetic”), or
     * SA (“Southeast Asian”) line breaking classes are instead treated
     * as ID (“ideographic characters”) for the purpose of line-breaking.
     */
    if (ctxt->wbr == FOIL_WBR_BREAK_ALL &&
            (bt == G_UNICODE_BREAK_NUMERIC || bt == G_UNICODE_BREAK_ALPHABETIC
                || bt == G_UNICODE_BREAK_COMPLEX_CONTEXT)) {
        bt = G_UNICODE_BREAK_IDEOGRAPHIC;
    }

    return bt;
}

/* Find the Grapheme Break Type of uc */
static _GBType resolve_gbt(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeType gc)
{
    _GBType gbt;

    gbt = GB_Other;
    switch ((uint8_t)gc) {
    case G_UNICODE_FORMAT:
        if (uc == 0x200C) {
            gbt = GB_Extend;
            break;
        }
        if (uc == 0x200D) {
            gbt = GB_ZWJ;
            break;
        }
        if ((uc >= 0x600 && uc <= 0x605) ||
                uc == 0x6DD || uc == 0x70F ||
                uc == 0x8E2 || uc == 0xD4E || uc == 0x110BD ||
                (uc >= 0x111C2 && uc <= 0x111C3)) {
            gbt = GB_Prepend;
        }
        break;

    case G_UNICODE_CONTROL:
    case G_UNICODE_LINE_SEPARATOR:
    case G_UNICODE_PARAGRAPH_SEPARATOR:
    case G_UNICODE_SURROGATE:
        /* fall through */
        gbt = GB_ControlCRLF;
        break;

    case G_UNICODE_UNASSIGNED:
        /* Unassigned default ignorables */
        if ((uc >= 0xFFF0 && uc <= 0xFFF8) ||
                (uc >= 0xE0000 && uc <= 0xE0FFF)) {
            gbt = GB_ControlCRLF;
        }
        break;

    case G_UNICODE_OTHER_LETTER:
        if (ctxt->makes_hangul_syllable)
            gbt = GB_InHangulSyllable;
        break;

    case G_UNICODE_MODIFIER_LETTER:
        if (uc >= 0xFF9E && uc <= 0xFF9F)
            gbt = GB_Extend; /* Other_Grapheme_Extend */
        break;

    case G_UNICODE_SPACING_MARK:
        gbt = GB_SpacingMark; /* SpacingMark */
        if (uc >= 0x0900 &&
            (uc == 0x09BE || uc == 0x09D7 ||
                uc == 0x0B3E || uc == 0x0B57 ||
                uc == 0x0BBE || uc == 0x0BD7 ||
                uc == 0x0CC2 || uc == 0x0CD5 ||
                uc == 0x0CD6 || uc == 0x0D3E ||
                uc == 0x0D57 || uc == 0x0DCF ||
                uc == 0x0DDF || uc == 0x1D165 ||
                (uc >= 0x1D16E && uc <= 0x1D172))) {
            gbt = GB_Extend; /* Other_Grapheme_Extend */
        }
        break;

    case G_UNICODE_ENCLOSING_MARK:
    case G_UNICODE_NON_SPACING_MARK:
        gbt = GB_Extend; /* Grapheme_Extend */
        break;

    case G_UNICODE_OTHER_SYMBOL:
        if (uc >= 0x1F1E6 && uc <= 0x1F1FF) {
            if (ctxt->prev_gbt == GB_RI_Odd)
                gbt = GB_RI_Even;
            else if (ctxt->prev_gbt == GB_RI_Even)
                gbt = GB_RI_Odd;
            else
                gbt = GB_RI_Odd;
            break;
        }
        break;

    case G_UNICODE_MODIFIER_SYMBOL:
        if (uc >= 0x1F3FB && uc <= 0x1F3FF)
            gbt = GB_Extend;
        break;
    }

    return gbt;
}

/* Find the Word Break Type of uc */
static _WBType resolve_wbt(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeType gc, GUnicodeBreakType bt)
{
    GUnicodeScript script;
    _WBType wbt;

    script = g_unichar_get_script(uc);
    wbt = WB_Other;

    if (script == G_UNICODE_SCRIPT_KATAKANA)
        wbt = WB_Katakana;

    if (script == G_UNICODE_SCRIPT_HEBREW && gc == G_UNICODE_OTHER_LETTER)
        wbt = WB_Hebrew_Letter;

    if (wbt == WB_Other) {
        switch (uc >> 8) {
        case 0x30:
            if ((uc >= 0x3031 && uc <= 0x3035) ||
                    uc == 0x309b || uc == 0x309c ||
                    uc == 0x30a0 || uc == 0x30fc)
                wbt = WB_Katakana; /* Katakana exceptions */
            break;
        case 0xFF:
            if (uc == 0xFF70)
                wbt = WB_Katakana; /* Katakana exceptions */
            else if (uc >= 0xFF9E && uc <= 0xFF9F)
                wbt = WB_ExtendFormat; /* Other_Grapheme_Extend */
            break;
        case 0x05:
            if (uc == 0x05F3)
                wbt = WB_ALetter; /* ALetter exceptions */
            break;
        }
    }

    if (wbt == WB_Other) {
        switch ((uint8_t)bt) {
        case G_UNICODE_BREAK_NUMERIC:
            if (uc != 0x066C)
                wbt = WB_Numeric; /* Numeric */
            break;
        case G_UNICODE_BREAK_INFIX_SEPARATOR:
            if (uc != 0x003A && uc != 0xFE13 && uc != 0x002E)
                wbt = WB_MidNum; /* MidNum */
            break;
        }
    }

    if (wbt == WB_Other) {
        switch ((uint8_t)gc) {
        case G_UNICODE_CONTROL:
            if (uc != 0x000D && uc != 0x000A && uc != 0x000B &&
                    uc != 0x000C && uc != 0x0085)
                break;
            /* fall through */
        case G_UNICODE_LINE_SEPARATOR:
        case G_UNICODE_PARAGRAPH_SEPARATOR:
            wbt = WB_NewlineCRLF; /* CR, LF, Newline */
            break;

        case G_UNICODE_FORMAT:
        case G_UNICODE_SPACING_MARK:
        case G_UNICODE_ENCLOSING_MARK:
        case G_UNICODE_NON_SPACING_MARK:
            wbt = WB_ExtendFormat; /* Extend, Format */
            break;

        case G_UNICODE_CONNECT_PUNCTUATION:
            wbt = WB_ExtendNumLet; /* ExtendNumLet */
            break;

        case G_UNICODE_INITIAL_PUNCTUATION:
        case G_UNICODE_FINAL_PUNCTUATION:
            if (uc == 0x2018 || uc == 0x2019)
                wbt = WB_MidNumLet; /* MidNumLet */
            break;
        case G_UNICODE_OTHER_PUNCTUATION:
            if (uc == 0x0027 || uc == 0x002e || uc == 0x2024 ||
                    uc == 0xfe52 || uc == 0xff07 || uc == 0xff0e)
                wbt = WB_MidNumLet; /* MidNumLet */
            else if (uc == 0x00b7 || uc == 0x05f4 || uc == 0x2027 ||
                    uc == 0x003a || uc == 0x0387 ||
                    uc == 0xfe13 || uc == 0xfe55 || uc == 0xff1a)
                wbt = WB_MidLetter; /* MidLetter */
            else if (uc == 0x066c ||
                    uc == 0xfe50 || uc == 0xfe54 || uc == 0xff0c ||
                    uc == 0xff1b)
                wbt = WB_MidNum; /* MidNum */
            break;

        case G_UNICODE_OTHER_SYMBOL:
            if (uc >= 0x24B6 && uc <= 0x24E9) /* Other_Alphabetic */
                goto Alphabetic;

            if (uc >=0x1F1E6 && uc <=0x1F1FF) {
                if (ctxt->prev_wbt == WB_RI_Odd)
                    wbt = WB_RI_Even;
                else if (ctxt->prev_wbt == WB_RI_Even)
                    wbt = WB_RI_Odd;
                else
                    wbt = WB_RI_Odd;
            }

            break;

        case G_UNICODE_OTHER_LETTER:
        case G_UNICODE_LETTER_NUMBER:
            if (uc == 0x3006 || uc == 0x3007 ||
                    (uc >= 0x3021 && uc <= 0x3029) ||
                    (uc >= 0x3038 && uc <= 0x303A) ||
                    (uc >= 0x3400 && uc <= 0x4DB5) ||
                    (uc >= 0x4E00 && uc <= 0x9FC3) ||
                    (uc >= 0xF900 && uc <= 0xFA2D) ||
                    (uc >= 0xFA30 && uc <= 0xFA6A) ||
                    (uc >= 0xFA70 && uc <= 0xFAD9) ||
                    (uc >= 0x20000 && uc <= 0x2A6D6) ||
                    (uc >= 0x2F800 && uc <= 0x2FA1D))
                break; /* ALetter exceptions: Ideographic */
            goto Alphabetic;

        case G_UNICODE_LOWERCASE_LETTER:
        case G_UNICODE_MODIFIER_LETTER:
        case G_UNICODE_TITLECASE_LETTER:
        case G_UNICODE_UPPERCASE_LETTER:
Alphabetic:
            if (bt != G_UNICODE_BREAK_COMPLEX_CONTEXT
                    && script != G_UNICODE_SCRIPT_HIRAGANA)
                wbt = WB_ALetter; /* ALetter */
            break;
        }
    }

    if (wbt == WB_Other) {
        if (gc == G_UNICODE_SPACE_SEPARATOR &&
                bt != G_UNICODE_BREAK_NON_BREAKING_GLUE)
            wbt = WB_WSegSpace;
    }

    LOG_DEBUG("%s: uc(%0X), script(%d), wbt(%d)\n",
        __FUNCTION__, uc, script, wbt);

    return wbt;
}

/* Find the Sentence Break Type of wc */
static _SBType resolve_sbt(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeType gc, GUnicodeBreakType bt)
{
    (void)ctxt;

    _SBType sbt;

    sbt = SB_Other;
    if (bt == G_UNICODE_BREAK_NUMERIC)
        sbt = SB_Numeric; /* Numeric */

    if (sbt == SB_Other) {
        switch ((uint8_t)gc) {
        case G_UNICODE_CONTROL:
            if (uc == '\r' || uc == '\n')
                sbt = SB_ParaSep;
            else if (uc == 0x0009 || uc == 0x000B || uc == 0x000C)
                sbt = SB_Sp;
            else if (uc == 0x0085)
                sbt = SB_ParaSep;
            break;

        case G_UNICODE_SPACE_SEPARATOR:
            if (uc == 0x0020 || uc == 0x00A0 || uc == 0x1680 ||
                    (uc >= 0x2000 && uc <= 0x200A) ||
                    uc == 0x202F || uc == 0x205F || uc == 0x3000)
                sbt = SB_Sp;
            break;

        case G_UNICODE_LINE_SEPARATOR:
        case G_UNICODE_PARAGRAPH_SEPARATOR:
            sbt = SB_ParaSep;
            break;

        case G_UNICODE_FORMAT:
        case G_UNICODE_SPACING_MARK:
        case G_UNICODE_ENCLOSING_MARK:
        case G_UNICODE_NON_SPACING_MARK:
            sbt = SB_ExtendFormat; /* Extend, Format */
            break;

        case G_UNICODE_MODIFIER_LETTER:
            if (uc >= 0xFF9E && uc <= 0xFF9F)
                sbt = SB_ExtendFormat; /* Other_Grapheme_Extend */
            break;

        case G_UNICODE_TITLECASE_LETTER:
            sbt = SB_Upper;
            break;

        case G_UNICODE_DASH_PUNCTUATION:
            if (uc == 0x002D ||
                    (uc >= 0x2013 && uc <= 0x2014) ||
                    (uc >= 0xFE31 && uc <= 0xFE32) ||
                    uc == 0xFE58 || uc == 0xFE63 || uc == 0xFF0D)
                sbt = SB_SContinue;
            break;

        case G_UNICODE_OTHER_PUNCTUATION:
            if (uc == 0x05F3)
                sbt = SB_OLetter;
            else if (uc == 0x002E || uc == 0x2024 ||
                    uc == 0xFE52 || uc == 0xFF0E)
                sbt = SB_ATerm;

            if (uc == 0x002C || uc == 0x003A || uc == 0x055D ||
                    (uc >= 0x060C && uc <= 0x060D) ||
                    uc == 0x07F8 || uc == 0x1802 ||
                    uc == 0x1808 || uc == 0x3001 ||
                    (uc >= 0xFE10 && uc <= 0xFE11) ||
                    uc == 0xFE13 ||
                    (uc >= 0xFE50 && uc <= 0xFE51) ||
                    uc == 0xFE55 || uc == 0xFF0C ||
                    uc == 0xFF1A || uc == 0xFF64)
                sbt = SB_SContinue;

            if (uc == 0x0021 || uc == 0x003F ||
                    uc == 0x0589 || uc == 0x061F || uc == 0x06D4 ||
                    (uc >= 0x0700 && uc <= 0x0702) ||
                    uc == 0x07F9 ||
                    (uc >= 0x0964 && uc <= 0x0965) ||
                    (uc >= 0x104A && uc <= 0x104B) ||
                    uc == 0x1362 ||
                    (uc >= 0x1367 && uc <= 0x1368) ||
                    uc == 0x166E ||
                    (uc >= 0x1735 && uc <= 0x1736) ||
                    uc == 0x1803 || uc == 0x1809 ||
                    (uc >= 0x1944 && uc <= 0x1945) ||
                    (uc >= 0x1AA8 && uc <= 0x1AAB) ||
                    (uc >= 0x1B5A && uc <= 0x1B5B) ||
                    (uc >= 0x1B5E && uc <= 0x1B5F) ||
                    (uc >= 0x1C3B && uc <= 0x1C3C) ||
                    (uc >= 0x1C7E && uc <= 0x1C7F) ||
                    (uc >= 0x203C && uc <= 0x203D) ||
                    (uc >= 0x2047 && uc <= 0x2049) ||
                    uc == 0x2E2E || uc == 0x2E3C ||
                    uc == 0x3002 || uc == 0xA4FF ||
                    (uc >= 0xA60E && uc <= 0xA60F) ||
                    uc == 0xA6F3 || uc == 0xA6F7 ||
                    (uc >= 0xA876 && uc <= 0xA877) ||
                    (uc >= 0xA8CE && uc <= 0xA8CF) ||
                    uc == 0xA92F ||
                    (uc >= 0xA9C8 && uc <= 0xA9C9) ||
                    (uc >= 0xAA5D && uc <= 0xAA5F) ||
                    (uc >= 0xAAF0 && uc <= 0xAAF1) ||
                    uc == 0xABEB ||
                    (uc >= 0xFE56 && uc <= 0xFE57) ||
                    uc == 0xFF01 || uc == 0xFF1F || uc == 0xFF61 ||
                    (uc >= 0x10A56 && uc <= 0x10A57) ||
                    (uc >= 0x11047 && uc <= 0x11048) ||
                    (uc >= 0x110BE && uc <= 0x110C1) ||
                    (uc >= 0x11141 && uc <= 0x11143) ||
                    (uc >= 0x111C5 && uc <= 0x111C6) ||
                    uc == 0x111CD ||
                    (uc >= 0x111DE && uc <= 0x111DF) ||
                    (uc >= 0x11238 && uc <= 0x11239) ||
                    (uc >= 0x1123B && uc <= 0x1123C) ||
                    uc == 0x112A9 ||
                    (uc >= 0x1144B && uc <= 0x1144C) ||
                    (uc >= 0x115C2 && uc <= 0x115C3) ||
                    (uc >= 0x115C9 && uc <= 0x115D7) ||
                    (uc >= 0x11641 && uc <= 0x11642) ||
                    (uc >= 0x1173C && uc <= 0x1173E) ||
                    (uc >= 0x11C41 && uc <= 0x11C42) ||
                    (uc >= 0x16A6E && uc <= 0x16A6F) ||
                    uc == 0x16AF5 ||
                    (uc >= 0x16B37 && uc <= 0x16B38) ||
                    uc == 0x16B44 || uc == 0x1BC9F || uc == 0x1DA88)
                sbt = SB_STerm;
            break;
        }
    }

    if (sbt == SB_Other) {
        if (g_unichar_islower(uc))
            sbt = SB_Lower;
        else if (g_unichar_isupper(uc))
            sbt = SB_Upper;
        else if (g_unichar_isalpha(uc))
            sbt = SB_OLetter;

        if (gc == G_UNICODE_OPEN_PUNCTUATION ||
                gc == G_UNICODE_CLOSE_PUNCTUATION ||
                bt == G_UNICODE_BREAK_QUOTATION)
            sbt = SB_Close;
    }

    return sbt;
}

/* There are Hangul syllables encoded as characters, that act like a
 * sequence of Jamos. For each character we define a JamoType
 * that the character starts with, and one that it ends with.  This
 * decomposes JAMO_LV and JAMO_LVT to simple other JAMOs.  So for
 * example, a character with LineBreak type
 * G_UNICODE_BREAK_HANGUL_LV_SYLLABLE has start=JAMO_L and end=JAMO_V.
 */
struct _CharJamoProps {
    _JamoType start, end;
};

/* Map from JamoType to CharJamoProps that hold only simple
 * JamoTypes (no LV or LVT) or none.
 */
static const struct _CharJamoProps HangulJamoProps[] = {
    {JAMO_L, JAMO_L},   /* JAMO_L */
    {JAMO_V, JAMO_V},   /* JAMO_V */
    {JAMO_T, JAMO_T},   /* JAMO_T */
    {JAMO_L, JAMO_V},   /* JAMO_LV */
    {JAMO_L, JAMO_T},   /* JAMO_LVT */
    {NO_JAMO, NO_JAMO}  /* NO_JAMO */
};

/* A character forms a syllable with the previous character if and only if:
 * JamoType(this) is not NO_JAMO and:
 *
 * HangulJamoProps[JamoType(prev)].end and
 * HangulJamoProps[JamoType(this)].start are equal,
 * or the former is one less than the latter.
 */

#define IS_JAMO(btype)                          \
    ((btype >= G_UNICODE_BREAK_HANGUL_L_JAMO) &&    \
     (btype <= G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE))

#define JAMO_TYPE(btype)    \
    (IS_JAMO(btype) ? (btype - G_UNICODE_BREAK_HANGUL_L_JAMO) : NO_JAMO)

/* Determine wheter this forms a Hangul syllable with prev. */
static void check_hangul_syllable(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeBreakType bt)
{
    (void)uc;

    _JamoType jamo = JAMO_TYPE (bt);
    if (jamo == NO_JAMO)
        ctxt->makes_hangul_syllable = 0;
    else {
        _JamoType prev_end   = HangulJamoProps[ctxt->prev_jamo].end;
        _JamoType this_start = HangulJamoProps[jamo].start;

        /* See comments before IS_JAMO */
        ctxt->makes_hangul_syllable = (prev_end == this_start) ||
            (prev_end + 1 == this_start);
    }

    if (bt != G_UNICODE_BREAK_SPACE)
        ctxt->prev_jamo = jamo;
}

static uint16_t check_space(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeType gc)
{
    (void)ctxt;
    uint16_t bo;

    switch (gc) {
    case G_UNICODE_SPACE_SEPARATOR:
    case G_UNICODE_LINE_SEPARATOR:
    case G_UNICODE_PARAGRAPH_SEPARATOR:
        bo = FOIL_BOV_WHITESPACE;
        break;

    default:
        if (uc == '\t' || uc == '\n' || uc == '\r' || uc == '\f')
            bo = FOIL_BOV_WHITESPACE;
        else
            bo = FOIL_BOV_UNKNOWN;
        break;
    }

    /* Just few spaces have variable width. So explicitly mark them.
     */
    if (0x0020 == uc || 0x00A0 == uc) {
        bo |= FOIL_BOV_EXPANDABLE_SPACE;
    }

    if (uc != 0x00AD && (
            gc == G_UNICODE_NON_SPACING_MARK ||
            gc == G_UNICODE_ENCLOSING_MARK ||
            gc == G_UNICODE_FORMAT))
        bo |= FOIL_BOV_ZERO_WIDTH;
    else if ((uc >= 0x1160 && uc < 0x1200) || uc == 0x200B)
        bo |= FOIL_BOV_ZERO_WIDTH;

    return bo;
}

static inline
void check_emoji_extended_pictographic(struct break_ctxt* ctxt,
        uint32_t uc)
{
    ctxt->is_extended_pictographic =
        foil_uchar_is_extended_pictographic(uc);
}

/* Types of Japanese characters */
#define JAPANESE(uc) ((uc) >= 0x2F00 && (uc) <= 0x30FF)
#define KANJI(uc)    ((uc) >= 0x2F00 && (uc) <= 0x2FDF)
#define HIRAGANA(uc) ((uc) >= 0x3040 && (uc) <= 0x309F)
#define KATAKANA(uc) ((uc) >= 0x30A0 && (uc) <= 0x30FF)

#define LATIN(uc) (((uc) >= 0x0020 && (uc) <= 0x02AF) || ((uc) >= 0x1E00 && (uc) <= 0x1EFF))
#define CYRILLIC(uc) (((uc) >= 0x0400 && (uc) <= 0x052F))
#define GREEK(uc) (((uc) >= 0x0370 && (uc) <= 0x3FF) || ((uc) >= 0x1F00 && (uc) <= 0x1FFF))
#define KANA(uc) ((uc) >= 0x3040 && (uc) <= 0x30FF)
#define HANGUL(uc) ((uc) >= 0xAC00 && (uc) <= 0xD7A3)
#define BACKSPACE_DELETES_CHARACTER(uc) \
    (!LATIN (uc) && !CYRILLIC (uc) && !GREEK (uc) && !KANA(uc) && !HANGUL(uc))

static uint16_t check_grapheme_boundaries(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeType gc, GUnicodeBreakType bt)
{
    (void)bt;
    _GBType gbt;
    uint16_t bo;

    gbt = resolve_gbt(ctxt, uc, gc);

    /* Rule GB11 */
    if (ctxt->met_extended_pictographic) {
        if (gbt == GB_Extend)
            ctxt->met_extended_pictographic = 1;
        else if (foil_uchar_is_extended_pictographic(ctxt->prev_uc) &&
                gbt == GB_ZWJ)
            ctxt->met_extended_pictographic = 1;
        else if (ctxt->prev_gbt == GB_Extend && gbt == GB_ZWJ)
            ctxt->met_extended_pictographic = 1;
        else if (ctxt->prev_gbt == GB_ZWJ && ctxt->is_extended_pictographic)
            ctxt->met_extended_pictographic = 1;
        else
            ctxt->met_extended_pictographic = 0;
    }

    /* Grapheme Cluster Boundary Rules */
    ctxt->is_grapheme_boundary = 1; /* Rule GB999 */

    /* We apply Rules GB1 and GB2 at the upper level of the function */

    if (uc == '\n' && ctxt->prev_uc == '\r')
        ctxt->is_grapheme_boundary = 0; /* Rule GB3 */
    else if (ctxt->prev_gbt == GB_ControlCRLF || gbt == GB_ControlCRLF)
        ctxt->is_grapheme_boundary = 1; /* Rules GB4 and GB5 */
    else if (gbt == GB_InHangulSyllable)
        ctxt->is_grapheme_boundary = 0; /* Rules GB6, GB7, GB8 */
    else if (gbt == GB_Extend) {
        ctxt->is_grapheme_boundary = 0; /* Rule GB9 */
    }
    else if (gbt == GB_ZWJ)
        ctxt->is_grapheme_boundary = 0; /* Rule GB9 */
    else if (gbt == GB_SpacingMark)
        ctxt->is_grapheme_boundary = 0; /* Rule GB9a */
    else if (ctxt->prev_gbt == GB_Prepend)
        ctxt->is_grapheme_boundary = 0; /* Rule GB9b */
    else if (ctxt->is_extended_pictographic) {
        /* Rule GB11 */
        if (ctxt->prev_gbt == GB_ZWJ && ctxt->met_extended_pictographic)
            ctxt->is_grapheme_boundary = 0;
    }
    else if (ctxt->prev_gbt == GB_RI_Odd && gbt == GB_RI_Even)
        ctxt->is_grapheme_boundary = 0; /* Rule GB12 and GB13 */

    if (ctxt->is_extended_pictographic)
        ctxt->met_extended_pictographic = 1;

    bo = 0;
    if (ctxt->is_grapheme_boundary) {
        bo = FOIL_BOV_GB_CURSOR_POS;
    }

    /* If this is a grapheme boundary, we have to decide if backspace
     * deletes a character or the whole grapheme cluster */
    if (ctxt->is_grapheme_boundary) {
        bo |= FOIL_BOV_GB_CHAR_BREAK;
        if (BACKSPACE_DELETES_CHARACTER (ctxt->base_uc))
            bo |= FOIL_BOV_GB_BACKSPACE_DEL_CH;
    }

    ctxt->prev_gbt = gbt;

    return bo;
}

static uint16_t check_word_boundaries(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeType gc, GUnicodeBreakType bt, ssize_t i)
{
    ctxt->is_word_boundary = 0;

    /* Rules WB3 and WB4 */
    if (ctxt->is_grapheme_boundary ||
            (uc >= 0x1F1E6 && uc <= 0x1F1FF)) {

        _WBType wbt = resolve_wbt(ctxt, uc, gc, bt);

        /* We apply Rules WB1 and WB2 at the upper level of the function */

        if (ctxt->prev_uc == 0x3031 && uc == 0x41) {
            LOG_DEBUG ("%s: Y %d %d\n", __func__, ctxt->prev_wbt, wbt);
        }

        if (ctxt->prev_wbt == WB_NewlineCRLF &&
                ctxt->prev_wb_index + 1 == i) {
            /* The extra check for ctxt->prev_wb_index is to
             * correctly handle sequences like
             * Newline ÷ Extend × Extend
             * since we have not skipped ExtendFormat yet.
             */
            ctxt->is_word_boundary = 1; /* Rule WB3a */
        }
        else if (wbt == WB_NewlineCRLF)
            ctxt->is_word_boundary = 1; /* Rule WB3b */
        else if (ctxt->prev_uc == 0x200D &&
                ctxt->is_extended_pictographic)
            ctxt->is_word_boundary = 0; /* Rule WB3c */
        else if (ctxt->prev_wbt == WB_WSegSpace &&
                wbt == WB_WSegSpace && ctxt->prev_wb_index + 1 == i)
            ctxt->is_word_boundary = 0; /* Rule WB3d */
        else if (wbt == WB_ExtendFormat)
            ctxt->is_word_boundary = 0; /* Rules WB4? */
        else if ((ctxt->prev_wbt == WB_ALetter  ||
                    ctxt->prev_wbt == WB_Hebrew_Letter ||
                    ctxt->prev_wbt == WB_Numeric) &&
                (wbt == WB_ALetter  ||
                 wbt == WB_Hebrew_Letter ||
                 wbt == WB_Numeric))
            ctxt->is_word_boundary = 0; /* Rules WB5, WB8, WB9, WB10 */
        else if (ctxt->prev_wbt == WB_Katakana && wbt == WB_Katakana)
            ctxt->is_word_boundary = 0; /* Rule WB13 */
        else if ((ctxt->prev_wbt == WB_ALetter ||
                    ctxt->prev_wbt == WB_Hebrew_Letter ||
                    ctxt->prev_wbt == WB_Numeric ||
                    ctxt->prev_wbt == WB_Katakana ||
                    ctxt->prev_wbt == WB_ExtendNumLet) &&
                wbt == WB_ExtendNumLet)
            ctxt->is_word_boundary = 0; /* Rule WB13a */
        else if (ctxt->prev_wbt == WB_ExtendNumLet &&
                (wbt == WB_ALetter ||
                 wbt == WB_Hebrew_Letter ||
                 wbt == WB_Numeric ||
                 wbt == WB_Katakana))
            ctxt->is_word_boundary = 0; /* Rule WB13b */
        else if (((ctxt->prev_prev_wbt == WB_ALetter ||
                        ctxt->prev_prev_wbt == WB_Hebrew_Letter) &&
                    (wbt == WB_ALetter ||
                     wbt == WB_Hebrew_Letter)) &&
                (ctxt->prev_wbt == WB_MidLetter ||
                 ctxt->prev_wbt == WB_MidNumLet ||
                 ctxt->prev_uc == 0x0027))
        {
            /* Rule WB6 */
            ctxt->bos[ctxt->prev_wb_index - 1] &= ~FOIL_BOV_WB_WORD_BOUNDARY;
            ctxt->is_word_boundary = 0; /* Rule WB7 */
        }
        else if (ctxt->prev_wbt == WB_Hebrew_Letter && uc == 0x0027)
            ctxt->is_word_boundary = 0; /* Rule WB7a */
        else if (ctxt->prev_prev_wbt == WB_Hebrew_Letter &&
                ctxt->prev_uc == 0x0022 &&
                wbt == WB_Hebrew_Letter) {

            /* Rule WB7b */
            ctxt->bos[ctxt->prev_wb_index - 1] &= ~FOIL_BOV_WB_WORD_BOUNDARY;
            ctxt->is_word_boundary = 0; /* Rule WB7c */
        }
        else if ((ctxt->prev_prev_wbt == WB_Numeric &&
                wbt == WB_Numeric) &&
                (ctxt->prev_wbt == WB_MidNum ||
                    ctxt->prev_wbt == WB_MidNumLet ||
                    ctxt->prev_uc == 0x0027)) {
            ctxt->is_word_boundary = 0; /* Rule WB11 */

            /* Rule WB12 */
            ctxt->bos[ctxt->prev_wb_index - 1] &= ~FOIL_BOV_WB_WORD_BOUNDARY;
        }
        else if (ctxt->prev_wbt == WB_RI_Odd && wbt == WB_RI_Even)
            ctxt->is_word_boundary = 0; /* Rule WB15 and WB16 */
        else
            ctxt->is_word_boundary = 1; /* Rule WB999 */

        if (wbt != WB_ExtendFormat) {
            ctxt->prev_prev_wbt = ctxt->prev_wbt;
            ctxt->prev_wbt = wbt;
            ctxt->prev_wb_index = i;
        }
    }

    if (ctxt->is_word_boundary)
        return FOIL_BOV_WB_WORD_BOUNDARY;

    return 0;
}

#define IS_OTHER_TERM(sbt)                                      \
    /* not in (OLetter | Upper | Lower | ParaSep | SATerm) */   \
    !(sbt == SB_OLetter ||                                      \
            sbt == SB_Upper || sbt == SB_Lower ||               \
            sbt == SB_ParaSep ||                                \
            sbt == SB_ATerm || sbt == SB_STerm ||               \
            sbt == SB_ATerm_Close_Sp ||                         \
            sbt == SB_STerm_Close_Sp)

static uint16_t check_sentence_boundaries(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeType gc, GUnicodeBreakType bt, ssize_t i)
{
    ctxt->is_sentence_boundary = 0;

    /* Rules SB3 and SB5 */
    if (ctxt->is_word_boundary || uc == '\r' || uc == '\n') {

        _SBType sbt = resolve_sbt(ctxt, uc, gc, bt);

        /* Sentence Boundary Rules */

        /* We apply Rules SB1 and SB2 at the upper level of the function */

        if (uc == '\n' && ctxt->prev_uc == '\r')
            ctxt->is_sentence_boundary = 0; /* Rule SB3 */
        else if (ctxt->prev_sbt == SB_ParaSep &&
                ctxt->prev_sb_index + 1 == i) {
            /* The extra check for ctxt->prev_sb_index is to correctly
             * handle sequences like
             * ParaSep ÷ Extend × Extend
             * since we have not skipped ExtendFormat yet.
             */

            ctxt->is_sentence_boundary = 1; /* Rule SB4 */
        }
        else if (sbt == SB_ExtendFormat)
            ctxt->is_sentence_boundary = 0; /* Rule SB5? */
        else if (ctxt->prev_sbt == SB_ATerm && sbt == SB_Numeric)
            ctxt->is_sentence_boundary = 0; /* Rule SB6 */
        else if ((ctxt->prev_prev_sbt == SB_Upper ||
                    ctxt->prev_prev_sbt == SB_Lower) &&
                ctxt->prev_sbt == SB_ATerm &&
                sbt == SB_Upper)
            ctxt->is_sentence_boundary = 0; /* Rule SB7 */
        else if (ctxt->prev_sbt == SB_ATerm && sbt == SB_Close)
            sbt = SB_ATerm;
        else if (ctxt->prev_sbt == SB_STerm && sbt == SB_Close)
            sbt = SB_STerm;
        else if (ctxt->prev_sbt == SB_ATerm && sbt == SB_Sp)
            sbt = SB_ATerm_Close_Sp;
        else if (ctxt->prev_sbt == SB_STerm && sbt == SB_Sp)
            sbt = SB_STerm_Close_Sp;
        /* Rule SB8 */
        else if ((ctxt->prev_sbt == SB_ATerm ||
                ctxt->prev_sbt == SB_ATerm_Close_Sp) &&
                sbt == SB_Lower)
            ctxt->is_sentence_boundary = 0;
        else if ((ctxt->prev_prev_sbt == SB_ATerm ||
                ctxt->prev_prev_sbt == SB_ATerm_Close_Sp) &&
                IS_OTHER_TERM(ctxt->prev_sbt) &&
                sbt == SB_Lower)
            ctxt->bos[ctxt->prev_sb_index - 1] &= ~FOIL_BOV_SB_SENTENCE_BOUNDARY;
        else if ((ctxt->prev_sbt == SB_ATerm ||
                    ctxt->prev_sbt == SB_ATerm_Close_Sp ||
                    ctxt->prev_sbt == SB_STerm ||
                    ctxt->prev_sbt == SB_STerm_Close_Sp) &&
                (sbt == SB_SContinue ||
                    sbt == SB_ATerm || sbt == SB_STerm))
            ctxt->is_sentence_boundary = 0; /* Rule SB8a */
        else if ((ctxt->prev_sbt == SB_ATerm ||
                    ctxt->prev_sbt == SB_STerm) &&
                (sbt == SB_Close || sbt == SB_Sp ||
                    sbt == SB_ParaSep))
            ctxt->is_sentence_boundary = 0; /* Rule SB9 */
        else if ((ctxt->prev_sbt == SB_ATerm ||
                    ctxt->prev_sbt == SB_ATerm_Close_Sp ||
                    ctxt->prev_sbt == SB_STerm ||
                    ctxt->prev_sbt == SB_STerm_Close_Sp) &&
                (sbt == SB_Sp || sbt == SB_ParaSep))
            ctxt->is_sentence_boundary = 0; /* Rule SB10 */
        else if ((ctxt->prev_sbt == SB_ATerm ||
                    ctxt->prev_sbt == SB_ATerm_Close_Sp ||
                    ctxt->prev_sbt == SB_STerm ||
                    ctxt->prev_sbt == SB_STerm_Close_Sp) &&
                sbt != SB_ParaSep)
            ctxt->is_sentence_boundary = 1; /* Rule SB11 */
        else
            ctxt->is_sentence_boundary = 0; /* Rule SB998 */

        if (sbt != SB_ExtendFormat &&
                !((ctxt->prev_prev_sbt == SB_ATerm ||
                    ctxt->prev_prev_sbt == SB_ATerm_Close_Sp) &&
                IS_OTHER_TERM(ctxt->prev_sbt) &&
                IS_OTHER_TERM(sbt))) {
            ctxt->prev_prev_sbt = ctxt->prev_sbt;
            ctxt->prev_sbt = sbt;
            ctxt->prev_sb_index = i;
        }
    }

    if (i == 1)
        ctxt->is_sentence_boundary = 1; /* Rules SB1 and SB2 */

    if (ctxt->is_sentence_boundary)
        return FOIL_BOV_SB_SENTENCE_BOUNDARY;

    return 0;
}

#undef IS_OTHER_TERM

/* ---- Word breaks ---- */
static void check_word_breaks(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeType gc, size_t i)
{
    /* default to not a word start/end */
    ctxt->bos[i] &= ~FOIL_BOV_WB_WORD_START;
    ctxt->bos[i] &= ~FOIL_BOV_WB_WORD_END;

    if (ctxt->curr_wt != WordNone) {
        /* Check for a word end */
        switch (gc) {
        case G_UNICODE_SPACING_MARK:
        case G_UNICODE_ENCLOSING_MARK:
        case G_UNICODE_NON_SPACING_MARK:
        case G_UNICODE_FORMAT:
            /* nothing, we just eat these up as part of the word */
            break;

        case G_UNICODE_LOWERCASE_LETTER:
        case G_UNICODE_MODIFIER_LETTER:
        case G_UNICODE_OTHER_LETTER:
        case G_UNICODE_TITLECASE_LETTER:
        case G_UNICODE_UPPERCASE_LETTER:
            if (ctxt->curr_wt == WordLetters) {
                /* Japanese special cases for ending the word */
                if (JAPANESE (ctxt->last_word_letter) ||
                        JAPANESE (uc)) {
                    if ((HIRAGANA (ctxt->last_word_letter) &&
                                !HIRAGANA (uc)) ||
                            (KATAKANA (ctxt->last_word_letter) &&
                             !(KATAKANA (uc) || HIRAGANA (uc))) ||
                            (KANJI (ctxt->last_word_letter) &&
                             !(HIRAGANA (uc) || KANJI (uc))) ||
                            (JAPANESE (ctxt->last_word_letter) &&
                             !JAPANESE (uc)) ||
                            (!JAPANESE (ctxt->last_word_letter) &&
                             JAPANESE (uc)))
                        ctxt->bos[i] |= FOIL_BOV_WB_WORD_END;
                }
            }
            ctxt->last_word_letter = uc;
            break;

        case G_UNICODE_DECIMAL_NUMBER:
        case G_UNICODE_LETTER_NUMBER:
        case G_UNICODE_OTHER_NUMBER:
            ctxt->last_word_letter = uc;
            break;

        default:
            /* Punctuation, control/format chars, etc. all end a word. */
            ctxt->bos[i] |= FOIL_BOV_WB_WORD_END;
            ctxt->curr_wt = WordNone;
            break;
        }
    }
    else {
        /* Check for a word start */
        switch (gc) {
        case G_UNICODE_LOWERCASE_LETTER:
        case G_UNICODE_MODIFIER_LETTER:
        case G_UNICODE_OTHER_LETTER:
        case G_UNICODE_TITLECASE_LETTER:
        case G_UNICODE_UPPERCASE_LETTER:
            ctxt->curr_wt = WordLetters;
            ctxt->last_word_letter = uc;
            ctxt->bos[i] |= FOIL_BOV_WB_WORD_START;
            break;

        case G_UNICODE_DECIMAL_NUMBER:
        case G_UNICODE_LETTER_NUMBER:
        case G_UNICODE_OTHER_NUMBER:
            ctxt->curr_wt = WordNumbers;
            ctxt->last_word_letter = uc;
            ctxt->bos[i] |= FOIL_BOV_WB_WORD_START;
            break;

        default:
            /* No word here */
            break;
        }
    }
}

/* ---- Sentence breaks ---- */
static void check_sentence_breaks(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeType gc, ssize_t i)
{
    (void)uc;
    (void)gc;

    /* default to not a sentence start/end */
    ctxt->bos[i] &= ~FOIL_BOV_SB_SENTENCE_START;
    ctxt->bos[i] &= ~FOIL_BOV_SB_SENTENCE_END;

    /* maybe start sentence */
    if (ctxt->last_stc_start == -1 && !ctxt->is_sentence_boundary)
        ctxt->last_stc_start = i - 1;

    /* remember last non space character position */
    if (i > 0 && !(ctxt->bos[i - 1] & FOIL_BOV_WHITESPACE))
        ctxt->last_non_space = i;

    /* meets sentence end, mark both sentence start and end */
    if (ctxt->last_stc_start != -1 && ctxt->is_sentence_boundary) {
        if (ctxt->last_non_space != -1) {
            ctxt->bos[ctxt->last_stc_start] |= FOIL_BOV_SB_SENTENCE_START;
            ctxt->bos[ctxt->last_non_space] |= FOIL_BOV_SB_SENTENCE_END;
        }

        ctxt->last_stc_start = -1;
        ctxt->last_non_space = -1;
    }

    /* meets space character, move sentence start */
    if (ctxt->last_stc_start != -1 &&
            ctxt->last_stc_start == i - 1 &&
            (ctxt->bos[i - 1] & FOIL_BOV_WHITESPACE))
        ctxt->last_stc_start++;
}

#ifndef NDEBUG
static void dbg_dump_ctxt(struct break_ctxt* ctxt,
        const char* func, uint32_t uc, uint16_t gwsbo)
{
    LOG_DEBUG("After calling %s (%06X):\n"
        "\tmakes_hangul_syllable: %d (prev_jamo: %d)\n"
        "\tmet_extended_pictographic: %d\n"
        "\tis_extended_pictographic: %d\n"
        "\tis_grapheme_boundary: %d\n"
        "\tis_word_boundary: %d\n"
        "\tis_sentence_boundary: %d\n"
        "\tFOIL_BOV_WHITESPACE: %s\n"
        "\tFOIL_BOV_EXPANDABLE_SPACE: %s\n"
        "\tFOIL_BOV_GB_CHAR_BREAK: %s\n"
        "\tFOIL_BOV_GB_CURSOR_POS: %s\n"
        "\tFOIL_BOV_GB_BACKSPACE_DEL_CH: %s\n"
        "\tFOIL_BOV_WB_WORD_BOUNDARY: %s\n"
        "\tFOIL_BOV_WB_WORD_START: %s\n"
        "\tFOIL_BOV_WB_WORD_END: %s\n"
        "\tFOIL_BOV_SB_SENTENCE_BOUNDARY: %s\n"
        "\tFOIL_BOV_SB_SENTENCE_START: %s\n"
        "\tFOIL_BOV_SB_SENTENCE_END: %s\n",
        func, uc,
        ctxt->makes_hangul_syllable, ctxt->prev_jamo,
        ctxt->met_extended_pictographic,
        ctxt->is_extended_pictographic,
        ctxt->is_grapheme_boundary,
        ctxt->is_word_boundary,
        ctxt->is_sentence_boundary,
        (gwsbo & FOIL_BOV_WHITESPACE)?"true":"false",
        (gwsbo & FOIL_BOV_EXPANDABLE_SPACE)?"true":"false",
        (gwsbo & FOIL_BOV_GB_CHAR_BREAK)?"true":"false",
        (gwsbo & FOIL_BOV_GB_CURSOR_POS)?"true":"false",
        (gwsbo & FOIL_BOV_GB_BACKSPACE_DEL_CH)?"true":"false",
        (gwsbo & FOIL_BOV_WB_WORD_BOUNDARY)?"true":"false",
        (gwsbo & FOIL_BOV_WB_WORD_START)?"true":"false",
        (gwsbo & FOIL_BOV_WB_WORD_END)?"true":"false",
        (gwsbo & FOIL_BOV_SB_SENTENCE_BOUNDARY)?"true":"false",
        (gwsbo & FOIL_BOV_SB_SENTENCE_START)?"true":"false",
        (gwsbo & FOIL_BOV_SB_SENTENCE_END)?"true":"false");
}
#else
#define dbg_dump_ctxt(ctxt, func, uc, gwsbo)
#endif

#define LOCAL_ARRAY_SIZE 256

static size_t break_init_spaces(struct break_ctxt* ctxt,
        uint32_t* ucs, uint16_t* bos, uint8_t* local_bts, uint8_t* local_ods,
        size_t nr_ucs)
{
    ctxt->ucs = ucs;
    ctxt->nr_ucs = nr_ucs;

    if (nr_ucs > LOCAL_ARRAY_SIZE) {
        ctxt->bts = (uint8_t*)malloc(sizeof(uint8_t) * ctxt->nr_ucs);
        ctxt->ods = (uint8_t*)malloc(sizeof(uint8_t) * (ctxt->nr_ucs + 1));
    }
    else {
        ctxt->bts = local_bts;
        ctxt->ods = local_ods;
    }

    if (bos)
        ctxt->bos = bos;
    else
        ctxt->bos = (uint16_t*)malloc(sizeof(uint16_t) * (ctxt->nr_ucs + 1));

    if (ctxt->bts == NULL || ctxt->bos == NULL || ctxt->ods == NULL)
        return 0;

    return ctxt->nr_ucs;
}

static size_t break_push_back(struct break_ctxt* ctxt,
        uint32_t uc, GUnicodeBreakType bt, uint16_t lbo)
{
    if (ctxt->n == 0) {
        // set the before line break opportunity
        ctxt->bos[0] = lbo;
        ctxt->ods[0] = LBLAST;
    }
    else {
        // break opportunities for grapheme, word, and sentence.
        GUnicodeType gc;
        uint16_t gwsbo = 0;

        // set the after line break opportunity
        ctxt->bts[ctxt->n - 1] = bt;
        ctxt->bos[ctxt->n] = lbo;
        if (lbo == FOIL_BOV_UNKNOWN)
            ctxt->ods[ctxt->n] = LBLAST;
        else
            ctxt->ods[ctxt->n] = ctxt->curr_od;

        // determine the grapheme, word, and sentence breaks
        gc = g_unichar_type(uc);
        // use the original breaking class for GWS breaking test.
        bt = g_unichar_break_type(uc);

        check_hangul_syllable(ctxt, uc, bt);
        //dbg_dump_ctxt(ctxt, "check_hangul_syllable", uc, gwsbo);

        ctxt->bos[ctxt->n] |= check_space(ctxt, uc, gc);
        //dbg_dump_ctxt(ctxt, "check_space", uc, gwsbo);

        check_emoji_extended_pictographic(ctxt, uc);
        //dbg_dump_ctxt(ctxt, "check_extended_pictographic", uc, gwsbo);

        gwsbo |= check_grapheme_boundaries(ctxt, uc, gc, bt);
        //dbg_dump_ctxt(ctxt, "check_grapheme_boundaries", uc, gwsbo);

        gwsbo |= check_word_boundaries(ctxt, uc, gc, bt, ctxt->n);
        //dbg_dump_ctxt(ctxt, "check_word_boundaries", uc, gwsbo);

        gwsbo |= check_sentence_boundaries(ctxt, uc, gc, bt, ctxt->n);
        //dbg_dump_ctxt(ctxt, "check_sentence_boundaries", uc, gwsbo);

        ctxt->bos[ctxt->n - 1] |= gwsbo;

        check_word_breaks(ctxt, uc, gc, ctxt->n);
        //dbg_dump_ctxt(ctxt, "check_word_breaks", uc, gwsbo);

        check_sentence_breaks(ctxt, uc, gc, ctxt->n);
        dbg_dump_ctxt(ctxt, "check_sentence_breaks", uc, gwsbo);

        // Character Transformation
        // NOTE: Assume character transformation will not affect the breaks
        if (ctxt->ctr && (is_letter(gc, bt) || uc == 0x0020)) {
            uint32_t new_uc = uc;

            switch(ctxt->ctr & FOIL_CTR_CASE_MASK) {
            case FOIL_CTR_UPPERCASE:
                new_uc = g_unichar_toupper(uc);
                break;

            case FOIL_CTR_LOWERCASE:
                new_uc = g_unichar_tolower(uc);
                break;

            case FOIL_CTR_CAPITALIZE:
                if (ctxt->bos[ctxt->n] & FOIL_BOV_WB_WORD_START)
                    new_uc = g_unichar_toupper(uc);
                break;
            }

            if (ctxt->ctr & FOIL_CTR_FULL_WIDTH) {
                new_uc = foil_uchar_to_fullwidth(new_uc);
            }

            if (ctxt->ctr & FOIL_CTR_FULL_SIZE_KANA) {
                new_uc = foil_uchar_to_fullsize_kana(new_uc);
            }

            if (new_uc != uc)
                ctxt->ucs[ctxt->n - 1] = new_uc;
        }

        ctxt->prev_uc = uc;

        /* uc might not be a valid Unicode base character, but really all we
         * need to know is the last non-combining character */
        if (gc != G_UNICODE_SPACING_MARK &&
                gc != G_UNICODE_ENCLOSING_MARK &&
                gc != G_UNICODE_NON_SPACING_MARK)
            ctxt->base_uc = uc;
    }

    ctxt->n++;
    return ctxt->n;
}

static inline size_t break_next_uchar(struct break_ctxt* ctxt,
        const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t* uc)
{
    (void)ctxt;

    if (nr_left_ucs > 0) {
        *uc = *ucs_left;
        return 1;
    }

    return 0;
}

static size_t check_uchars_following_zw(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs)
{
    size_t consumed = 0;

    do {
        size_t uclen;
        uint32_t uc;

        uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, &uc);
        if (uclen > 0) {
            ucs_left += uclen;
            nr_left_ucs -= uclen;

            if (resolve_lbc(ctxt, uc) == G_UNICODE_BREAK_SPACE) {
                consumed += uclen;
                break_change_lbo_last(ctxt, FOIL_BOV_LB_NOTALLOWED);
                break_push_back(ctxt, uc, G_UNICODE_BREAK_SPACE,
                    FOIL_BOV_LB_NOTALLOWED);
            }
            else {
                break;
            }
        }
        else
            break;

    } while (true);

    return consumed;
}

static size_t is_next_uchar_bt(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t* uc,
    GUnicodeBreakType bt)
{
    size_t uclen;

    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0 && resolve_lbc(ctxt, *uc) == bt)
        return uclen;

    return 0;
}

static size_t is_next_uchar_letter(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t* uc,
    GUnicodeBreakType* pbt)
{
    size_t uclen;
    GUnicodeBreakType bt;

    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        bt = resolve_lbc(ctxt, *uc);
        if (is_letter(ctxt->curr_gc, bt)) {
            if (pbt) *pbt = bt;
            return uclen;
        }
    }

    return 0;
}

static inline size_t is_next_uchar_lf(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t* uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
            G_UNICODE_BREAK_LINE_FEED);
}

static inline size_t is_next_uchar_sp(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t* uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
            G_UNICODE_BREAK_SPACE);
}

static inline size_t is_next_uchar_gl(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t* uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_NON_BREAKING_GLUE);
}

static inline size_t is_next_uchar_hl(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_HEBREW_LETTER);
}

static inline size_t is_next_uchar_in(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_INSEPARABLE);
}

static inline size_t is_next_uchar_nu(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_NUMERIC);
}

static inline size_t is_next_uchar_po(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_POSTFIX);
}

static inline size_t is_next_uchar_pr(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_PREFIX);
}

static inline size_t is_next_uchar_op(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_OPEN_PUNCTUATION);
}

static inline size_t is_next_uchar_jt(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_HANGUL_T_JAMO);
}

static inline size_t is_next_uchar_ri(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_REGIONAL_INDICATOR);
}

static inline size_t is_next_uchar_em(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    return is_next_uchar_bt(ctxt, ucs_left, nr_left_ucs, uc,
        G_UNICODE_BREAK_EMOJI_MODIFIER);
}

static size_t is_next_uchar_cm_zwj(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t* uc,
    GUnicodeBreakType* pbt)
{
    size_t uclen;

    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = g_unichar_break_type(*uc);
        if (bt == G_UNICODE_BREAK_COMBINING_MARK
                || bt == G_UNICODE_BREAK_ZERO_WIDTH_JOINER) {
            if (pbt) *pbt = bt;
            return uclen;
        }
    }

    return 0;
}

static size_t is_next_uchar_hy_ba(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc,
    GUnicodeBreakType* pbt)
{
    size_t uclen;

    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_HYPHEN
                || bt == G_UNICODE_BREAK_AFTER) {
            if (pbt) *pbt = bt;
            return uclen;
        }
    }

    return 0;
}

static size_t is_next_uchar_al_hl(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    size_t uclen;
    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_HEBREW_LETTER || bt == G_UNICODE_BREAK_ALPHABETIC)
            return uclen;
    }

    return 0;
}

static size_t is_next_uchar_pr_po(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    size_t uclen;
    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_PREFIX || bt == G_UNICODE_BREAK_POSTFIX)
            return uclen;
    }

    return 0;
}

static size_t is_next_uchar_id_eb_em(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    size_t uclen;
    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_IDEOGRAPHIC
                || bt == G_UNICODE_BREAK_EMOJI_BASE
                || bt == G_UNICODE_BREAK_EMOJI_MODIFIER)
            return uclen;
    }

    return 0;
}

static size_t is_next_uchar_jl_jv_jt_h2_h3(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc,
    GUnicodeBreakType* pbt)
{
    size_t uclen;
    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_HANGUL_L_JAMO
                || bt == G_UNICODE_BREAK_HANGUL_V_JAMO
                || bt == G_UNICODE_BREAK_HANGUL_T_JAMO
                || bt == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE
                || bt == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE) {
            if (pbt) *pbt = bt;
            return uclen;
        }
    }

    return 0;
}

static size_t is_next_uchar_jl_jv_h2_h3(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc, GUnicodeBreakType* pbt)
{
    size_t uclen;
    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_HANGUL_L_JAMO
                || bt == G_UNICODE_BREAK_HANGUL_V_JAMO
                || bt == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE
                || bt == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE) {
            if (pbt) *pbt = bt;
            return uclen;
        }
    }

    return 0;
}

static size_t is_next_uchar_jv_jt(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc,
    GUnicodeBreakType* pbt)
{
    size_t uclen;
    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_HANGUL_V_JAMO
                || bt == G_UNICODE_BREAK_HANGUL_T_JAMO) {
            if (pbt) *pbt = bt;
            return uclen;
        }
    }

    return 0;
}

static size_t is_next_uchar_al_hl_nu(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    size_t uclen;
    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_HEBREW_LETTER
                || bt == G_UNICODE_BREAK_ALPHABETIC
                || bt == G_UNICODE_BREAK_NUMERIC)
            return uclen;
    }

    return 0;
}

static size_t is_next_uchar_cl_cp_is_sy(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    size_t uclen;
    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_CLOSE_PUNCTUATION
                || bt == G_UNICODE_BREAK_CLOSE_PARANTHESIS
                || bt == G_UNICODE_BREAK_INFIX_SEPARATOR
                || bt == G_UNICODE_BREAK_SYMBOL)
            return uclen;
    }

    return 0;
}

static size_t is_next_uchar_nu_OR_op_hy_followed_nu(
    struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    size_t uclen;
    size_t next_uclen;
    uint32_t next_uc;

    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_NUMERIC) {
            return uclen;
        }
        else if ((bt == G_UNICODE_BREAK_OPEN_PUNCTUATION
                    || bt == G_UNICODE_BREAK_HYPHEN)
                && (next_uclen = is_next_uchar_nu(ctxt,
                    ucs_left + uclen, nr_left_ucs - uclen, &next_uc)) > 0) {
            return uclen + next_uclen;
        }
    }

    return 0;
}

static size_t is_next_uchar_nu_sy_is(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, uint32_t *uc)
{
    size_t uclen;
    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, uc);
    if (uclen > 0) {
        GUnicodeBreakType bt = resolve_lbc(ctxt, *uc);
        if (bt == G_UNICODE_BREAK_NUMERIC
                || bt == G_UNICODE_BREAK_SYMBOL
                || bt == G_UNICODE_BREAK_INFIX_SEPARATOR)
            return uclen;
    }

    return 0;
}

static bool are_prev_uchars_nu_AND_nu_sy_is(
        const struct break_ctxt* ctxt, bool before_last)
{
    size_t last, i;

    if (before_last) {
        if (ctxt->n < 4)
            return false;
        last = ctxt->n - 4;
    }
    else {
        if (ctxt->n < 3)
            return false;
        last = ctxt->n - 3;
    }

    LOG_DEBUG("%s: break type of last (%d/%d): %d\n",
            __FUNCTION__, last, ctxt->n, ctxt->bts[last]);
    if (ctxt->bts[last] == G_UNICODE_BREAK_NUMERIC)
        return true;

    i = last;
    while (i > 0) {
        GUnicodeBreakType bt = ctxt->bts[i];
        if (bt == G_UNICODE_BREAK_NUMERIC
                || bt == G_UNICODE_BREAK_SYMBOL
                || bt == G_UNICODE_BREAK_INFIX_SEPARATOR)
            i--;
        else
            break;
    }

    LOG_DEBUG("%s: break type of first (%d/%d): %d\n",
            __FUNCTION__, i, ctxt->n, ctxt->bts[i]);
    if (i == last)
        return false;

    if (ctxt->bts[i + 1] == G_UNICODE_BREAK_NUMERIC) {
        return true;
    }

    return false;
}

static bool are_prev_uchars_nu_AND_nu_sy_is_AND_cl_cp(
    const struct break_ctxt* ctxt)
{
    size_t last = ctxt->n - 3;
    GUnicodeBreakType bt = ctxt->bts[last];

    if (bt == G_UNICODE_BREAK_CLOSE_PUNCTUATION
            || bt == G_UNICODE_BREAK_CLOSE_PARANTHESIS) {
        return are_prev_uchars_nu_AND_nu_sy_is(ctxt, true);
    }

    return are_prev_uchars_nu_AND_nu_sy_is(ctxt, false);
}

#if 0
static bool is_next_uchar_zw(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs)
{
    size_t uclen;
    uint32_t uc;

    uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, &uc);
    if (uclen > 0 && resolve_lbc(ctxt, uc)
            == G_UNICODE_BREAK_ZERO_WIDTH_SPACE)
        return true;

    return false;
}
#endif

static size_t check_subsequent_cm_zwj(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs)
{
    size_t consumed = 0;
    size_t uclen;
    uint32_t uc;
    GUnicodeBreakType bt;

    while ((uclen = is_next_uchar_cm_zwj(ctxt, ucs_left, nr_left_ucs,
            &uc, &bt)) > 0) {

        // CM/ZWJ should have the same break class as
        // its base character.
        break_push_back(ctxt, uc, ctxt->base_bt, FOIL_BOV_LB_NOTALLOWED);

        ucs_left += uclen;
        nr_left_ucs -= uclen;
        consumed += uclen;
    }

    return consumed;
}

static size_t check_subsequent_sp(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs)
{
    size_t consumed = 0;
    size_t uclen;
    uint32_t uc;

    while ((uclen = is_next_uchar_sp(ctxt, ucs_left, nr_left_ucs, &uc)) > 0) {
        break_push_back(ctxt, uc, G_UNICODE_BREAK_SPACE, FOIL_BOV_LB_NOTALLOWED);
        ucs_left += uclen;
        nr_left_ucs -= uclen;
        consumed += uclen;
    }

    return consumed;
}

static size_t is_subsequent_sps_and_end_bt(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, GUnicodeBreakType end_bt)
{
    uint32_t uc;
    size_t uclen;

    while (nr_left_ucs > 0 && *ucs_left != '\0') {
        uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, &uc);
        if (uclen > 0) {
            GUnicodeBreakType bt = resolve_lbc(ctxt, uc);
            if (bt == G_UNICODE_BREAK_SPACE) {
                ucs_left += uclen;
                nr_left_ucs -= uclen;
                continue;
            }
            else if (bt == end_bt) {
                return true;
            }
        }

        break;
    }

    return false;
}

static size_t check_subsequent_sps_and_end_bt(struct break_ctxt* ctxt,
    const uint32_t* ucs_left, size_t nr_left_ucs, bool col_sp, GUnicodeBreakType end_bt)
{
    size_t consumed = 0;
    size_t uclen;
    uint32_t uc;

    while (nr_left_ucs > 0 && *ucs_left != '\0') {
        uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, &uc);
        if (uclen > 0) {
            GUnicodeBreakType bt = resolve_lbc(ctxt, uc);
            LOG_DEBUG("%s: %04X (%d)\n", __FUNCTION__, uc, bt);
            if (bt == G_UNICODE_BREAK_SPACE) {
                ucs_left += uclen;
                nr_left_ucs -= uclen;
                consumed += uclen;
                if (!col_sp)
                    break_push_back(ctxt, uc, bt, FOIL_BOV_LB_NOTALLOWED);
                continue;
            }
            else if (bt == end_bt) {
                return consumed;
            }
        }

        break;
    }

    return consumed;
}

#if 0
static bool is_odd_nubmer_of_subsequent_ri(struct break_ctxt* ctxt,
        const uint32_t* ucs_left, size_t nr_left_ucs)
{
    size_t nr = 0;
    size_t uclen;
    uint32_t uc;

    while (nr_left_ucs > 0 && *ucs_left != '\0') {
        uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, &uc);
        ucs_left += uclen;
        nr_left_ucs -= uclen;

        if (uclen > 0 && resolve_lbc(ctxt, uc)
                    == G_UNICODE_BREAK_REGIONAL_INDICATOR) {
            nr++;
            continue;
        }
        else
            break;
    }

    if (nr > 0 && nr % 2 != 0)
        return true;

    return false;
}
#endif

static bool is_even_nubmer_of_subsequent_ri(struct break_ctxt* ctxt,
        const uint32_t* ucs_left, size_t nr_left_ucs)
{
    size_t nr = 0;
    size_t uclen;
    uint32_t uc;

    while (nr_left_ucs > 0 && *ucs_left != '\0') {
        uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, &uc);
        ucs_left += uclen;
        nr_left_ucs -= uclen;

        if (uclen > 0 && resolve_lbc(ctxt, uc)
                == G_UNICODE_BREAK_REGIONAL_INDICATOR) {
            nr++;
            continue;
        }
        else
            break;
    }

    if (nr > 0 && nr % 2 == 0)
        return true;

    return false;
}

static size_t check_subsequent_ri(struct break_ctxt* ctxt,
        const uint32_t* ucs_left, size_t nr_left_ucs)
{
    size_t consumed = 0;
    size_t uclen;
    uint32_t uc;

    while (nr_left_ucs > 0 && *ucs_left != '\0') {
        uclen = break_next_uchar(ctxt, ucs_left, nr_left_ucs, &uc);
        if (uclen > 0 && resolve_lbc(ctxt, uc)
                    == G_UNICODE_BREAK_REGIONAL_INDICATOR) {
            ucs_left += uclen;
            nr_left_ucs -= uclen;
            consumed += uclen;

            break_push_back(ctxt, uc,
                G_UNICODE_BREAK_REGIONAL_INDICATOR, FOIL_BOV_LB_NOTALLOWED);
            continue;
        }
        else
            break;
    }

    return consumed;
}

/*
** Reference:
**
**  [UNICODE TEXT SEGMENTATION](https://www.unicode.org/reports/tr29/)
**  [UNICODE LINE BREAKING ALGORITHM](https://www.unicode.org/reports/tr14/)
**  [CSS Text Module Level 3](https://www.w3.org/TR/css-text-3/#content-writing-system)
*/
size_t foil_ustr_get_breaks(foil_langcode_t lang_code,
            uint8_t ctr, uint8_t wbr, uint8_t lbp,
            uint32_t* ucs, size_t nr_ucs, uint16_t** break_oppos)
{
    struct break_ctxt ctxt;
    uint8_t local_bts [LOCAL_ARRAY_SIZE];
    uint8_t local_ods [LOCAL_ARRAY_SIZE + 1];
    uint32_t* ucs_left;
    size_t nr_left_ucs;

    memset(&ctxt, 0, sizeof(ctxt));

    ctxt.base_bt = G_UNICODE_BREAK_UNSET;
    ctxt.lc = lang_code;
    ctxt.ctr = ctr;
    ctxt.wbr = wbr;
    ctxt.lbp = lbp;

    // NOTE: the index 0 of break_oppos is the break opportunity
    // before the first uchar.
    ctxt.last_stc_start = -1;
    ctxt.last_non_space = -1;
    ctxt.prev_wb_index = -1;
    ctxt.prev_sb_index = -1;

    ctxt.prev_gbt = GB_Other;
    ctxt.prev_gbt = GB_Other;
    ctxt.prev_wbt = ctxt.prev_prev_wbt = WB_Other;
    ctxt.prev_sbt = ctxt.prev_prev_sbt = SB_Other;
    ctxt.prev_jamo = NO_JAMO;
    ctxt.curr_wt = WordNone;

    if (ucs == NULL || nr_ucs == 0)
        return 0;

    if (break_init_spaces(&ctxt, ucs, *break_oppos, local_bts, local_ods,
            nr_ucs) <= 0) {
        goto error;
    }

    ucs_left = ucs;
    nr_left_ucs = nr_ucs;
    while (true) {
        uint32_t uc;
        size_t uclen = 0;
        GUnicodeBreakType bt;
        GUnicodeType gc;
        // line break opportunity
        uint16_t lbo;

        uint32_t next_uc;
        GUnicodeBreakType next_bt;
        size_t next_uclen;
        size_t consumed_one_loop = 0;

        uclen = break_next_uchar(&ctxt, ucs_left, nr_left_ucs, &uc);
        if (uclen == 0) {
            // badly encoded or end of text
            break;
        }
        ucs_left += uclen;
        nr_left_ucs -= uclen;

        ctxt.base_bt = G_UNICODE_BREAK_UNSET;

        LOG_DEBUG ("Got a uchar: %04X\n", uc);

        /*
         * UNICODE LINE BREAKING ALGORITHM
         */

        // LB1 Resolve line breaking class
        ctxt.curr_od = LB1;
        bt = resolve_lbc(&ctxt, uc);
        gc = ctxt.curr_gc;

        /* Start and end of text */
        // LB2 Never break at the start of text.
        if (ctxt.n == 0) {
            LOG_DEBUG ("LB2 Never break at the start of text\n");
            ctxt.curr_od = LB2;
            if (break_push_back(&ctxt, 0, 0, FOIL_BOV_LB_NOTALLOWED) == 0)
                goto error;
        }

        // Set the default breaking manner is not set.
        ctxt.curr_od = LBLAST;
        lbo = FOIL_BOV_UNKNOWN;

        // Set default break opportunity of the current uchar
        if (break_push_back(&ctxt, uc, bt, lbo) == 0)
            goto error;

        // LB3 Always break at the end of text.
        if (break_next_uchar(&ctxt, ucs_left, nr_left_ucs, &next_uc) == 0) {
            LOG_DEBUG ("LB3 Always break at the end of text\n");
            ctxt.curr_od = LB3;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_MANDATORY);
        }

        /* Mandatory breaks */
        // LB4 Always break after hard line breaks
        // LB6 Do not break before hard line breaks
        if (bt == G_UNICODE_BREAK_MANDATORY) {
            LOG_DEBUG ("LB4 Always break after hard line breaks\n");
            ctxt.curr_od = LB4;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_MANDATORY);
            LOG_DEBUG ("LB6 Do not break before hard line breaks\n");
            ctxt.curr_od = LB6;
            break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }
        // LB5 Treat CR followed by LF, as well as CR, LF,
        // and NL as hard line breaks.
        // LB6 Do not break before hard line breaks.
        else if (bt == G_UNICODE_BREAK_CARRIAGE_RETURN
                && (next_uclen = is_next_uchar_lf(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc)) > 0) {
            consumed_one_loop += next_uclen;

            LOG_DEBUG ("LB5 Treat CR followed by LF, as well as CR, LF, and NL as hard line breaks.\n");
            ctxt.curr_od = LB5;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            LOG_DEBUG ("LB6 Do not break before hard line breaks\n");
            ctxt.curr_od = LB6;
            break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);

            lbo = FOIL_BOV_LB_MANDATORY;
            if (break_push_back(&ctxt, next_uc,
                    G_UNICODE_BREAK_LINE_FEED, lbo) == 0)
                goto error;
        }
        // LB5 Treat CR followed by LF, as well as CR, LF,
        // and NL as hard line breaks.
        // LB6 Do not break before hard line breaks.
        else if (bt == G_UNICODE_BREAK_CARRIAGE_RETURN
                || bt == G_UNICODE_BREAK_LINE_FEED
                || bt == G_UNICODE_BREAK_NEXT_LINE) {

             lbo = FOIL_BOV_LB_MANDATORY;
            LOG_DEBUG ("LB5 Treat CR followed by LF, as well as CR, LF, and NL as hard line breaks.\n");
            ctxt.curr_od = LB5;
            break_change_lbo_last(&ctxt, lbo);
            LOG_DEBUG ("LB6 Do not break before hard line breaks\n");
            ctxt.curr_od = LB6;
            break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }

        /* Explicit breaks and non-breaks */
        // LB7 Do not break before spaces or zero width space.
        else if (bt == G_UNICODE_BREAK_SPACE
                || bt == G_UNICODE_BREAK_ZERO_WIDTH_SPACE) {

            LOG_DEBUG ("LB7 Do not break before spaces or zero width space\n");
            ctxt.curr_od = LB7;
            break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }

        // LB8: Break before any character following a zero-width space,
        // even if one or more spaces intervene.
        if (bt == G_UNICODE_BREAK_ZERO_WIDTH_SPACE) {
            LOG_DEBUG ("LB8: Break before any character following a zero-width space...\n");
            ctxt.curr_od = LB8;
            consumed_one_loop += check_uchars_following_zw(&ctxt,
                ucs_left, nr_left_ucs);
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_ALLOWED);
            goto next_uchar;
        }

        if (lbp == FOIL_LBP_ANYWHERE) {
            // ignore the following breaking rules.
            goto next_uchar;
        }

        // LB8a Do not break after a zero width joiner.
        // This changed since Unicode 11.0.0. Before 11.0.0, the rule is:
        // Do not break between a zero width joiner and an ideograph,
        // emoji base or emoji modifier.
        //if (bt == G_UNICODE_BREAK_ZERO_WIDTH_JOINER
        //        && is_next_uchar_id_eb_em(&ctxt,
        //            ucs_left, nr_left_ucs, &next_uc) > 0) {
        if (bt == G_UNICODE_BREAK_ZERO_WIDTH_JOINER) {

            LOG_DEBUG ("LB8a Do not break after a zero width joiner\n");
            ctxt.curr_od = LB8a;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }

        /* Combining marks */
        // LB9 Do not break a combining character sequence;
        // treat it as if it has the line breaking class of
        // the base character in all of the following rules.
        // Treat ZWJ as if it were CM.
        // CM/ZWJ should have the same break class as
        // its base character.
        else if (bt != G_UNICODE_BREAK_MANDATORY
                && bt != G_UNICODE_BREAK_CARRIAGE_RETURN
                && bt != G_UNICODE_BREAK_LINE_FEED
                && bt != G_UNICODE_BREAK_NEXT_LINE
                && bt != G_UNICODE_BREAK_SPACE
                && bt != G_UNICODE_BREAK_ZERO_WIDTH_SPACE
                && is_next_uchar_cm_zwj(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc, NULL) > 0) {

            LOG_DEBUG ("LB9 Do not break a combining character sequence\n");
            ctxt.curr_od = LB9;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);

            // LB10 Treat any remaining combining mark or ZWJ as AL.
            if ((bt == G_UNICODE_BREAK_COMBINING_MARK
                    || bt == G_UNICODE_BREAK_ZERO_WIDTH_JOINER)) {
                LOG_DEBUG ("LB10.a Treat any remaining combining mark or ZWJ as AL\n");
                bt = G_UNICODE_BREAK_ALPHABETIC;
            }

            ctxt.base_bt = bt;
            consumed_one_loop += check_subsequent_cm_zwj(&ctxt, ucs_left, nr_left_ucs);
            break_change_lbo_last(&ctxt, FOIL_BOV_UNKNOWN);

            ucs_left += consumed_one_loop;
            nr_left_ucs -= consumed_one_loop;
            consumed_one_loop = 0;

            ctxt.base_bt = G_UNICODE_BREAK_UNSET;
        }
        // LB10 Treat any remaining combining mark or ZWJ as AL.
        else if ((bt == G_UNICODE_BREAK_COMBINING_MARK
                    || bt == G_UNICODE_BREAK_ZERO_WIDTH_JOINER)) {
            LOG_DEBUG ("LB10.b Treat any remaining combining mark or ZWJ as AL\n");
            bt = G_UNICODE_BREAK_ALPHABETIC;
        }
#if 0
        else if ((bt == G_UNICODE_BREAK_COMBINING_MARK
                    || bt == G_UNICODE_BREAK_ZERO_WIDTH_JOINER)
                && ctxt.base_bt != G_UNICODE_BREAK_UNSET) {
            LOG_DEBUG ("LB9 CM/ZWJ should have the same break class as its base character\n");
            bt = ctxt.base_bt;
        }
#endif

        /* Word joiner */
        // LB11 Do not break before or after Word joiner
        // and related characters.
        if (bt == G_UNICODE_BREAK_WORD_JOINER) {
            lbo = FOIL_BOV_LB_NOTALLOWED;
            LOG_DEBUG ("LB11 Do not break before or after Word joiner and ...\n");
            ctxt.curr_od = LB11;
            break_change_lbo_last(&ctxt, lbo);
            break_change_lbo_before_last(&ctxt, lbo);
        }
        // LB12 Do not break after NBSP and related characters.
        else if (bt == G_UNICODE_BREAK_NON_BREAKING_GLUE) {
            LOG_DEBUG ("LB12 Do not break after NBSP and related characters\n");
            ctxt.curr_od = LB12;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }

        /* Breaking is forbidden within “words”: implicit soft wrap
         * opportunities between typographic letter units (or other
         * typographic character units belonging to the NU, AL, AI, or ID
         * Unicode line breaking classes) are suppressed, i.e. breaks are
         * prohibited between pairs of such characters (regardless of
         * line-break settings other than anywhere) except where opportunities
         * exist due to dictionary-based breaking.
         */
        if (wbr == FOIL_WBR_KEEP_ALL && is_letter(gc, bt)
                && (next_uclen = is_next_uchar_letter(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc, &next_bt)) > 0) {
            LOG_DEBUG ("FOIL_WBR_KEEP_ALL.\n");
            ctxt.curr_od = LB12a;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt, next_uc, next_bt,
                    FOIL_BOV_UNKNOWN) == 0)
                goto error;

            consumed_one_loop += next_uclen;
            goto next_uchar;
        }

        /*
         * Tailorable Line Breaking Rules
         */

        /* Non-breaking characters */
        // LB12a Do not break before NBSP and related characters,
        // except after spaces and hyphens.
        else if (bt != G_UNICODE_BREAK_SPACE
                && bt != G_UNICODE_BREAK_AFTER
                && bt != G_UNICODE_BREAK_HYPHEN
                && is_next_uchar_gl(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc) > 0) {
            LOG_DEBUG ("LB12a Do not break before NBSP and related characters, except after SP and HY\n");
            ctxt.curr_od = LB12a;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }

        /* Opening and closing */
        if (lbp == FOIL_LBP_LOOSE) {
            // LB13 for FOIL_LBP_LOOSE: Do not break before ‘!’, even after spaces.
            if (bt == G_UNICODE_BREAK_EXCLAMATION) {
                LOG_DEBUG ("LB13 for FOIL_LBP_LOOSE: Do not break before ‘!’, even after spaces.\n");
                ctxt.curr_od = LB13;
                break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
        }
        else if (lbp == FOIL_LBP_NORMAL) {
            // LB13 for FOIL_LBP_NORMAL
            if (bt != G_UNICODE_BREAK_NUMERIC
                && is_next_uchar_cl_cp_is_sy(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc) > 0) {

                LOG_DEBUG ("LB13 for FOIL_LBP_NORMAL: Do not break between non-number and ‘]’ or ‘;’ or ‘/’.\n");
                ctxt.curr_od = LB13;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }

            if (bt == G_UNICODE_BREAK_EXCLAMATION) {
                LOG_DEBUG ("LB13 for FOIL_LBP_NORMAL: Do not break before ‘!’, even after spaces\n");
                ctxt.curr_od = LB13;
                break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
        }
        else {
            // LB13 for FOIL_LBP_STRICT: Do not break before ‘]’ or ‘!’ or ‘;’ or ‘/’, even after spaces.
            if (bt == G_UNICODE_BREAK_CLOSE_PUNCTUATION
                    || bt == G_UNICODE_BREAK_CLOSE_PARANTHESIS
                    || bt == G_UNICODE_BREAK_EXCLAMATION
                    || bt == G_UNICODE_BREAK_INFIX_SEPARATOR
                    || bt == G_UNICODE_BREAK_SYMBOL) {
                LOG_DEBUG ("LB13  for FOIL_LBP_STRICT: Do not break before ‘]’ or ‘!’ or ‘;’ or ‘/’, even after spaces\n");
                ctxt.curr_od = LB13;
                break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
        }

        // LB14 Do not break after ‘[’, even after spaces.
        if (bt == G_UNICODE_BREAK_OPEN_PUNCTUATION) {
            LOG_DEBUG ("LB14 Do not break after ‘[’, even after spaces\n");
            ctxt.curr_od = LB14;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);

            // For any possible subsequent space.
            consumed_one_loop += check_subsequent_sp(&ctxt, ucs_left, nr_left_ucs);
        }
        // LB15 Do not break within ‘”[’, even with intervening spaces.
        else if (bt == G_UNICODE_BREAK_QUOTATION
                && (is_subsequent_sps_and_end_bt(&ctxt, ucs_left, nr_left_ucs,
                    G_UNICODE_BREAK_OPEN_PUNCTUATION))) {

            LOG_DEBUG ("LB15 Do not break within ‘”[’, even with intervening spaces\n");
            ctxt.curr_od = LB15;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);

            LOG_DEBUG ("LB19 Do not break before or after quotation marks, such as ‘ ” ’\n");
            ctxt.curr_od = LB19;
            break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);

            ctxt.curr_od = LB15;
            // For subsequent spaces and OP.
            consumed_one_loop += check_subsequent_sps_and_end_bt(&ctxt,
                    ucs_left, nr_left_ucs, false,
                    G_UNICODE_BREAK_OPEN_PUNCTUATION);
        }
        // LB16 Do not break between closing punctuation and a nonstarter
        // (lb=NS), even with intervening spaces.
        if ((bt == G_UNICODE_BREAK_CLOSE_PUNCTUATION
                    || bt == G_UNICODE_BREAK_CLOSE_PARANTHESIS)
                && (is_subsequent_sps_and_end_bt(&ctxt, ucs_left, nr_left_ucs,
                    G_UNICODE_BREAK_NON_STARTER))) {

            LOG_DEBUG ("LB16 Do not break between closing punctuation and NS, even...\n");
            ctxt.curr_od = LB16;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);

            // For subsequent spaces and NS.
            consumed_one_loop += check_subsequent_sps_and_end_bt(&ctxt,
                    ucs_left, nr_left_ucs, false,
                    G_UNICODE_BREAK_NON_STARTER);
        }
        // LB17 Do not break within ‘——’, even with intervening spaces.
        else if (bt == G_UNICODE_BREAK_BEFORE_AND_AFTER
                && (is_subsequent_sps_and_end_bt(&ctxt, ucs_left, nr_left_ucs,
                    G_UNICODE_BREAK_BEFORE_AND_AFTER))) {

            LOG_DEBUG ("LB17 Do not break within ‘——’, even with intervening spaces\n");
            ctxt.curr_od = LB17;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);

            // For subsequent spaces and B2.
            consumed_one_loop += check_subsequent_sps_and_end_bt(&ctxt,
                    ucs_left, nr_left_ucs, false,
                    G_UNICODE_BREAK_BEFORE_AND_AFTER);
        }
        /* Spaces */
        // LB18 Break after spaces.
        else if (bt == G_UNICODE_BREAK_SPACE) {
            LOG_DEBUG ("LB18 Break after spaces\n");
            ctxt.curr_od = LB18;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_ALLOWED);
        }
        /* Special case rules */
        // LB19 Do not break before or after quotation marks, such as ‘ ” ’.
        else if (bt == G_UNICODE_BREAK_QUOTATION) {
            LOG_DEBUG ("LB19 Do not break before or after quotation marks, such as ‘ ” ’\n");
            ctxt.curr_od = LB19;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }
        // LB20 Break before and after unresolved CB.
        else if (bt == G_UNICODE_BREAK_CONTINGENT) {
            LOG_DEBUG ("LB20 Break before and after unresolved CB.\n");
            ctxt.curr_od = LB20;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_ALLOWED);
            break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_ALLOWED);
        }
        // LB21 Do not break before hyphen-minus, other hyphens,
        // fixed-width spaces, small kana, and other non-starters,
        // or after acute accents.
        else if (bt == G_UNICODE_BREAK_AFTER
                || bt == G_UNICODE_BREAK_HYPHEN
                || bt == G_UNICODE_BREAK_NON_STARTER) {
            LOG_DEBUG ("LB21.1 Do not break before hyphen-minus, other hyphens...\n");
            ctxt.curr_od = LB21;
            break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }
        else if (bt == G_UNICODE_BREAK_BEFORE) {
            LOG_DEBUG ("LB21.2 Do not break before hyphen-minus, other hyphens...\n");
            ctxt.curr_od = LB21;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }
        // LB21a Don't break after Hebrew + Hyphen.
        else if (bt == G_UNICODE_BREAK_HEBREW_LETTER
                && (next_uclen = is_next_uchar_hy_ba(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc, &next_bt)) > 0) {

            LOG_DEBUG ("LB21a Don't break after Hebrew + Hyphen\n");
            ctxt.curr_od = LB21a;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt,
                    next_uc, next_bt, FOIL_BOV_LB_NOTALLOWED) == 0)
                goto error;

            consumed_one_loop += next_uclen;
        }
        // LB21b Don’t break between Solidus and Hebrew letters.
        else if (bt == G_UNICODE_BREAK_SYMBOL
                && is_next_uchar_hl(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc) > 0) {
            LOG_DEBUG ("LB21b Don’t break between Solidus and Hebrew letters\n");
            ctxt.curr_od = LB21b;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }
        // LB22 Do not break between two ellipses, or between letters,
        // numbers or exclamations and ellipsis.
        else if ((bt == G_UNICODE_BREAK_HEBREW_LETTER
                    || bt == G_UNICODE_BREAK_ALPHABETIC
                    || bt == G_UNICODE_BREAK_EXCLAMATION
                    || bt == G_UNICODE_BREAK_IDEOGRAPHIC
                    || bt == G_UNICODE_BREAK_EMOJI_BASE
                    || bt == G_UNICODE_BREAK_EMOJI_MODIFIER
                    || bt == G_UNICODE_BREAK_INSEPARABLE
                    || bt == G_UNICODE_BREAK_NUMERIC)
                && is_next_uchar_in(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc) > 0) {

            LOG_DEBUG ("LB22 Do not break between two ellipses, or between letters...\n");
            ctxt.curr_od = LB22;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }

        /* Numbers */
        // LB23 Do not break between digits and letters.
        if (lbp != FOIL_LBP_LOOSE) {
            if ((bt == G_UNICODE_BREAK_HEBREW_LETTER
                        || bt == G_UNICODE_BREAK_ALPHABETIC)
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB23 Do not break between digits and letters\n");
                ctxt.curr_od = LB23;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_NUMERIC
                    && is_next_uchar_al_hl(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB23 Do not break between digits and letters\n");
                ctxt.curr_od = LB23;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            // LB23a Do not break between numeric prefixes and ideographs,
            // or between ideographs and numeric postfixes.
            else if (bt == G_UNICODE_BREAK_PREFIX
                    && is_next_uchar_id_eb_em(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB23a.1 Do not break between numeric prefixes and ID...\n");
                ctxt.curr_od = LB23a;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if ((bt == G_UNICODE_BREAK_IDEOGRAPHIC
                        || bt == G_UNICODE_BREAK_EMOJI_BASE
                        || bt == G_UNICODE_BREAK_EMOJI_MODIFIER)
                    && is_next_uchar_po(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB23a.2 Do not break between numeric prefixes and ID...\n");
                ctxt.curr_od = LB23a;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            // LB24 Do not break between numeric prefix/postfix and letters,
            // or between letters and prefix/postfix.
            else if ((bt == G_UNICODE_BREAK_PREFIX
                        || bt == G_UNICODE_BREAK_POSTFIX)
                    && is_next_uchar_al_hl(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB24 Do not break between numeric prefix/postfix and letters\n");
                ctxt.curr_od = LB24;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if ((bt == G_UNICODE_BREAK_ALPHABETIC
                        || bt == G_UNICODE_BREAK_HEBREW_LETTER)
                    && is_next_uchar_pr_po(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB24 Do not break between numeric prefix/postfix and letters\n");
                ctxt.curr_od = LB24;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
        }

        // LB25 Do not break between the following pairs of classes
        // relevant to numbers
        if (lbp == FOIL_LBP_LOOSE) {
            if (bt == G_UNICODE_BREAK_NUMERIC
                    && is_next_uchar_po(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.5 for FOIL_LBP_LOOSE: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_NUMERIC
                    && is_next_uchar_pr(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.6 for FOIL_LBP_LOOSE: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_POSTFIX
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                ctxt.curr_od = LB25;
                LOG_DEBUG ("LB25.8 for FOIL_LBP_LOOSE: Do not break between the certain pairs of classes\n");
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_PREFIX
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.a for FOIL_LBP_LOOSE: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_HYPHEN
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.b for FOIL_LBP_LOOSE: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_NUMERIC
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.d for FOIL_LBP_LOOSE: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
        }
        else if (lbp == FOIL_LBP_NORMAL) {
            if ((bt == G_UNICODE_BREAK_PREFIX || bt == G_UNICODE_BREAK_POSTFIX)
                    && is_next_uchar_nu_OR_op_hy_followed_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.1 for FOIL_LBP_NORMAL: (PR | PO) × ( OP | HY )? NU.\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if ((bt == G_UNICODE_BREAK_OPEN_PUNCTUATION
                        || bt == G_UNICODE_BREAK_HYPHEN)
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.2 for FOIL_LBP_NORMAL: ( OP | HY ) × NU.\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_NUMERIC
                    && is_next_uchar_nu_sy_is(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.3 for FOIL_LBP_NORMAL: NU × (NU | SY | IS).\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }

            if ((bt == G_UNICODE_BREAK_NUMERIC
                    || bt == G_UNICODE_BREAK_SYMBOL
                    || bt == G_UNICODE_BREAK_INFIX_SEPARATOR
                    || bt == G_UNICODE_BREAK_CLOSE_PUNCTUATION
                    || bt == G_UNICODE_BREAK_CLOSE_PARANTHESIS)
                    && are_prev_uchars_nu_AND_nu_sy_is(&ctxt, false)) {
                LOG_DEBUG ("LB25.4 for FOIL_LBP_NORMAL: NU (NU | SY | IS)* × (NU | SY | IS | CL | CP ).\n");
                ctxt.curr_od = LB25;
                break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            if ((bt == G_UNICODE_BREAK_POSTFIX || bt == G_UNICODE_BREAK_PREFIX)
                    && are_prev_uchars_nu_AND_nu_sy_is_AND_cl_cp(&ctxt)) {
                LOG_DEBUG ("LB25.5 for FOIL_LBP_NORMAL: NU (NU | SY | IS)* (CL | CP)? × (PO | PR).\n");
                ctxt.curr_od = LB25;
                break_change_lbo_before_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
        }
        else {
            if (bt == G_UNICODE_BREAK_CLOSE_PUNCTUATION
                    && is_next_uchar_po(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.1 for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_CLOSE_PUNCTUATION
                    && is_next_uchar_pr(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.2 for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_CLOSE_PARANTHESIS
                    && is_next_uchar_po(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                ctxt.curr_od = LB25;
                LOG_DEBUG ("LB25.3 for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_CLOSE_PARANTHESIS
                    && is_next_uchar_pr(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.4 for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_NUMERIC
                    && is_next_uchar_po(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.5 for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_NUMERIC
                    && is_next_uchar_pr(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.6 for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_POSTFIX
                    && is_next_uchar_op(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                ctxt.curr_od = LB25;
                LOG_DEBUG ("LB25.7 for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_POSTFIX
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                ctxt.curr_od = LB25;
                LOG_DEBUG ("LB25.8 for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_PREFIX
                    && is_next_uchar_op(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                ctxt.curr_od = LB25;
                LOG_DEBUG ("LB25.9 for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_PREFIX
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.a for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_HYPHEN
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.b for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_INFIX_SEPARATOR
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.c for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_NUMERIC
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.d for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
            else if (bt == G_UNICODE_BREAK_SYMBOL
                    && is_next_uchar_nu(&ctxt,
                        ucs_left, nr_left_ucs, &next_uc) > 0) {
                LOG_DEBUG ("LB25.e for FOIL_LBP_STRICT: Do not break between the certain pairs of classes\n");
                ctxt.curr_od = LB25;
                break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            }
        }

        /* Korean syllable blocks */
        // LB26 Do not break a Korean syllable.
        if (bt == G_UNICODE_BREAK_HANGUL_L_JAMO
                && (next_uclen = is_next_uchar_jl_jv_h2_h3(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc, &next_bt)) > 0) {
            LOG_DEBUG ("LB26.1 Do not break a Korean syllable.\n");
            ctxt.curr_od = LB26;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt, next_uc, next_bt,
                    FOIL_BOV_UNKNOWN) == 0)
                goto error;

            consumed_one_loop += next_uclen;
        }
        else if ((bt == G_UNICODE_BREAK_HANGUL_V_JAMO
                    || bt == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE)
                && (next_uclen = is_next_uchar_jv_jt(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc, &next_bt)) > 0) {
            LOG_DEBUG ("LB26.2 Do not break a Korean syllable.\n");
            ctxt.curr_od = LB26;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt, next_uc, next_bt,
                    FOIL_BOV_UNKNOWN) == 0)
                goto error;

            consumed_one_loop += next_uclen;
        }
        else if ((bt == G_UNICODE_BREAK_HANGUL_T_JAMO
                    || bt == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE)
                && (next_uclen = is_next_uchar_jt(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc)) > 0) {
            LOG_DEBUG ("LB26.3 Do not break a Korean syllable.\n");
            ctxt.curr_od = LB26;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt, next_uc,
                    G_UNICODE_BREAK_HANGUL_T_JAMO, FOIL_BOV_UNKNOWN) == 0)
                goto error;

            consumed_one_loop += next_uclen;
        }
        // LB27 Treat a Korean Syllable Block the same as ID.
        else if ((bt == G_UNICODE_BREAK_HANGUL_L_JAMO
                    || bt == G_UNICODE_BREAK_HANGUL_V_JAMO
                    || bt == G_UNICODE_BREAK_HANGUL_T_JAMO
                    || bt == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE
                    || bt == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE)
                && (next_uclen = is_next_uchar_in(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc)) > 0) {
            LOG_DEBUG ("LB27.1 Treat a Korean Syllable Block the same as ID.\n");
            ctxt.curr_od = LB27;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt, next_uc,
                    G_UNICODE_BREAK_INSEPARABLE, FOIL_BOV_UNKNOWN) == 0)
                goto error;

            consumed_one_loop += next_uclen;
        }
        else if ((bt == G_UNICODE_BREAK_HANGUL_L_JAMO
                    || bt == G_UNICODE_BREAK_HANGUL_V_JAMO
                    || bt == G_UNICODE_BREAK_HANGUL_T_JAMO
                    || bt == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE
                    || bt == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE)
                && (next_uclen = is_next_uchar_po(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc)) > 0) {
            LOG_DEBUG ("LB27.2 Treat a Korean Syllable Block the same as ID.\n");
            ctxt.curr_od = LB27;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt, next_uc,
                    G_UNICODE_BREAK_POSTFIX, FOIL_BOV_UNKNOWN) == 0)
                goto error;

            consumed_one_loop += next_uclen;
        }
        else if (bt == G_UNICODE_BREAK_PREFIX
                && (next_uclen = is_next_uchar_jl_jv_jt_h2_h3(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc, &next_bt)) > 0) {
            LOG_DEBUG ("LB27.3 Treat a Korean Syllable Block the same as ID.\n");
            ctxt.curr_od = LB27;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt, next_uc,
                    next_bt, FOIL_BOV_UNKNOWN) == 0)
                goto error;

            consumed_one_loop += next_uclen;
        }

        /* Finally, join alphabetic letters into words
           and break everything else. */

        // LB28 Do not break between alphabetics (“at”).
        else if ((bt == G_UNICODE_BREAK_HEBREW_LETTER
                    || bt == G_UNICODE_BREAK_ALPHABETIC)
                && is_next_uchar_al_hl(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc) > 0) {
            LOG_DEBUG ("LB28 Do not break between alphabetics (“at”)\n");
            ctxt.curr_od = LB28;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }

        // LB29 Do not break between numeric punctuation
        // and alphabetics (“e.g.”).
        else if (bt == G_UNICODE_BREAK_INFIX_SEPARATOR
                && is_next_uchar_al_hl(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc) > 0) {
            LOG_DEBUG ("LB29 Do not break between numeric punctuation\n");
            ctxt.curr_od = LB29;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }

        // LB30 Do not break between letters, numbers, or ordinary symbols and
        // opening or closing parentheses.
        else if ((bt == G_UNICODE_BREAK_HEBREW_LETTER
                    || bt == G_UNICODE_BREAK_ALPHABETIC
                    || bt == G_UNICODE_BREAK_NUMERIC)
                && is_next_uchar_op(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc) > 0) {
            LOG_DEBUG ("LB30.1 Do not break between letters, numbers...\n");
            ctxt.curr_od = LB30;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }
        else if (bt == G_UNICODE_BREAK_CLOSE_PARANTHESIS
                && is_next_uchar_al_hl_nu(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc) > 0) {
            LOG_DEBUG ("LB30.2 Do not break between letters, numbers...\n");
            ctxt.curr_od = LB30;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
        }

        // LB30a Break between two regional indicator symbols if and only if
        // there are an even number of regional indicators preceding the
        // position of the break.
        else if (bt == G_UNICODE_BREAK_REGIONAL_INDICATOR
                && (next_uclen = is_next_uchar_ri(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc)) > 0) {
            LOG_DEBUG ("LB30a.1 Break between two regional indicator symbols...\n");
            ctxt.curr_od = LB30a;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt, next_uc,
                    G_UNICODE_BREAK_REGIONAL_INDICATOR, FOIL_BOV_UNKNOWN) == 0)
                goto error;
            consumed_one_loop += next_uclen;
        }
        else if (bt != G_UNICODE_BREAK_REGIONAL_INDICATOR
                && (next_uclen = is_even_nubmer_of_subsequent_ri(&ctxt,
                    ucs_left, nr_left_ucs)) > 0) {

            LOG_DEBUG ("LB30a.2 Break between two regional indicator symbols...\n");
            ctxt.curr_od = LB30a;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            next_uclen = check_subsequent_ri(&ctxt,
                ucs_left, nr_left_ucs);
            break_change_lbo_last(&ctxt, FOIL_BOV_UNKNOWN);

            consumed_one_loop += next_uclen;
        }

        // LB30b Do not break between an emoji base and an emoji modifier.
        else if (bt == G_UNICODE_BREAK_EMOJI_BASE
                && (next_uclen = is_next_uchar_em(&ctxt,
                    ucs_left, nr_left_ucs, &next_uc)) > 0) {
            LOG_DEBUG ("LB30b Do not break between an emoji base and an emoji modifier\n");
            ctxt.curr_od = LB30b;
            break_change_lbo_last(&ctxt, FOIL_BOV_LB_NOTALLOWED);
            if (break_push_back(&ctxt, next_uc,
                    G_UNICODE_BREAK_EMOJI_MODIFIER, FOIL_BOV_UNKNOWN) == 0)
                goto error;

            consumed_one_loop += next_uclen;
        }

        ctxt.base_bt = G_UNICODE_BREAK_UNSET;

next_uchar:
        nr_left_ucs -= consumed_one_loop;
        ucs_left += consumed_one_loop;

        LOG_DEBUG("%s: nr_ucs: %d, ctxt.n: %d, nr_left_ucs: %d\n",
                __FUNCTION__, nr_ucs, ctxt.n, nr_left_ucs);
#if 0
        // Do not return if we got any BK!
        if (0 && (ctxt.bos[ctxt.n - 1] & FOIL_BOV_LB_MASK) == FOIL_BOV_LB_MANDATORY) {
            break;
        }
#endif
    }

    if (ctxt.n > 0) {
        // Rule GB1
        ctxt.bos[0] |= FOIL_BOV_GB_CHAR_BREAK | FOIL_BOV_GB_CURSOR_POS;
        // Rule WB1
        ctxt.bos[0] |= FOIL_BOV_WB_WORD_BOUNDARY;
        // Ruel SB1
        ctxt.bos[0] |= FOIL_BOV_SB_SENTENCE_BOUNDARY;
        // Rule GB2
        ctxt.bos[ctxt.n - 1] |= FOIL_BOV_GB_CHAR_BREAK | FOIL_BOV_GB_CURSOR_POS;
        // Rule WB2
        ctxt.bos[ctxt.n - 1] |= FOIL_BOV_WB_WORD_BOUNDARY;
        // Rule SB2
        ctxt.bos[ctxt.n - 1] |= FOIL_BOV_SB_SENTENCE_BOUNDARY;

        // LB31 Break everywhere else.
        ssize_t n;
        for (n = 1; n < ctxt.n; n++) {
            if ((ctxt.bos[n] & FOIL_BOV_LB_MASK) == FOIL_BOV_UNKNOWN) {
                LOG_DEBUG ("LB31 Break everywhere else: %d\n", n);
                ctxt.bos[n] &= ~FOIL_BOV_LB_MASK;
                ctxt.bos[n] |= FOIL_BOV_LB_ALLOWED;
            }
        }

    }
    else
        goto error;

    if (*break_oppos == NULL) *break_oppos = ctxt.bos;
    if (ctxt.ods && ctxt.ods != local_ods) free(ctxt.ods);
    if (ctxt.bts && ctxt.bts != local_bts) free(ctxt.bts);

    return ctxt.n;

error:
    if (*break_oppos == NULL && ctxt.bos) free(ctxt.bos);
    if (ctxt.ods && ctxt.ods != local_ods) free(ctxt.ods);
    if (ctxt.bts && ctxt.bts != local_bts) free(ctxt.bts);
    return 0;
}

