/*
** @file unicode.h
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

#ifndef purc_foil_unicode_h
#define purc_foil_unicode_h

#define FOIL_UCHAR_TAB               0x0009
#define FOIL_UCHAR_LINE_FEED         0x000A
#define FOIL_UCHAR_SPACE             0x0020
#define FOIL_UCHAR_SHY               0x00AD
#define FOIL_UCHAR_IDSPACE           0x3000
#define FOIL_UCHAR_LINE_SEPARATOR    0x2028

#include "rdrbox.h"

/**
 * \def FOIL_WSR_NORMAL
 *
 * \brief This value directs \a foil_ustr_from_utf8_until_paragraph_boundary
 * collapses sequences of white space into a single character.
 * Lines may wrap at allowed soft wrap opportunities.
 */
#define FOIL_WSR_NORMAL          FOIL_RDRBOX_WHITE_SPACE_NORMAL

/**
 * \def FOIL_WSR_PRE
 *
 * \brief This value prevents \a foil_ustr_from_utf8_until_paragraph_boundary
 * from collapsing
 * sequences of white space. Segment breaks such as line feeds are
 * preserved as forced line breaks. Lines only break at forced line breaks;
 * content that does not fit within the specified extent overflows it.
 */
#define FOIL_WSR_PRE             FOIL_RDRBOX_WHITE_SPACE_PRE

/**
 * \def FOIL_WSR_NOWRAP
 *
 * \brief Like \a FOIL_WSR_NORMAL, this value collapses white spaces; but like
 * \a FOIL_WSR_PRE, it does not allow wrapping.
 */
#define FOIL_WSR_NOWRAP          FOIL_RDRBOX_WHITE_SPACE_NOWRAP

/**
 * \def FOIL_WSR_PRE_WRAP
 *
 * \brief Like \a FOIL_WSR_PRE, this value preserves white space; but like
 * \a FOIL_WSR_NORMAL, it allows wrapping.
 */
#define FOIL_WSR_PRE_WRAP        FOIL_RDRBOX_WHITE_SPACE_PRE_WRAP

/**
 * \def FOIL_WSR_BREAK_SPACES
 *
 * \brief
 * The behavior is identical to that of \a FOIL_WSR_PRE_WRAP, except that:
 *  - Any sequence of preserved white space always takes up space,
 *      including at the end of the line.
 *  - A line breaking opportunity exists after every preserved
 *      white space glyph, including between white space characters.
 *
 * When white space rule is specified to be FOIL_WSR_BREAK_SPACES, the manner
 * of \a foil_ustr_from_utf8_until_paragraph_boundary will conform
 * to CSS Text Module Level 3.
 */
#define FOIL_WSR_BREAK_SPACES    FOIL_RDRBOX_WHITE_SPACE_BREAK_SPACES

/**
 * \def FOIL_WSR_PRE_LINE
 *
 * \brief Like WS_NORMAL, this value collapses consecutive spaces and
 * allows wrapping, but preserves segment breaks in the source
 * as forced line breaks.
 */
#define FOIL_WSR_PRE_LINE        FOIL_RDRBOX_WHITE_SPACE_PRE_LINE

/**
 * \def FOIL_CTR_NONE
 *
 * \brief No effects.
 */
#define FOIL_CTR_NONE            0x00

#define FOIL_CTR_CASE_MASK       0x0F

/**
 * \def FOIL_CTR_CAPITALIZE
 *
 * \brief Puts the first typographic letter unit of each word,
 * if lowercase, in titlecase; other characters are unaffected.
 */
#define FOIL_CTR_CAPITALIZE      FOIL_RDRBOX_TEXT_TRANSFORM_CAPITALIZE

/**
 * \def FOIL_CTR_UPPERCASE
 *
 * \brief Puts all letters in uppercase.
 */
#define FOIL_CTR_UPPERCASE       FOIL_RDRBOX_TEXT_TRANSFORM_UPPERCASE

/**
 * \def FOIL_CTR_LOWERCASE
 *
 * \brief Puts all letters in lowercase.
 */
#define FOIL_CTR_LOWERCASE       FOIL_RDRBOX_TEXT_TRANSFORM_LOWERCASE

/**
 * \def FOIL_CTR_FULL_WIDTH
 *
 * \brief Puts all typographic character units in fullwidth form.
 * If a character does not have a corresponding fullwidth form,
 * it is left as is. This value is typically used to typeset
 * Latin letters and digits as if they were ideographic characters.
 */
#define FOIL_CTR_FULL_WIDTH      FOIL_RDRBOX_TEXT_TRANSFORM_FULL_WIDTH

/**
 * \def FOIL_CTR_FULL_SIZE_KANA
 *
 * \brief Converts all small Kana characters to the equivalent
 * full-size Kana. This value is typically used for ruby annotation
 * text, where authors may want all small Kana to be drawn as large
 * Kana to compensate for legibility issues at the small font sizes
 * typically used in ruby.
 */
#define FOIL_CTR_FULL_SIZE_KANA  FOIL_RDRBOX_TEXT_TRANSFORM_FULL_SIZE_KANA

/**
 * \def FOIL_WBR_NORMAL
 *
 * \brief Words break according to their customary rules, as defined
 * by UNICODE LINE BREAKING ALGORITHM.
 */
#define FOIL_WBR_NORMAL          FOIL_RDRBOX_WORD_BREAK_NORMAL

