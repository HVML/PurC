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
#define FOIL_UCHAR_SPACE             0x0020
#define FOIL_UCHAR_SHY               0x00AD
#define FOIL_UCHAR_IDSPACE           0x3000
#define FOIL_UCHAR_LINE_SEPARATOR    0x2028

/**
 * \def FOIL_WSR_NORMAL
 *
 * \brief This value directs \a GetUCharsUntilParagraphBoundary
 * collapses sequences of white space into a single character.
 * Lines may wrap at allowed soft wrap opportunities.
 */
#define FOIL_WSR_NORMAL          0x00

/**
 * \def FOIL_WSR_PRE
 *
 * \brief This value prevents \a GetUCharsUntilParagraphBoundary from collapsing
 * sequences of white space. Segment breaks such as line feeds are
 * preserved as forced line breaks. Lines only break at forced line breaks;
 * content that does not fit within the specified extent overflows it.
 */
#define FOIL_WSR_PRE             0x01

/**
 * \def FOIL_WSR_NOWRAP
 *
 * \brief Like \a FOIL_WSR_NORMAL, this value collapses white spaces; but like
 * \a FOIL_WSR_PRE, it does not allow wrapping.
 */
#define FOIL_WSR_NOWRAP          0x02

/**
 * \def FOIL_WSR_PRE_WRAP
 *
 * \brief Like \a FOIL_WSR_PRE, this value preserves white space; but like
 * \a FOIL_WSR_NORMAL, it allows wrapping.
 */
#define FOIL_WSR_PRE_WRAP        0x03

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
 * of \a GetUCharsUntilParagraphBoundary will conform
 * to CSS Text Module Level 3.
 */
#define FOIL_WSR_BREAK_SPACES    0x04

/**
 * \def FOIL_WSR_PRE_LINE
 *
 * \brief Like WS_NORMAL, this value collapses consecutive spaces and
 * allows wrapping, but preserves segment breaks in the source
 * as forced line breaks.
 */
#define FOIL_WSR_PRE_LINE        0x05

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
#define FOIL_CTR_CAPITALIZE      0x01

/**
 * \def FOIL_CTR_UPPERCASE
 *
 * \brief Puts all letters in uppercase.
 */
#define FOIL_CTR_UPPERCASE       0x02

/**
 * \def FOIL_CTR_LOWERCASE
 *
 * \brief Puts all letters in lowercase.
 */
#define FOIL_CTR_LOWERCASE       0x03

/**
 * \def FOIL_CTR_FULL_WIDTH
 *
 * \brief Puts all typographic character units in fullwidth form.
 * If a character does not have a corresponding fullwidth form,
 * it is left as is. This value is typically used to typeset
 * Latin letters and digits as if they were ideographic characters.
 */
#define FOIL_CTR_FULL_WIDTH      0x10

/**
 * \def FOIL_CTR_FULL_SIZE_KANA
 *
 * \brief Converts all small Kana characters to the equivalent
 * full-size Kana. This value is typically used for ruby annotation
 * text, where authors may want all small Kana to be drawn as large
 * Kana to compensate for legibility issues at the small font sizes
 * typically used in ruby.
 */
#define FOIL_CTR_FULL_SIZE_KANA  0x20

/**
 * \def FOIL_WBR_NORMAL
 *
 * \brief Words break according to their customary rules, as defined
 * by UNICODE LINE BREAKING ALGORITHM.
 */
#define FOIL_WBR_NORMAL          0x00

/**
 * \def FOIL_WBR_BREAK_ALL
 *
 * \brief Breaking is allowed within “words”.
 */
#define FOIL_WBR_BREAK_ALL       0x01

/**
 * \def FOIL_WBR_KEEP_ALL
 *
 * \brief Breaking is forbidden within “words”.
 */
#define FOIL_WBR_KEEP_ALL        0x02

/**
 * \def FOIL_LBP_NORMAL
 *
 * \brief Breaks text using the most common set of line-breaking rules.
 */
#define FOIL_LBP_NORMAL          0x00

/**
 * \def FOIL_LBP_LOOSE
 *
 * \brief Breaks text using the least restrictive set of line-breaking rules.
 * Typically used for short lines, such as in newspapers.
 */
#define FOIL_LBP_LOOSE           0x01

/**
 * \def FOIL_LBP_STRICT
 *
 * \brief Breaks text using the most stringent set of line-breaking rules.
 */
#define FOIL_LBP_STRICT          0x02

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
#define FOIL_LBP_ANYWHERE        0x03

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
 * \param logfont The logfont used to parse the string.
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
size_t foil_ustr_from_utf8_until_paragraph_boundary(const char* mstr,
        size_t mstr_len, uint8_t wsr, uint32_t** uchars, size_t* nr_uchars);

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
 * \param ucs The uint32_t string.
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
 *          - BOV_LB_MANDATORY\n
 *            The mandatory breaking.
 *          - BOV_LB_NOTALLOWED\n
 *            No breaking allowed after the uchar definitely.
 *          - BOV_LB_ALLOWED\n
 *            Breaking allowed after the uchar.
 *
 * \return The length of break oppoortunities array; zero on error.
 */
size_t foil_ustr_get_breaks(foil_langcode_t langcode,
        uint8_t ctr, uint8_t wbr, uint8_t lbp,
        uint32_t* ucs, size_t nr_ucs, foil_break_oppo_t** break_oppos);

#ifdef __cplusplus
}
#endif

#endif /* purc_foil_unicode_h */