/**
 * \def FOIL_WBR_BREAK_ALL
 *
 * \brief Breaking is allowed within “words”.
 */
#define FOIL_WBR_BREAK_ALL       FOIL_RDRBOX_WORD_BREAK_BREAK_ALL

/**
 * \def FOIL_WBR_KEEP_ALL
 *
 * \brief Breaking is forbidden within “words”.
 */
#define FOIL_WBR_KEEP_ALL        FOIL_RDRBOX_WORD_BREAK_KEEP_ALL

/**
 * \def FOIL_LBP_NORMAL
 *
 * \brief Breaks text using the most common set of line-breaking rules.
 */
#define FOIL_LBP_NORMAL          FOIL_RDRBOX_LINE_BREAK_NORMAL

/**
 * \def FOIL_LBP_LOOSE
 *
 * \brief Breaks text using the least restrictive set of line-breaking rules.
 * Typically used for short lines, such as in newspapers.
 */
#define FOIL_LBP_LOOSE           FOIL_RDRBOX_LINE_BREAK_LOOSE

/**
 * \def FOIL_LBP_STRICT
 *
 * \brief Breaks text using the most stringent set of line-breaking rules.
 */
#define FOIL_LBP_STRICT          FOIL_RDRBOX_LINE_BREAK_STRICT

/**
 * \def FOIL_LBP_ANYWHERE
 *
 * \brief There is a soft wrap opportunity around every typographic character
 * unit, including around any punctuation character or preserved spaces,
 * or in the middle of words, disregarding any prohibition against line
 * breaks, even those introduced by characters with the GL, WJ, or ZWJ
 * breaking class or mandated by the word breaking rule. The different wrapping
 * opportunities must not be prioritized. Hyphenation is not applied.
 */
#define FOIL_LBP_ANYWHERE        FOIL_RDRBOX_LINE_BREAK_ANYWHERE

/**
 * Unknown breaking code.
 */
#define FOIL_BOV_UNKNOWN                 0x0000
/**
 * If set, the character is a whitespace character.
 */
#define FOIL_BOV_WHITESPACE              0x8000
/**
 * If set, the character is an expandable space.
 */
#define FOIL_BOV_EXPANDABLE_SPACE        0x0800

/**
 * If set, the character has zero width.
 */
#define FOIL_BOV_ZERO_WIDTH              0x0080

#define FOIL_BOV_GB_MASK                 0x7000
/**
 * If set, can break at the character when doing character wrap.
 */
#define FOIL_BOV_GB_CHAR_BREAK           0x1000
/**
 * If set, cursor can appear in front of the character
 * (i.e. this is a grapheme boundary, or the first character in the text).
 */
#define FOIL_BOV_GB_CURSOR_POS           0x2000
/**
 * If set, backspace deletes one character rather than
 * the entire grapheme cluster.
 */
#define FOIL_BOV_GB_BACKSPACE_DEL_CH     0x4000

#define FOIL_BOV_WB_MASK                 0x0700
/**
 * If set, the glyph is the word boundary as defined by UAX#29.
 */
#define FOIL_BOV_WB_WORD_BOUNDARY        0x0100
/**
 * If set, the glyph is the first character in a word.
 */
#define FOIL_BOV_WB_WORD_START           0x0200
/**
 * If set, the glyph is the first non-word character after a word.
 */
#define FOIL_BOV_WB_WORD_END             0x0400

#define FOIL_BOV_SB_MASK                 0x0070
/**
 * If set, the glyph is the sentence boundary as defined by UAX#29.
 */
#define FOIL_BOV_SB_SENTENCE_BOUNDARY    0x0010
/**
 * If set, the glyph is the first character in a sentence.
 */
#define FOIL_BOV_SB_SENTENCE_START       0x0020
/**
 * If set, the glyph is the first non-sentence character after a sentence.
 */
#define FOIL_BOV_SB_SENTENCE_END         0x0040

#define FOIL_BOV_LB_MASK                 0x000F
#define FOIL_BOV_LB_BREAK_FLAG           0x0004
#define FOIL_BOV_LB_MANDATORY_FLAG       0x0008
/**
 * The line can break after the character.
 */
#define FOIL_BOV_LB_ALLOWED              (FOIL_BOV_LB_BREAK_FLAG | 0x0001)
/**
 * The line must break after the character.
 */
#define FOIL_BOV_LB_MANDATORY            (FOIL_BOV_LB_BREAK_FLAG | \
        FOIL_BOV_LB_MANDATORY_FLAG | 0x0002)
/**
 * The line break is not allowed after the character.
 */
#define FOIL_BOV_LB_NOTALLOWED           0x0003

/**
 * The type for breaking opportunity (uint16_t).
 */
typedef uint16_t foil_break_oppo_t;

/**
 * The language code.
 */
typedef enum {
    /** Unknown language code */
    FOIL_LANGCODE_unknown = 0xFF,
    /** Language code for Afar */
    FOIL_LANGCODE_aa = 0,
    /** Language code for Abkhazian */
    FOIL_LANGCODE_ab,
    /** Language code for Afrikaans */
    FOIL_LANGCODE_af,
    /** Language code for Amharic */
    FOIL_LANGCODE_am,
    /** Language code for Arabic */
    FOIL_LANGCODE_ar,
    /** Language code for Assamese */
    FOIL_LANGCODE_as,
    /** Language code for Aymara */
    FOIL_LANGCODE_ay,
    /** Language code for Azerbaijani */
    FOIL_LANGCODE_az,
    /** Language code for Bashkir */
    FOIL_LANGCODE_ba,
    /** Language code for Byelorussian */
    FOIL_LANGCODE_be,
    /** Language code for Bulgarian */
    FOIL_LANGCODE_bg,
    /** Language code for Bihari */
    FOIL_LANGCODE_bh,
    /** Language code for Bislama */
    FOIL_LANGCODE_bi,
    /** Language code for Bengali */
    FOIL_LANGCODE_bn,
    /** Language code for Tibetan */
    FOIL_LANGCODE_bo,
    /** Language code for Breton */
    FOIL_LANGCODE_br,
    /** Language code for Catalan */
    FOIL_LANGCODE_ca,
    /** Language code for Corsican */
    FOIL_LANGCODE_co,
    /** Language code for Czech */
    FOIL_LANGCODE_cs,
    /** Language code for Welch */
    FOIL_LANGCODE_cy,
    /** Language code for Danish */
    FOIL_LANGCODE_da,
    /** Language code for German */
    FOIL_LANGCODE_de,
    /** Language code for Divehi */
    FOIL_LANGCODE_dv,
    /** Language code for Bhutani */
    FOIL_LANGCODE_dz,
    /** Language code for Greek */
    FOIL_LANGCODE_el,
    /** Language code for English */
    FOIL_LANGCODE_en,
    /** Language code for Esperanto */
    FOIL_LANGCODE_eo,
    /** Language code for Spanish */
    FOIL_LANGCODE_es,
    /** Language code for Estonian */
    FOIL_LANGCODE_et,
    /** Language code for Basque */
    FOIL_LANGCODE_eu,
    /** Language code for Persian */
    FOIL_LANGCODE_fa,
    /** Language code for Finnish */
    FOIL_LANGCODE_fi,
    /** Language code for Fiji */
    FOIL_LANGCODE_fj,
    /** Language code for Faeroese */
    FOIL_LANGCODE_fo,
    /** Language code for French */
    FOIL_LANGCODE_fr,
    /** Language code for Frisian */
    FOIL_LANGCODE_fy,
    /** Language code for Irish */
    FOIL_LANGCODE_ga,
    /** Language code for Scots Gaelic */
    FOIL_LANGCODE_gd,
    /** Language code for Galician */
    FOIL_LANGCODE_gl,
    /** Language code for Guarani */
    FOIL_LANGCODE_gn,
    /** Language code for Gujarati */
    FOIL_LANGCODE_gu,
    /** Language code for Hausa */
    FOIL_LANGCODE_ha,
    /** Language code for Hindi */
    FOIL_LANGCODE_hi,
    /** Language code for Hebrew */
    FOIL_LANGCODE_he,
    /** Language code for Croatian */
    FOIL_LANGCODE_hr,
    /** Language code for Hungarian */
    FOIL_LANGCODE_hu,
    /** Language code for Armenian */
    FOIL_LANGCODE_hy,
    /** Language code for Interlingua */
    FOIL_LANGCODE_ia,
    /** Language code for Indonesian */
    FOIL_LANGCODE_id,
    /** Language code for Interlingue */
    FOIL_LANGCODE_ie,
    /** Language code for Inupiak */
    FOIL_LANGCODE_ik,
    /** Language code for former Indonesian */
    FOIL_LANGCODE_in,
    /** Language code for Icelandic */
    FOIL_LANGCODE_is,
    /** Language code for Italian */
    FOIL_LANGCODE_it,
    /** Language code for Inuktitut (Eskimo) */
    FOIL_LANGCODE_iu,
    /** Language code for former Hebrew */
    FOIL_LANGCODE_iw,
    /** Language code for Japanese */
    FOIL_LANGCODE_ja,
    /** Language code for former Yiddish */
    FOIL_LANGCODE_ji,
    /** Language code for Javanese */
    FOIL_LANGCODE_jw,
    /** Language code for Georgian */
    FOIL_LANGCODE_ka,
    /** Language code for Kazakh */
    FOIL_LANGCODE_kk,
    /** Language code for Greenlandic */
    FOIL_LANGCODE_kl,
    /** Language code for Cambodian */
    FOIL_LANGCODE_km,
    /** Language code for Kannada */
    FOIL_LANGCODE_kn,
    /** Language code for Korean */
    FOIL_LANGCODE_ko,
    /** Language code for Kashmiri */
    FOIL_LANGCODE_ks,
    /** Language code for Kurdish */
    FOIL_LANGCODE_ku,
    /** Language code for Kirghiz */
    FOIL_LANGCODE_ky,
    /** Language code for Latin */
    FOIL_LANGCODE_la,
    /** Language code for Lingala */
    FOIL_LANGCODE_ln,
    /** Language code for Laothian */
    FOIL_LANGCODE_lo,
    /** Language code for Lithuanian */
    FOIL_LANGCODE_lt,
    /** Language code for Latvian, Lettish */
    FOIL_LANGCODE_lv,
    /** Language code for Malagasy */
    FOIL_LANGCODE_mg,
    /** Language code for Maori */
    FOIL_LANGCODE_mi,
    /** Language code for Macedonian */
    FOIL_LANGCODE_mk,
    /** Language code for Malayalam */
    FOIL_LANGCODE_ml,
    /** Language code for Mongolian */
    FOIL_LANGCODE_mn,
    /** Language code for Moldavian */
    FOIL_LANGCODE_mo,
    /** Language code for Marathi */
    FOIL_LANGCODE_mr,
    /** Language code for Malay */
    FOIL_LANGCODE_ms,
    /** Language code for Maltese */
    FOIL_LANGCODE_mt,
    /** Language code for Burmese */
    FOIL_LANGCODE_my,
    /** Language code for Nauru */
    FOIL_LANGCODE_na,
    /** Language code for Nepali */
    FOIL_LANGCODE_ne,
    /** Language code for Dutch */
    FOIL_LANGCODE_nl,
    /** Language code for Norwegian */
    FOIL_LANGCODE_no,
    /** Language code for Occitan */
    FOIL_LANGCODE_oc,
    /** Language code for (Afan) Oromo */
    FOIL_LANGCODE_om,
    /** Language code for Oriya */
    FOIL_LANGCODE_or,
    /** Language code for Punjabi */
    FOIL_LANGCODE_pa,
    /** Language code for Polish */
    FOIL_LANGCODE_pl,
    /** Language code for Pashto, Pushto */
    FOIL_LANGCODE_ps,
    /** Language code for Portuguese */
    FOIL_LANGCODE_pt,
    /** Language code for Quechua */
    FOIL_LANGCODE_qu,
    /** Language code for Rhaeto-Romance */
    FOIL_LANGCODE_rm,
    /** Language code for Kirundi */
    FOIL_LANGCODE_rn,
    /** Language code for Romanian */
    FOIL_LANGCODE_ro,
    /** Language code for Russian */
    FOIL_LANGCODE_ru,
    /** Language code for Kinyarwanda */
    FOIL_LANGCODE_rw,
    /** Language code for Sanskrit */
    FOIL_LANGCODE_sa,
    /** Language code for Sindhi */
    FOIL_LANGCODE_sd,
    /** Language code for Sangro */
    FOIL_LANGCODE_sg,
    /** Language code for Serbo-Croatian */
    FOIL_LANGCODE_sh,
    /** Language code for Singhalese */
    FOIL_LANGCODE_si,
    /** Language code for Slovak */
    FOIL_LANGCODE_sk,
    /** Language code for Slovenian */
    FOIL_LANGCODE_sl,
    /** Language code for Samoan */
    FOIL_LANGCODE_sm,
    /** Language code for Shona */
    FOIL_LANGCODE_sn,
    /** Language code for Somali */
    FOIL_LANGCODE_so,
    /** Language code for Albanian */
    FOIL_LANGCODE_sq,
    /** Language code for Serbian */
    FOIL_LANGCODE_sr,
    /** Language code for Siswati */
    FOIL_LANGCODE_ss,
    /** Language code for Sesotho */
    FOIL_LANGCODE_st,
    /** Language code for Sudanese */
    FOIL_LANGCODE_su,
    /** Language code for Swedish */
    FOIL_LANGCODE_sv,
    /** Language code for Swahili */
    FOIL_LANGCODE_sw,
    /** Language code for Tamil */
    FOIL_LANGCODE_ta,
    /** Language code for Tegulu */
    FOIL_LANGCODE_te,
    /** Language code for Tajik */
    FOIL_LANGCODE_tg,
    /** Language code for Thai */
    FOIL_LANGCODE_th,
    /** Language code for Tigrinya */
    FOIL_LANGCODE_ti,
    /** Language code for Turkmen */
    FOIL_LANGCODE_tk,
    /** Language code for Tagalog */
    FOIL_LANGCODE_tl,
    /** Language code for Setswana */
    FOIL_LANGCODE_tn,
    /** Language code for Tonga */
    FOIL_LANGCODE_to,
    /** Language code for Turkish */
    FOIL_LANGCODE_tr,
    /** Language code for Tsonga */
    FOIL_LANGCODE_ts,
    /** Language code for Tatar */
    FOIL_LANGCODE_tt,
    /** Language code for Twi */
    FOIL_LANGCODE_tw,
    /** Language code for Uigur */
    FOIL_LANGCODE_ug,
    /** Language code for Ukrainian */
    FOIL_LANGCODE_uk,
    /** Language code for Urdu */
    FOIL_LANGCODE_ur,
    /** Language code for Uzbek */
    FOIL_LANGCODE_uz,
    /** Language code for Vietnamese */
    FOIL_LANGCODE_vi,
    /** Language code for Volapuk */
    FOIL_LANGCODE_vo,
    /** Language code for Wolof */
    FOIL_LANGCODE_wo,
    /** Language code for Xhosa */
    FOIL_LANGCODE_xh,
    /** Language code for Yiddish */
    FOIL_LANGCODE_yi,
    /** Language code for Yoruba */
    FOIL_LANGCODE_yo,
    /** Language code for Zhuang */
    FOIL_LANGCODE_za,
    /** Language code for Chinese */
    FOIL_LANGCODE_zh,
    /** Language code for Zulu */
    FOIL_LANGCODE_zu,
} foil_langcode_t;

/**
 * \defgroup glyph_render_flags Glyph Rendering Flags
 *
 * The glyph rendering flags indicate \a GetGlyphsExtentFromUChars and
 * \a CreateLayout how to lay out the text:
 *      - The writing mode (horizontal or vertical) and the glyph orientation;
 *      - The indentation mode (none, first line or hanging);
 *      - Whether and how to break if the line overflows the max extent;
 *      - Whether and how to ellipsize if the line overflows the max extent;
 *      - The alignment of lines;
 *      - Whether and how to adjust the glyph position for alignment of justify;
 *      - The hanging punctation method;
 *      - Remove or hange the spaces at the start and/or end of the line.
 * @{
 */

#define FOIL_GRF_WRITING_MODE_MASK               0xF0000000
#define FOIL_GRF_WRITING_MODE_VERTICAL_FLAG      0x20000000
/**
 * Top-to-bottom horizontal direction.
 * Both the writing mode and the typographic mode are horizontal.
 */
#define FOIL_GRF_WRITING_MODE_HORIZONTAL_TB      0x00000000
/**
 * Bottom-to-top horizontal direction.
 * Both the writing mode and the typographic mode are horizontal,
 * but lines are generated from bottom to top.
 */
#define FOIL_GRF_WRITING_MODE_HORIZONTAL_BT      0x10000000
/**
 * Right-to-left vertical direction.
 * Both the writing mode and the typographic mode are vertical,
 * but the lines are generated from right to left.
 */
#define FOIL_GRF_WRITING_MODE_VERTICAL_RL        0x20000000
/**
 * Left-to-right vertical direction.
 * Both the writing mode and the typographic mode are vertical.
 * but the lines are generated from left to right.
 */
#define FOIL_GRF_WRITING_MODE_VERTICAL_LR        0x30000000

#define FOIL_GRF_TEXT_ORIENTATION_MASK           0x0F000000
/**
 * The glyphs are individually typeset upright in
 * vertical lines with vertical font metrics.
 */
#define FOIL_GRF_TEXT_ORIENTATION_UPRIGHT        0x00000000
/**
 * The glyphs typeset a run rotated 90° clockwise
 * from their upright orientation.
 */
#define FOIL_GRF_TEXT_ORIENTATION_SIDEWAYS       0x01000000
/**
 * The glyphs are individually typeset upside down in
 * vertical lines with vertical font metrics.
 */
#define FOIL_GRF_TEXT_ORIENTATION_UPSIDE_DOWN    0x02000000
/**
 * The glyphs typeset a run rotated 90° counter-clockwise
 * from their upright orientation.
 */
#define FOIL_GRF_TEXT_ORIENTATION_SIDEWAYS_LEFT  0x03000000
/**
 * In vertical writing modes, all typographic character units
 * keep in their intrinsic orientation.
 */
#define FOIL_GRF_TEXT_ORIENTATION_AUTO           0x04000000
/**
 * In vertical writing modes, typographic character units from
 * horizontal-only scripts are typeset sideways, i.e. 90° clockwise
 * from their standard orientation in horizontal text.
 * Typographic character units from vertical scripts are
 * typeset with their intrinsic orientation.
 */
#define FOIL_GRF_TEXT_ORIENTATION_MIXED          0x05000000

#define FOIL_GRF_TEXT_ORIENTATION_LINE           0x06000000

#define FOIL_GRF_LINE_EXTENT_MASK                0x00C00000
/**
 * The maximal line extent is fixed.
 * The maximal line extent value you passed to \a LayoutNextLine
 * will be ignored.
 */
#define FOIL_GRF_LINE_EXTENT_FIXED               0x00000000
/**
 * The maximal line extent is variable. You should pass the desired
 * maximal line extent value for a new line when calling
 * \a LayoutNextLine. The intent mode will be ignored as well.
 */
#define FOIL_GRF_LINE_EXTENT_VARIABLE            0x00400000

#define FOIL_GRF_INDENT_MASK                     0x00300000
/**
 * No indentation.
 */
#define FOIL_GRF_INDENT_NONE                     0x00000000
/**
 * The first line is indented.
 */
#define FOIL_GRF_INDENT_FIRST_LINE               0x00100000
/**
 * Indent all the lines of a paragraph except the first line.
 */
#define FOIL_GRF_INDENT_HANGING                  0x00200000

#define FOIL_GRF_OVERFLOW_WRAP_MASK              0x000C0000
/**
 * Lines may break only at allowed break points.
 */
#define FOIL_GRF_OVERFLOW_WRAP_NORMAL            0x00000000
/**
 * Lines may break only at word seperators.
 */
#define FOIL_GRF_OVERFLOW_WRAP_BREAK_WORD        0x00040000
/**
 * An otherwise unbreakable sequence of characters may be broken
 * at an arbitrary point if there are no otherwise-acceptable
 * break points in the line.
 */
#define FOIL_GRF_OVERFLOW_WRAP_ANYWHERE          0x00080000

#define FOIL_GRF_OVERFLOW_ELLIPSIZE_MASK         0x00030000
/**
 * No ellipsization
 */
#define FOIL_GRF_OVERFLOW_ELLIPSIZE_NONE         0x00000000
/**
 * Omit characters at the start of the text
 */
#define FOIL_GRF_OVERFLOW_ELLIPSIZE_START        0x00010000
/**
 * Omit characters in the middle of the text
 */
#define FOIL_GRF_OVERFLOW_ELLIPSIZE_MIDDLE       0x00020000
/**
 * Omit characters at the end of the text
 */
#define FOIL_GRF_OVERFLOW_ELLIPSIZE_END          0x00030000

#define FOIL_GRF_ALIGN_MASK                      0x0000F000
/**
 * Text content is aligned to the start edge of the line box.
 */
#define FOIL_GRF_ALIGN_START                     0x00000000
/**
 * Text content is aligned to the end edge of the line box.
 */
#define FOIL_GRF_ALIGN_END                       0x00001000
/**
 * Text content is aligned to the line left edge of the line box.
 * In vertical writing modes, this will be the physical top edge.
 */
#define FOIL_GRF_ALIGN_LEFT                      0x00002000
/**
 * Text content is aligned to the line right edge of the line box.
 * In vertical writing modes, this will be the physical bottom edge.
 */
#define FOIL_GRF_ALIGN_RIGHT                     0x00003000
/**
 * Text content is centered within the line box.
 */
#define FOIL_GRF_ALIGN_CENTER                    0x00004000
/**
 * All lines will be justified according to the method specified by
 * FOIL_GRF_TEXT_JUSTIFY_XXX, in order to exactly fill the line box.
 *
 * If you specify only a valid justification method (not FOIL_GRF_TEXT_JUSTIFY_NONE)
 * without FOIL_GRF_ALIGN_JUSTIFY, the last line will not be justified.
 */
#define FOIL_GRF_ALIGN_JUSTIFY                   0x00005000

#define FOIL_GRF_TEXT_JUSTIFY_MASK               0x00000F00
/**
 * Do not justify.
 */
#define FOIL_GRF_TEXT_JUSTIFY_NONE               0x00000000
/**
 * Justification adjusts primarily the spacing at word separators
 * and between CJK typographic letter units along with secondarily
 * between Southeast Asian typographic letter units.
 */
#define FOIL_GRF_TEXT_JUSTIFY_AUTO               0x00000100
/**
 * Justification adjusts spacing at word separators only.
 */
#define FOIL_GRF_TEXT_JUSTIFY_INTER_WORD         0x00000200
/**
 * Justification adjusts spacing between each pair of adjacent
 * typographic character units.
 */
#define FOIL_GRF_TEXT_JUSTIFY_INTER_CHAR         0x00000300

#define FOIL_GRF_HANGING_PUNC_MASK               0x000000F0
/**
 * No character hangs.
 */
#define FOIL_GRF_HANGING_PUNC_NONE               0x00000000
/**
 * A stop or comma at the end of a line hangs.
 */
#define FOIL_GRF_HANGING_PUNC_FORCE_END          0x00000010
/**
 * A stop or comma at the end of a line hangs
 * if it does not otherwise fit prior to justification.
 */
#define FOIL_GRF_HANGING_PUNC_ALLOW_END          0x00000020
/**
 * An opening bracket or quote at the start of the line hangs.
 */
#define FOIL_GRF_HANGING_PUNC_OPEN               0x00000040
/**
 * An closing bracket or quote at the end of the line hangs.
 */
#define FOIL_GRF_HANGING_PUNC_CLOSE              0x00000080

#define FOIL_GRF_SPACES_MASK                     0x0000000F
/**
 * All spaces are kept.
 */
#define FOIL_GRF_SPACES_KEEP                     0x00000000
/**
 * A sequence of spaces at the start of a line is removed.
 */
#define FOIL_GRF_SPACES_REMOVE_START             0x00000001
/**
 * A sequence of spaces at the end of a line is removed.
 */
#define FOIL_GRF_SPACES_REMOVE_END               0x00000002
/**
 * A sequence of spaces at the end of a line hangs.
 */
#define FOIL_GRF_SPACES_HANGE_END                0x00000004

/** @} end of glyph_render_flags */

typedef enum {
    FOIL_GLYPH_GRAVITY_SOUTH = 0,
    FOIL_GLYPH_GRAVITY_EAST,
    FOIL_GLYPH_GRAVITY_NORTH,
    FOIL_GLYPH_GRAVITY_WEST,
    FOIL_GLYPH_GRAVITY_AUTO,
} foil_glyph_gravity;

typedef enum {
    FOIL_GLYPH_ORIENT_UPRIGHT        = FOIL_GLYPH_GRAVITY_SOUTH,
    FOIL_GLYPH_ORIENT_SIDEWAYS       = FOIL_GLYPH_GRAVITY_EAST,
    FOIL_GLYPH_ORIENT_UPSIDE_DOWN    = FOIL_GLYPH_GRAVITY_NORTH,
    FOIL_GLYPH_ORIENT_SIDEWAYS_LEFT  = FOIL_GLYPH_GRAVITY_WEST,
} foil_glyph_orient;

typedef enum {
    FOIL_GLYPH_HANGED_NONE = 0,
    FOIL_GLYPH_HANGED_START,
    FOIL_GLYPH_HANGED_END,
} foil_glyph_hanged;

/**
 * The glyph extent information.
 */
typedef struct _foil_glyph_extinfo {
    /** The bounding box of the glyph. */
    int bbox_x, bbox_y, bbox_w, bbox_h;
    /** The advance values of the glyph. */
    int adv_x, adv_y;
    /** The extra spacing values of the glyph. */
    int extra_x, extra_y;
    /** The advance value of the glyph along the line direction. */
    int line_adv;
    /**
     * Whether suppress the glyph.
     */
    uint8_t suppressed:1;
    /**
     * Whether is a whitespace glyph.
     */
    uint8_t whitespace:1;
    /**
     * The orientation of the glyph; can be one of the following values:
     *  - FOIL_GLYPH_ORIENT_UPRIGHT\n
     *      the glyph is in the standard horizontal orientation.
     *  - FOIL_GLYPH_ORIENT_SIDEWAYS\n
     *      the glyph rotates 90° clockwise from horizontal.
     *  - FOIL_GLYPH_ORIENT_SIDEWAYS_LEFT\n
     *      the glyph rotates 90° counter-clockwise from horizontal.
     *  - FOIL_GLYPH_ORIENT_UPSIDE_DOWN\n
     *      the glyph is in the inverted horizontal orientation.
     */
    uint8_t orientation:2;
} foil_glyph_extinfo;

/**
 * The glyph position information.
 */
typedef struct _foil_glyph_pos {
    /**
     * The x coordinate of the glyph position.
     */
    int x;
    /**
     * The y coordinate of the glyph position.
     */
    int y;
    /**
     * The line advance of the glyph.
     */
    int advance;
    /**
     * Whether suppress the glyph.
     */
    uint8_t suppressed:1;
    /**
     * Whether is a whitespace glyph.
     */
    uint8_t whitespace:1;
    /**
     * Whether is an ellipsized glyph.
     */
    uint8_t ellipsis:1;
    /**
     * The orientation of the glyph; can be one of the following values:
     *  - FOIL_GLYPH_ORIENT_UPRIGHT\n
     *      the glyph is in the standard horizontal orientation.
     *  - FOIL_GLYPH_ORIENT_SIDEWAYS\n
     *      the glyph rotates 90° clockwise from horizontal.
     *  - FOIL_GLYPH_ORIENT_SIDEWAYS_LEFT\n
     *      the glyph rotates 90° counter-clockwise from horizontal.
     *  - FOIL_GLYPH_ORIENT_UPSIDE_DOWN\n
     *      the glyph is upside down.
     */
    uint8_t orientation:2;
    /**
     * Whether hanged the glyph; can be one of the following values:
     *  - FOIL_GLYPH_HANGED_NONE\n
     *      the glyph is not hanged.
     *  - FOIL_GLYPH_HANGED_START\n
     *      the glyph is hanged at the start of the line.
     *  - FOIL_GLYPH_HANGED_END\n
     *      the glyph is hanged at the end of the line.
     */
    uint8_t hanged:2;
} foil_glyph_pos;

#ifdef __cplusplus
extern "C" {
#endif

bool foil_uchar_is_letter(uint32_t uc);
uint32_t foil_uchar_to_fullwidth(uint32_t uc);
uint32_t foil_uchar_to_singlewidth(uint32_t uc);
uint32_t foil_uchar_to_fullsize_kana(uint32_t uc);
uint32_t foil_uchar_to_small_kana(uint32_t uc);

bool foil_uchar_is_emoji(uint32_t uc);
bool foil_uchar_is_emoji_presentation(uint32_t uc);
bool foil_uchar_is_emoji_modifier(uint32_t uc);
bool foil_uchar_is_emoji_modifier_base(uint32_t uc);
bool foil_uchar_is_extended_pictographic(uint32_t uc);

typedef struct _foil_emoji_iterator {
    const uint32_t *text_start;
    const uint32_t *text_end;
    const uint32_t *start;
    const uint32_t *end;
    bool is_emoji;

    unsigned char *types;
    unsigned int n_chars;
    unsigned int cursor;
} foil_emoji_iterator;

foil_emoji_iterator* foil_emoji_iterator_init(foil_emoji_iterator *iter,
        const uint32_t* ucs, int nr_ucs, uint8_t* types_buff);
bool foil_emoji_iterator_next(foil_emoji_iterator *iter);
void foil_emoji_iterator_fini(foil_emoji_iterator *iter);

foil_langcode_t foil_langcode_from_iso639_1(const char *iso639_1_code);
const char* foil_langcode_to_iso639_1(foil_langcode_t lc);

/**
 * \fn foil_ustr_from_utf8_until_paragraph_boundary
 * \brief Convert a UTF-8 character string to a Unicode character
 *      (uint32_t) string until the end of text (null character) or an
 *      explicit paragraph boundary encountered.
 *
 * This function calculates and allocates the uint32_t string from a multi-byte
 * string until it encounters the end of text (null character) or an explicit
 * paragraph boundary. It also processes the text according to the specified
 * white space rule \a wsr and the text transformation rule \a ctr.
 *
 * The implementation of this function conforms to the CSS Text Module Level 3:
 *
 * https://www.w3.org/TR/css-text-3/
 *
 * Note that you are responsible for freeing the uint32_t string allocated
 * by this function.
 *
 * \param lang The language code.
 * \param mstr The pointer to the multi-byte string.
 * \param mstr_len The length of \a mstr in bytes.
 * \param wsr The white space rule; see \a white_space_rules.
 * \param uchars The pointer to a buffer to store the address of the
 *      uint32_t array which contains the Unicode character values.
 * \param nr_uchars The buffer to store the number of the allocated
 *      uint32_t array.
 *
 * \return The number of the bytes consumed in \a mstr; zero on error.
 */
size_t foil_ustr_from_utf8_until_paragraph_boundary(
        const char* mstr, size_t mstr_len, uint8_t wsr,
        uint32_t** uchars, size_t* nr_uchars);

/**
 * \fn foil_ustr_get_breaks
 * \brief Calculate the breaking opportunities of a uint32_t string under
 *      the specified rules and line breaking policy.
 *
 * This function calculates the breaking opportunities of the Unicode characters
 * under the specified the writing system \a writing_system, the word breaking
 * rule \a wbr, and the line breaking policy \a lbp. This function also
 * transforms the character according to the text transformation rule \a ctr.
 *
 * The implementation of this function conforms to UNICODE LINE BREAKING
 * ALGORITHM:
 *
 * https://www.unicode.org/reports/tr14/tr14-39.html
 *
 * and UNICODE TEXT SEGMENTATION:
 *
 * https://www.unicode.org/reports/tr29/tr29-33.html
 *
 * and the CSS Text Module Level 3:
 *
 * https://www.w3.org/TR/css-text-3/
 *
 * The function will return if it encounters the end of the text.
 *
 * Note that you are responsible for freeing the break opportunities array
 * allocated by this function if it allocates the buffer.
 *
 * \param lang_code The language code; not used so far, reserved for future.
 * \param ctr The character transformation rule; see \a char_transform_rules.
 * \param wbr The word breaking rule; see \a word_break_rules.
 * \param lbp The line breaking policy; see \a line_break_policies.
 * \param ucs The Unicode characters.
 * \param nr_ucs The length of the uint32_t string.
 * \param break_oppos The pointer to a buffer to store the address of a
 *        Uint16 array which will return the break opportunities of the uchars.
 *        If the buffer contains a NULL value, this function will try to
 *        allocate a new space for the break opportunities.
 *        Note that the length of this array is always one longer than
 *        the Unicode array. The first unit of the array stores the
 *        break opportunity before the first uchar, and the others store
 *        the break opportunities after other gyphs.
 *        The break opportunity can be one of the following values:
 *          - FOIL_BOV_LB_MANDATORY\n
 *            The mandatory breaking.
 *          - FOIL_BOV_LB_NOTALLOWED\n
 *            No breaking allowed after the uchar definitely.
 *          - FOIL_BOV_LB_ALLOWED\n
 *            Breaking allowed after the uchar.
 *
 * \return The length of break oppoortunities array; zero on error.
 */
size_t foil_ustr_get_breaks(foil_langcode_t langcode,
        uint8_t ctr, uint8_t wbr, uint8_t lbp,
        uint32_t* ucs, size_t nr_ucs, foil_break_oppo_t** break_oppos);

/**
 * \brief Get the visual extent info of all glyphs fitting in the specified
 *      maximal output extent.
 *
 * This function gets the visual extent information of a glyph string which can
 * fit a line with the specified maximal extent.
 *
 * \param uchars The pointer to the unicode string.
 * \param nr_uchars The number of characters.
 * \param break_oppos The pointer to the break opportunities array of the glyphs.
 *      It should be returned by \a foil_ustr_get_breaks. However, the caller
 *      should skip the first unit (the break opportunity before the first glyph)
 *      when passing the pointer to this function.
 * \param render_flags The render flags; see \a glyph_render_flags.
 * \param x The x-position of first glyph.
 * \param y The y-position of first glyph.
 * \param letter_spacing This parameter specifies additional spacing
 *      (commonly called tracking) between adjacent glyphs.
 * \param word_spacing This parameter specifies the additional spacing between
 *      words.
 * \param tab_size The tab size used to render preserved tab characters.
 * \param max_extent The maximal output extent value. No limit when it is < 0.
 * \param line_size The buffer to store the line extent info; can be NULL.
 * \param glyph_ext_info The buffer to store the extent info of all glyphs
 *      which can fit in the max extent; can be NULL.
 * \param glyph_pos The buffer to store the positions and orientations of
 *      all glyphs which can fit in the max extent; cannot be NULL.
 *
 * \return The number of characters which can be fit to the maximal extent.
 *      The glyphs and the extent info of every glyphs which are fit in
 *      the maximal extent will be returned through \a glyphs and
 *      \a glyph_ext_info (if it was not NULL), and the
 *      line extent info will be returned through \a line_size
 *      if it was not NULL. Note the function will return immediately if
 *      it encounters a mandatory breaking.
 *
 * \note This function ignore the special types (such as diacritic mark,
 *      vowel, contextual form, ligature, and so on) of the Unicode characters.
 *
 * \note Any invisible format character including SOFT HYPHEN (U+00AD)
 *      will be ignored (suppressed).
 *
 * \note The position coordinates of the first glyph are
 *      with respect to the top-left corner of the output rectangle
 *      if the writing mode is FOIL_GRF_WRITING_MODE_HORIZONTAL_TB or
 *      FOIL_GRF_WRITING_MODE_VERTICAL_LR, otherwise they are with respect
 *      to the top-right corner of the output rectangle. However,
 *      the positions contained in \a glyph_pos are always with respect to
 *      the top-left corner of the resulting output line rectangle.
 *
 * \note This function ignore the orientation flags and always treated as
 *      FOIL_GRF_TEXT_ORIENTATION_UPRIGHT.
 */
size_t foil_ustr_get_glyphs_extent_simple(const uint32_t* ucs, size_t nr_ucs,
        const foil_break_oppo_t* break_oppos, uint32_t render_flags,
        int x, int y, int letter_spacing, int word_spacing, int tab_size,
        int max_extent, foil_size* line_size,
        foil_glyph_extinfo* glyph_ext_info, foil_glyph_pos* glyph_pos);

#ifdef __cplusplus
}
#endif

#endif /* purc_foil_unicode_h */
