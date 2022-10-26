/* 
 * Copyright (C) 2020 Beijing FMSoft Technologies Co., Ltd.
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
 * 
 * Or,
 * 
 * As this component is a program released under LGPLv3, which claims
 * explicitly that the program could be modified by any end user
 * even if the program is conveyed in non-source form on the system it runs.
 * Generally, if you distribute this program in embedded devices,
 * you might not satisfy this condition. Under this situation or you can
 * not accept any condition of LGPLv3, you need to get a commercial license
 * from FMSoft, along with a patent license for the patents owned by FMSoft.
 * 
 * If you have got a commercial/patent license of this program, please use it
 * under the terms and conditions of the commercial license.
 * 
 * For more information about the commercial license and patent license,
 * please refer to
 * <https://hybridos.fmsoft.cn/blog/hybridos-licensing-policy/>.
 * 
 * Also note that the LGPLv3 license does not apply to any entity in the
 * Exception List published by Beijing FMSoft Technologies Co., Ltd.
 * 
 * If you are or the entity you represent is listed in the Exception List,
 * the above open source or free software license does not apply to you
 * or the entity you represent. Regardless of the purpose, you should not
 * use the software in any way whatsoever, including but not limited to
 * downloading, viewing, copying, distributing, compiling, and running.
 * If you have already downloaded it, you MUST destroy all of its copies.
 * 
 * The Exception List is published by FMSoft and may be updated
 * from time to time. For more information, please see
 * <https://www.fmsoft.cn/exception-list>.
 */ 

#include "config.h"
#include "FilterBase.h"

IGNORE_WARNINGS_BEGIN("implicit-fallthrough")

namespace PurCFetcher {

#define PARAGRAPH_SEPARATOR 0x2029
#define PARAGRAPH_SEPARATOR_STRING "\xE2\x80\xA9"

#define BREAK_TYPE_SAFE(btype)            \
    ((btype) < G_N_ELEMENTS(line_break_indexes) ? (btype) : G_UNICODE_BREAK_UNKNOWN)

typedef enum
{
  BREAK_ALREADY_HANDLED,   /* didn't use the table */
  BREAK_PROHIBITED, /* no break, even if spaces intervene */
  BREAK_IF_SPACES,  /* "indirect break" (only if there are spaces) */
  BREAK_ALLOWED     /* "direct break" (can always break here) */
  /* TR 14 has one more break-opportunity class,
   * "indirect break opportunity for combining marks following a space"
   * but we handle that inline in the code.
   */
} BreakOpportunity;

enum
{
  INDEX_OPEN_PUNCTUATION,
  INDEX_CLOSE_PUNCTUATION,
  INDEX_QUOTATION,
  INDEX_NON_BREAKING_GLUE,
  INDEX_NON_STARTER,
  INDEX_EXCLAMATION,
  INDEX_SYMBOL,
  INDEX_INFIX_SEPARATOR,
  INDEX_PREFIX,
  INDEX_POSTFIX,
  INDEX_NUMERIC,
  INDEX_ALPHABETIC,
  INDEX_IDEOGRAPHIC,
  INDEX_INSEPARABLE,
  INDEX_HYPHEN,
  INDEX_AFTER,
  INDEX_BEFORE,
  INDEX_BEFORE_AND_AFTER,
  INDEX_ZERO_WIDTH_SPACE,
  INDEX_COMBINING_MARK,
  INDEX_WORD_JOINER,

  /* End of the table */

  INDEX_END_OF_TABLE,

  /* The following are not in the tables */
  INDEX_MANDATORY,
  INDEX_CARRIAGE_RETURN,
  INDEX_LINE_FEED,
  INDEX_SURROGATE,
  INDEX_CONTINGENT,
  INDEX_SPACE,
  INDEX_COMPLEX_CONTEXT,
  INDEX_AMBIGUOUS,
  INDEX_UNKNOWN,
  INDEX_NEXT_LINE,
  INDEX_HANGUL_L_JAMO,
  INDEX_HANGUL_V_JAMO,
  INDEX_HANGUL_T_JAMO,
  INDEX_HANGUL_LV_SYLLABLE,
  INDEX_HANGUL_LVT_SYLLABLE,
};

static const int line_break_indexes[] = {
    INDEX_MANDATORY,
    INDEX_CARRIAGE_RETURN,
    INDEX_LINE_FEED,
    INDEX_COMBINING_MARK,
    INDEX_SURROGATE,
    INDEX_ZERO_WIDTH_SPACE,
    INDEX_INSEPARABLE,
    INDEX_NON_BREAKING_GLUE,
    INDEX_CONTINGENT,
    INDEX_SPACE,
    INDEX_AFTER,
    INDEX_BEFORE,
    INDEX_BEFORE_AND_AFTER,
    INDEX_HYPHEN,
    INDEX_NON_STARTER,
    INDEX_OPEN_PUNCTUATION,
    INDEX_CLOSE_PUNCTUATION,
    INDEX_QUOTATION,
    INDEX_EXCLAMATION,
    INDEX_IDEOGRAPHIC,
    INDEX_NUMERIC,
    INDEX_INFIX_SEPARATOR,
    INDEX_SYMBOL,
    INDEX_ALPHABETIC,
    INDEX_PREFIX,
    INDEX_POSTFIX,
    INDEX_COMPLEX_CONTEXT,
    INDEX_AMBIGUOUS,
    INDEX_UNKNOWN,
    INDEX_NEXT_LINE,
    INDEX_WORD_JOINER,
    INDEX_HANGUL_L_JAMO,
    INDEX_HANGUL_V_JAMO,
    INDEX_HANGUL_T_JAMO,
    INDEX_HANGUL_LV_SYLLABLE,
    INDEX_HANGUL_LVT_SYLLABLE
};

typedef enum
{
    STATE_SENTENCE_OUTSIDE,
    STATE_SENTENCE_BODY,
    STATE_SENTENCE_TERM,
    STATE_SENTENCE_POST_TERM_CLOSE,
    STATE_SENTENCE_POST_TERM_SPACE,
    STATE_SENTENCE_POST_TERM_SEP,
    STATE_SENTENCE_DOT,
    STATE_SENTENCE_POST_DOT_CLOSE,
    STATE_SENTENCE_POST_DOT_SPACE,
    STATE_SENTENCE_POST_DOT_OPEN,
    /* never include line/para separators in a sentence for now */
    /* This isn't in the spec, but I can't figure out why they'd include
     * one line/para separator in lines ending with Term but not with
     * period-terminated lines, so I'm doing it for the dot lines also
     */
    STATE_SENTENCE_POST_DOT_SEP
} SentenceState;

typedef enum
{
    WordNone,
    WordLetters,
    WordNumbers
} WordType;

typedef enum
{
    JAMO_L, /* G_UNICODE_BREAK_HANGUL_L_JAMO */
    JAMO_V, /* G_UNICODE_BREAK_HANGUL_V_JAMO */
    JAMO_T, /* G_UNICODE_BREAK_HANGUL_T_JAMO */
    JAMO_LV, /* G_UNICODE_BREAK_HANGUL_LV_SYLLABLE */
    JAMO_LVT, /* G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE */
    NO_JAMO /* Other */
} JamoType;

static const BreakOpportunity row_OPEN_PUNCTUATION[INDEX_END_OF_TABLE] = {
  BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_CLOSE_PUNCTUATION[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_QUOTATION[INDEX_END_OF_TABLE] = {
  BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_NON_BREAKING_GLUE[INDEX_END_OF_TABLE] = {
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_NON_STARTER[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_EXCLAMATION[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_SYMBOL[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_INFIX_SEPARATOR[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_PREFIX[INDEX_END_OF_TABLE] = {
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_POSTFIX[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_NUMERIC[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_ALPHABETIC[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_IDEOGRAPHIC[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_INSEPARABLE[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_HYPHEN[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_AFTER[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_BEFORE[INDEX_END_OF_TABLE] = {
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_BEFORE_AND_AFTER[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_ZERO_WIDTH_SPACE[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED, BREAK_ALLOWED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED
};

static const BreakOpportunity row_COMBINING_MARK[INDEX_END_OF_TABLE] = {
  BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_ALLOWED, BREAK_ALLOWED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};

static const BreakOpportunity row_WORD_JOINER[INDEX_END_OF_TABLE] = {
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_IF_SPACES,
  BREAK_IF_SPACES, BREAK_IF_SPACES, BREAK_PROHIBITED, BREAK_PROHIBITED,
  BREAK_PROHIBITED
};


static const BreakOpportunity *const line_break_rows[INDEX_END_OF_TABLE] = {
  row_OPEN_PUNCTUATION, /* INDEX_OPEN_PUNCTUATION */
  row_CLOSE_PUNCTUATION, /* INDEX_CLOSE_PUNCTUATION */
  row_QUOTATION, /* INDEX_QUOTATION */
  row_NON_BREAKING_GLUE, /* INDEX_NON_BREAKING_GLUE */
  row_NON_STARTER, /* INDEX_NON_STARTER */
  row_EXCLAMATION, /* INDEX_EXCLAMATION */
  row_SYMBOL, /* INDEX_SYMBOL */
  row_INFIX_SEPARATOR, /* INDEX_INFIX_SEPARATOR */
  row_PREFIX, /* INDEX_PREFIX */
  row_POSTFIX, /* INDEX_POSTFIX */
  row_NUMERIC, /* INDEX_NUMERIC */
  row_ALPHABETIC, /* INDEX_ALPHABETIC */
  row_IDEOGRAPHIC, /* INDEX_IDEOGRAPHIC */
  row_INSEPARABLE, /* INDEX_INSEPARABLE */
  row_HYPHEN, /* INDEX_HYPHEN */
  row_AFTER, /* INDEX_AFTER */
  row_BEFORE, /* INDEX_BEFORE */
  row_BEFORE_AND_AFTER, /* INDEX_BEFORE_AND_AFTER */
  row_ZERO_WIDTH_SPACE, /* INDEX_ZERO_WIDTH_SPACE */
  row_COMBINING_MARK, /* INDEX_COMBINING_MARK */
  row_WORD_JOINER /* INDEX_WORD_JOINER */
};


#define IS_JAMO(btype)              \
    ((btype >= G_UNICODE_BREAK_HANGUL_L_JAMO) && (btype <= G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE))
#define JAMO_TYPE(btype)      \
    ((JamoType)(IS_JAMO(btype) ? (btype - G_UNICODE_BREAK_HANGUL_L_JAMO) : NO_JAMO))
#define BREAK_TYPE_SAFE(btype)            \
     ((btype) < G_N_ELEMENTS(line_break_indexes) ? (btype) : G_UNICODE_BREAK_UNKNOWN)
#define BREAK_INDEX(btype)                \
     (line_break_indexes[(btype)])
#define BREAK_ROW(before_type)            \
     (line_break_rows[BREAK_INDEX (before_type)])
#define BREAK_OP(before_type, after_type) \
     (BREAK_ROW (before_type)[BREAK_INDEX (after_type)])
#define IN_BREAK_TABLE(btype)             \
     ((btype) < G_N_ELEMENTS(line_break_indexes) && BREAK_INDEX((btype)) < INDEX_END_OF_TABLE)


typedef struct _CharJamoProps
{
    JamoType start, end;
} CharJamoProps;

static const CharJamoProps HangulJamoProps[] = {
  {JAMO_L, JAMO_L}, /* JAMO_L */
  {JAMO_V, JAMO_V}, /* JAMO_V */
  {JAMO_T, JAMO_T}, /* JAMO_T */
  {JAMO_L, JAMO_V}, /* JAMO_LV */
  {JAMO_L, JAMO_T}, /* JAMO_LVT */
  {NO_JAMO, NO_JAMO}    /* NO_JAMO */
};

#define JAPANESE(wc) ((wc) >= 0x2F00 && (wc) <= 0x30FF)
#define KANJI(wc)    ((wc) >= 0x2F00 && (wc) <= 0x2FDF)
#define HIRAGANA(wc) ((wc) >= 0x3040 && (wc) <= 0x309F)
#define KATAKANA(wc) ((wc) >= 0x30A0 && (wc) <= 0x30FF)

#define LATIN(wc) (((wc) >= 0x0020 && (wc) <= 0x02AF) || ((wc) >= 0x1E00 && (wc) <= 0x1EFF))
#define CYRILLIC(wc) (((wc) >= 0x0400 && (wc) <= 0x052F))
#define GREEK(wc) (((wc) >= 0x0370 && (wc) <= 0x3FF) || ((wc) >= 0x1F00 && (wc) <= 0x1FFF))
#define KANA(wc) ((wc) >= 0x3040 && (wc) <= 0x30FF)
#define HANGUL(wc) ((wc) >= 0xAC00 && (wc) <= 0xD7A3)
#define BACKSPACE_DELETES_CHARACTER(wc) (!LATIN (wc) && !CYRILLIC (wc) && !GREEK (wc) && !KANA(wc) && !HANGUL(wc))


void UCharBreaker::genBreaks(const char* text, int length, struct UCharBreakAttr* attrs)
{
    /* The rationale for all this is in section 5.15 of the Unicode 3.0 book,
     * the line breaking stuff is also in TR14 on unicode.org
     */

    /* This is a default break implementation that should work for nearly all
     * languages. Language engines can override it optionally.
     */

    /* FIXME one cheesy optimization here would be to memset attrs to 0
     * before we start, and then never assign %FALSE to anything
     */

    const gchar *next;
    gint i;

    gunichar prev_wc;
    gunichar next_wc;

    JamoType prev_jamo;

    GUnicodeBreakType next_break_type;
    GUnicodeType prev_type;
    GUnicodeBreakType prev_break_type; /* skips spaces */
    gboolean prev_was_break_space;

    /* See Grapheme_Cluster_Break Property Values table of UAX#29 */
    typedef enum
    {
        GB_Other,
        GB_ControlCRLF,
        GB_Extend,
        GB_Prepend,
        GB_SpacingMark,
        GB_InHangulSyllable, /* Handles all of L, V, T, LV, LVT rules */
    } GraphemeBreakType;
    GraphemeBreakType prev_GB_type = GB_Other;

    /* See Word_Break Property Values table of UAX#29 */
    typedef enum
    {
        WB_Other,
        WB_NewlineCRLF,
        WB_ExtendFormat,
        WB_Katakana,
        WB_ALetter,
        WB_MidNumLet,
        WB_MidLetter,
        WB_MidNum,
        WB_Numeric,
        WB_ExtendNumLet,
    } WordBreakType;
    WordBreakType prev_prev_WB_type = WB_Other, prev_WB_type = WB_Other;
    gint prev_WB_i = -1;

    WordType current_word_type = WordNone;
    gunichar last_word_letter = 0;
    gunichar base_character = 0;

    SentenceState sentence_state = STATE_SENTENCE_OUTSIDE;
    /* Tracks what will be the end of the sentence if a period is
     * determined to actually be a sentence-ending period.
     */
    gint possible_sentence_end = -1;
    /* possible sentence break before Open* after a period-ended sentence */
    gint possible_sentence_boundary = -1;
    gboolean almost_done = FALSE;
    gboolean done = FALSE;

    g_return_if_fail (length == 0 || text != NULL);
    g_return_if_fail (attrs != NULL);

    next = text;

    prev_type = G_UNICODE_PARAGRAPH_SEPARATOR;
    prev_break_type = G_UNICODE_BREAK_UNKNOWN;
    prev_was_break_space = FALSE;
    prev_wc = 0;
    prev_jamo = NO_JAMO;

    if (length == 0 || *text == '\0')
    {
        next_wc = PARAGRAPH_SEPARATOR;
        almost_done = TRUE;
    }
    else
        next_wc = g_utf8_get_char (next);

    next_break_type = g_unichar_break_type (next_wc);
    next_break_type = BREAK_TYPE_SAFE (next_break_type);

    for (i = 0; !done ; i++)
    {
        GUnicodeType type;
        gunichar wc;
        GUnicodeBreakType break_type;
        BreakOpportunity break_op;
        JamoType jamo;
        gboolean makes_hangul_syllable;

        /* UAX#29 boundaries */
        gboolean is_grapheme_boundary;
        gboolean is_word_boundary;


        wc = next_wc;
        break_type = next_break_type;

        if (almost_done)
        {
            /*
             * If we have already reached the end of @text g_utf8_next_char()
             * may not increment next
             */
            next_wc = 0;
            next_break_type = G_UNICODE_BREAK_UNKNOWN;
            done = TRUE;
        }
        else
        {
            next = g_utf8_next_char (next);

            if ((length >= 0 && next >= text + length) || *next == '\0')
            {
                /* This is how we fill in the last element (end position) of the
                 * attr array - assume there's a paragraph separators off the end
                 * of @text.
                 */
                next_wc = PARAGRAPH_SEPARATOR;
                almost_done = TRUE;
            }
            else
                next_wc = g_utf8_get_char (next);

            next_break_type = g_unichar_break_type (next_wc);
            next_break_type = BREAK_TYPE_SAFE (next_break_type);
        }

        type = g_unichar_type (wc);
        jamo = JAMO_TYPE (break_type);

        /* Determine wheter this forms a Hangul syllable with prev. */
        if (jamo == NO_JAMO)
            makes_hangul_syllable = FALSE;
        else
        {
            JamoType prev_end   = HangulJamoProps[prev_jamo].end  ;
            JamoType this_start = HangulJamoProps[     jamo].start;

            /* See comments before IS_JAMO */
            makes_hangul_syllable = (prev_end == this_start) || (prev_end + 1 == this_start);
        }

        switch (type)
        {
            case G_UNICODE_SPACE_SEPARATOR:
            case G_UNICODE_LINE_SEPARATOR:
            case G_UNICODE_PARAGRAPH_SEPARATOR:
                attrs[i].is_white = TRUE;
                break;
            default:
                if (wc == '\t' || wc == '\n' || wc == '\r' || wc == '\f')
                    attrs[i].is_white = TRUE;
                else
                    attrs[i].is_white = FALSE;
                break;
        }

        /* Just few spaces have variable width. So explicitly mark them.
        */
        attrs[i].is_expandable_space = (0x0020 == wc || 0x00A0 == wc);

        /* ---- UAX#29 Grapheme Boundaries ---- */
        {
            GraphemeBreakType GB_type;
            /* Find the GraphemeBreakType of wc */
            GB_type = GB_Other;
            switch ((int) type)
            {
                case G_UNICODE_FORMAT:
                    if (wc == 0x200C || wc == 0x200D)
                    {
                        GB_type = GB_Extend; /* U+200C and U+200D are Other_Grapheme_Extend */
                        break;
                    }
                    /* fall through */
                case G_UNICODE_CONTROL:
                case G_UNICODE_LINE_SEPARATOR:
                case G_UNICODE_PARAGRAPH_SEPARATOR:
                case G_UNICODE_SURROGATE:
                    GB_type = GB_ControlCRLF;
                    break;

                case G_UNICODE_UNASSIGNED:
                    /* Unassigned default ignorables */
                    if ((wc >= 0xFFF0 && wc <= 0xFFF8) ||
                            (wc >= 0xE0000 && wc <= 0xE0FFF))
                    {
                        GB_type = GB_ControlCRLF;
                        break;
                    }

                case G_UNICODE_OTHER_LETTER:
                    if (makes_hangul_syllable)
                        GB_type = GB_InHangulSyllable;
                    break;

                case G_UNICODE_MODIFIER_LETTER:
                    if (wc >= 0xFF9E && wc <= 0xFF9F)
                        GB_type = GB_Extend; /* Other_Grapheme_Extend */
                    break;

                case G_UNICODE_SPACING_MARK:
                    GB_type = GB_SpacingMark; /* SpacingMark */
                    if (wc >= 0x0900)
                    {
                        if (wc == 0x09BE || wc == 0x09D7 ||
                                wc == 0x0B3E || wc == 0x0B57 || wc == 0x0BBE || wc == 0x0BD7 ||
                                wc == 0x0CC2 || wc == 0x0CD5 || wc == 0x0CD6 ||
                                wc == 0x0D3E || wc == 0x0D57 || wc == 0x0DCF || wc == 0x0DDF ||
                                wc == 0x1D165 || (wc >= 0x1D16E && wc <= 0x1D172))
                            GB_type = GB_Extend; /* Other_Grapheme_Extend */
                    }
                    break;

                case G_UNICODE_ENCLOSING_MARK:
                case G_UNICODE_NON_SPACING_MARK:
                    GB_type = GB_Extend; /* Grapheme_Extend */
                    break;
            }

            /* Grapheme Cluster Boundary Rules */
            /* We apply Rules GB1 and GB2 at the end of the function */
            if (wc == '\n' && prev_wc == '\r')
                is_grapheme_boundary = FALSE; /* Rule GB3 */
            else if (prev_GB_type == GB_ControlCRLF || GB_type == GB_ControlCRLF)
                is_grapheme_boundary = TRUE; /* Rules GB4 and GB5 */
            else if (GB_type == GB_InHangulSyllable)
                is_grapheme_boundary = FALSE; /* Rules GB6, GB7, GB8 */
            else if (GB_type == GB_Extend)
                is_grapheme_boundary = FALSE; /* Rule GB9 */
            else if (GB_type == GB_SpacingMark)
                is_grapheme_boundary = FALSE; /* Rule GB9a */
            else if (prev_GB_type == GB_Prepend)
                is_grapheme_boundary = FALSE; /* Rule GB9b */
            else
                is_grapheme_boundary = TRUE;  /* Rule GB10 */

            prev_GB_type = GB_type;

            attrs[i].is_cursor_position = is_grapheme_boundary;
            /* If this is a grapheme boundary, we have to decide if backspace
             * deletes a character or the whole grapheme cluster */
            if (is_grapheme_boundary)
                attrs[i].backspace_deletes_character = BACKSPACE_DELETES_CHARACTER (base_character);
            else
                attrs[i].backspace_deletes_character = FALSE;
        }

        /* ---- UAX#29 Word Boundaries ---- */
        {
            is_word_boundary = FALSE;
            if (is_grapheme_boundary) /* Rules WB3 and WB4 */
            {
                GUnicodeScript script;
                WordBreakType WB_type;

                script = g_unichar_get_script (wc);

                /* Find the WordBreakType of wc */
                WB_type = WB_Other;

                if (script == G_UNICODE_SCRIPT_KATAKANA)
                    WB_type = WB_Katakana;

                if (WB_type == WB_Other)
                    switch (wc >> 8)
                    {
                        case 0x30:
                            if (wc == 0x3031 || wc == 0x3032 || wc == 0x3033 || wc == 0x3034 || wc == 0x3035 ||
                                    wc == 0x309b || wc == 0x309c || wc == 0x30a0 || wc == 0x30fc)
                                WB_type = WB_Katakana; /* Katakana exceptions */
                            break;
                        case 0xFF:
                            if (wc == 0xFF70)
                                WB_type = WB_Katakana; /* Katakana exceptions */
                            else if (wc >= 0xFF9E && wc <= 0xFF9F)
                                WB_type = WB_ExtendFormat; /* Other_Grapheme_Extend */
                            break;
                        case 0x05:
                            if (wc == 0x05F3)
                                WB_type = WB_ALetter; /* ALetter exceptions */
                            break;
                    }

                if (WB_type == WB_Other)
                    switch ((int) break_type)
                    {
                        case G_UNICODE_BREAK_NUMERIC:
                            if (wc != 0x066C)
                                WB_type = WB_Numeric; /* Numeric */
                            break;
                        case G_UNICODE_BREAK_INFIX_SEPARATOR:
                            if (wc != 0x003A && wc != 0xFE13 && wc != 0x002E)
                                WB_type = WB_MidNum; /* MidNum */
                            break;
                    }

                if (WB_type == WB_Other)
                    switch ((int) type)
                    {
                        case G_UNICODE_CONTROL:
                            if (wc != 0x000D && wc != 0x000A && wc != 0x000B && wc != 0x000C && wc != 0x0085)
                                break;
                            /* fall through */
                        case G_UNICODE_LINE_SEPARATOR:
                        case G_UNICODE_PARAGRAPH_SEPARATOR:
                            WB_type = WB_NewlineCRLF; /* CR, LF, Newline */
                            break;

                        case G_UNICODE_FORMAT:
                        case G_UNICODE_SPACING_MARK:
                        case G_UNICODE_ENCLOSING_MARK:
                        case G_UNICODE_NON_SPACING_MARK:
                            WB_type = WB_ExtendFormat; /* Extend, Format */
                            break;

                        case G_UNICODE_CONNECT_PUNCTUATION:
                            WB_type = WB_ExtendNumLet; /* ExtendNumLet */
                            break;

                        case G_UNICODE_INITIAL_PUNCTUATION:
                        case G_UNICODE_FINAL_PUNCTUATION:
                            if (wc == 0x2018 || wc == 0x2019)
                                WB_type = WB_MidNumLet; /* MidNumLet */
                            break;
                        case G_UNICODE_OTHER_PUNCTUATION:
                            if (wc == 0x0027 || wc == 0x002e || wc == 0x2024 ||
                                    wc == 0xfe52 || wc == 0xff07 || wc == 0xff0e)
                                WB_type = WB_MidNumLet; /* MidNumLet */
                            else if (wc == 0x00b7 || wc == 0x05f4 || wc == 0x2027 || wc == 0x003a || wc == 0x0387 ||
                                    wc == 0xfe13 || wc == 0xfe55 || wc == 0xff1a)
                                WB_type = WB_MidLetter; /* WB_MidLetter */
                            else if (wc == 0x066c ||
                                    wc == 0xfe50 || wc == 0xfe54 || wc == 0xff0c || wc == 0xff1b)
                                WB_type = WB_MidNum; /* MidNum */
                            break;

                        case G_UNICODE_OTHER_SYMBOL:
                            if (wc >= 0x24B6 && wc <= 0x24E9) /* Other_Alphabetic */
                                goto Alphabetic;
                            break;

                        case G_UNICODE_OTHER_LETTER:
                        case G_UNICODE_LETTER_NUMBER:
                            if (wc == 0x3006 || wc == 0x3007 ||
                                    (wc >= 0x3021 && wc <= 0x3029) ||
                                    (wc >= 0x3038 && wc <= 0x303A) ||
                                    (wc >= 0x3400 && wc <= 0x4DB5) ||
                                    (wc >= 0x4E00 && wc <= 0x9FC3) ||
                                    (wc >= 0xF900 && wc <= 0xFA2D) ||
                                    (wc >= 0xFA30 && wc <= 0xFA6A) ||
                                    (wc >= 0xFA70 && wc <= 0xFAD9) ||
                                    (wc >= 0x20000 && wc <= 0x2A6D6) ||
                                    (wc >= 0x2F800 && wc <= 0x2FA1D))
                                break; /* ALetter exceptions: Ideographic */
                            goto Alphabetic;

                        case G_UNICODE_LOWERCASE_LETTER:
                        case G_UNICODE_MODIFIER_LETTER:
                        case G_UNICODE_TITLECASE_LETTER:
                        case G_UNICODE_UPPERCASE_LETTER:
Alphabetic:
                            if (break_type != G_UNICODE_BREAK_COMPLEX_CONTEXT && script != G_UNICODE_SCRIPT_HIRAGANA)
                                WB_type = WB_ALetter; /* ALetter */
                            break;
                    }

                /* Grapheme Cluster Boundary Rules */

                /* We apply Rules WB1 and WB2 at the end of the function */

                if (prev_wc == 0x3031 && wc == 0x41)
                    g_debug ("Y %d %d", prev_WB_type, WB_type);
                if (prev_WB_type == WB_NewlineCRLF && prev_WB_i + 1 == i)
                {
                    /* The extra check for prev_WB_i is to correctly handle sequences like
                     * Newline ÷ Extend × Extend
                     * since we have not skipped ExtendFormat yet.
                     */
                    is_word_boundary = TRUE; /* Rule WB3a */
                }
                else if (WB_type == WB_NewlineCRLF)
                    is_word_boundary = TRUE; /* Rule WB3b */
                else if (WB_type == WB_ExtendFormat)
                    is_word_boundary = FALSE; /* Rules WB4? */
                else if ((prev_WB_type == WB_ALetter  ||
                            prev_WB_type == WB_Numeric  ||
                            prev_WB_type == WB_ExtendNumLet) &&
                        (     WB_type == WB_ALetter  ||
                              WB_type == WB_Numeric  ||
                              WB_type == WB_ExtendNumLet))
                    is_word_boundary = FALSE; /* Rules WB5, WB8, WB9, WB10, WB13a, WB13b */
                else if ((prev_WB_type == WB_Katakana ||
                            prev_WB_type == WB_ExtendNumLet) &&
                        (     WB_type == WB_Katakana ||
                              WB_type == WB_ExtendNumLet))
                    is_word_boundary = FALSE; /* Rules WB13, WB13a, WB13b */
                else if ((prev_prev_WB_type == WB_ALetter && WB_type == WB_ALetter) &&
                        (prev_WB_type == WB_MidLetter || prev_WB_type == WB_MidNumLet))
                {
                    attrs[prev_WB_i].is_word_boundary = FALSE; /* Rule WB6 */
                    is_word_boundary = FALSE; /* Rule WB7 */
                }
                else if ((prev_prev_WB_type == WB_Numeric && WB_type == WB_Numeric) &&
                        (prev_WB_type == WB_MidNum || prev_WB_type == WB_MidNumLet))
                {
                    is_word_boundary = FALSE; /* Rule WB11 */
                    attrs[prev_WB_i].is_word_boundary = FALSE; /* Rule WB12 */
                }
                else
                    is_word_boundary = TRUE; /* Rule WB14 */

                if (WB_type != WB_ExtendFormat)
                {
                    prev_prev_WB_type = prev_WB_type;
                    prev_WB_type = WB_type;
                    prev_WB_i = i;
                }
            }

            attrs[i].is_word_boundary = is_word_boundary;
        }


        /* ---- Line breaking ---- */

        break_op = BREAK_ALREADY_HANDLED;

        g_assert (prev_break_type != G_UNICODE_BREAK_SPACE);

        attrs[i].is_char_break = FALSE;
        attrs[i].is_line_break = FALSE;
        attrs[i].is_mandatory_break = FALSE;

        if (attrs[i].is_cursor_position) /* If it's not a grapheme boundary,
                                          * it's not a line break either
                                          */
            {
                /* space followed by a combining mark is handled
                 * specially; (rule 7a from TR 14)
                 */
                if (break_type == G_UNICODE_BREAK_SPACE &&
                        next_break_type == G_UNICODE_BREAK_COMBINING_MARK)
                    break_type = G_UNICODE_BREAK_IDEOGRAPHIC;

                /* Unicode doesn't specify char wrap; we wrap around all chars
                 * except where a line break is prohibited, which means we
                 * effectively break everywhere except inside runs of spaces.
                 */
                attrs[i].is_char_break = TRUE;

                /* Make any necessary replacements first */
                switch ((int) prev_break_type)
                {
                    case G_UNICODE_BREAK_HANGUL_L_JAMO:
                    case G_UNICODE_BREAK_HANGUL_V_JAMO:
                    case G_UNICODE_BREAK_HANGUL_T_JAMO:
                    case G_UNICODE_BREAK_HANGUL_LV_SYLLABLE:
                    case G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE:
                        /* treat Jamo as IDEOGRAPHIC from now
                        */
                        prev_break_type = G_UNICODE_BREAK_IDEOGRAPHIC;
                        break;

                    case G_UNICODE_BREAK_AMBIGUOUS:
                        /* FIXME
                         * we need to resolve the East Asian width
                         * to decide what to do here
                         */
                    case G_UNICODE_BREAK_COMPLEX_CONTEXT:
                        /* FIXME
                         * language engines should handle this case...
                         */
                    case G_UNICODE_BREAK_UNKNOWN:
                        /* convert unknown, complex, ambiguous to ALPHABETIC
                        */
                        prev_break_type = G_UNICODE_BREAK_ALPHABETIC;
                        break;

                    default:
                        ;
                }

                switch ((int) prev_break_type)
                {
                    case G_UNICODE_BREAK_MANDATORY:
                    case G_UNICODE_BREAK_LINE_FEED:
                    case G_UNICODE_BREAK_NEXT_LINE:
                        attrs[i].is_line_break = TRUE;
                        attrs[i].is_mandatory_break = TRUE;
                        break;

                    case G_UNICODE_BREAK_CARRIAGE_RETURN:
                        if (wc != '\n')
                        {
                            attrs[i].is_line_break = TRUE;
                            attrs[i].is_mandatory_break = TRUE;
                        }
                        break;

                    case G_UNICODE_BREAK_CONTINGENT:
                        /* can break after 0xFFFC by default, though we might want
                         * to eventually have a PangoLayout setting or
                         * PangoAttribute that disables this, if for some
                         * application breaking after objects is not desired.
                         */
                        break_op = BREAK_ALLOWED;
                        break;

                    case G_UNICODE_BREAK_SURROGATE:
                        /* Undefined according to UTR#14, but ALLOWED in test data. */
                        break_op = BREAK_ALLOWED;
                        break;

                    default:
                        g_assert (IN_BREAK_TABLE (prev_break_type));

                        /* Note that our table assumes that combining marks
                         * are only applied to alphabetic characters;
                         * tech report 14 explains how to remove this assumption
                         * from the code, if anyone ever cares, but it shouldn't
                         * be a problem. Also this issue sort of goes
                         * away since we only look for breaks on grapheme
                         * boundaries.
                         */

                        switch ((int) break_type)
                        {
                            case G_UNICODE_BREAK_MANDATORY:
                            case G_UNICODE_BREAK_LINE_FEED:
                            case G_UNICODE_BREAK_CARRIAGE_RETURN:
                            case G_UNICODE_BREAK_NEXT_LINE:
                            case G_UNICODE_BREAK_SPACE:
                                /* These types all "pile up" at the end of lines and
                                 * get elided.
                                 */
                                break_op = BREAK_PROHIBITED;
                                break;

                            case G_UNICODE_BREAK_CONTINGENT:
                                /* break before 0xFFFC by default, eventually
                                 * make this configurable?
                                 */
                                break_op = BREAK_ALLOWED;
                                break;

                            case G_UNICODE_BREAK_SURROGATE:
                                /* Undefined according to UTR#14, but ALLOWED in test data. */
                                break_op = BREAK_ALLOWED;
                                break;

                                /* Hangul additions are from Unicode 4.1 UAX#14 */
                            case G_UNICODE_BREAK_HANGUL_L_JAMO:
                            case G_UNICODE_BREAK_HANGUL_V_JAMO:
                            case G_UNICODE_BREAK_HANGUL_T_JAMO:
                            case G_UNICODE_BREAK_HANGUL_LV_SYLLABLE:
                            case G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE:
                                /* treat Jamo as IDEOGRAPHIC from now
                                */
                                break_type = G_UNICODE_BREAK_IDEOGRAPHIC;

                                if (makes_hangul_syllable)
                                    break_op = BREAK_IF_SPACES;
                                else
                                    break_op = BREAK_ALLOWED;
                                break;

                            case G_UNICODE_BREAK_AMBIGUOUS:
                                /* FIXME:
                                 * we need to resolve the East Asian width
                                 * to decide what to do here
                                 */
                            case G_UNICODE_BREAK_COMPLEX_CONTEXT:
                                /* FIXME:
                                 * language engines should handle this case...
                                 */
                            case G_UNICODE_BREAK_UNKNOWN:
                                /* treat unknown, complex, and ambiguous like ALPHABETIC
                                 * for now
                                 */
                                break_op = BREAK_OP (prev_break_type, G_UNICODE_BREAK_ALPHABETIC);
                                break;

                            default:

                                g_assert (IN_BREAK_TABLE (break_type));
                                break_op = BREAK_OP (prev_break_type, break_type);
                                break;
                        }
                        break;
                }

                switch (break_op)
                {
                    case BREAK_PROHIBITED:
                        /* can't break here */
                        attrs[i].is_char_break = FALSE;
                        break;

                    case BREAK_IF_SPACES:
                        /* break if prev char was space */
                        if (prev_was_break_space)
                            attrs[i].is_line_break = TRUE;
                        break;

                    case BREAK_ALLOWED:
                        attrs[i].is_line_break = TRUE;
                        break;

                    case BREAK_ALREADY_HANDLED:
                        break;

                    default:
                        g_assert_not_reached ();
                        break;
                }
            }

        if (break_type != G_UNICODE_BREAK_SPACE)
        {
            prev_break_type = break_type;
            prev_was_break_space = FALSE;
            prev_jamo = jamo;
        }
        else
            prev_was_break_space = TRUE;

        /* ---- Word breaks ---- */

        /* default to not a word start/end */
        attrs[i].is_word_start = FALSE;
        attrs[i].is_word_end = FALSE;

        if (current_word_type != WordNone)
        {
            /* Check for a word end */
            switch ((int) type)
            {
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
                    if (current_word_type == WordLetters)
                    {
                        /* Japanese special cases for ending the word */
                        if (JAPANESE (last_word_letter) ||
                                JAPANESE (wc))
                        {
                            if ((HIRAGANA (last_word_letter) &&
                                        !HIRAGANA (wc)) ||
                                    (KATAKANA (last_word_letter) &&
                                     !(KATAKANA (wc) || HIRAGANA (wc))) ||
                                    (KANJI (last_word_letter) &&
                                     !(HIRAGANA (wc) || KANJI (wc))) ||
                                    (JAPANESE (last_word_letter) &&
                                     !JAPANESE (wc)) ||
                                    (!JAPANESE (last_word_letter) &&
                                     JAPANESE (wc)))
                                attrs[i].is_word_end = TRUE;
                        }
                    }
                    last_word_letter = wc;
                    break;

                case G_UNICODE_DECIMAL_NUMBER:
                case G_UNICODE_LETTER_NUMBER:
                case G_UNICODE_OTHER_NUMBER:
                    last_word_letter = wc;
                    break;

                default:
                    /* Punctuation, control/format chars, etc. all end a word. */
                    attrs[i].is_word_end = TRUE;
                    current_word_type = WordNone;
                    break;
            }
        }
        else
        {
            /* Check for a word start */
            switch ((int) type)
            {
                case G_UNICODE_LOWERCASE_LETTER:
                case G_UNICODE_MODIFIER_LETTER:
                case G_UNICODE_OTHER_LETTER:
                case G_UNICODE_TITLECASE_LETTER:
                case G_UNICODE_UPPERCASE_LETTER:
                    current_word_type = WordLetters;
                    last_word_letter = wc;
                    attrs[i].is_word_start = TRUE;
                    break;

                case G_UNICODE_DECIMAL_NUMBER:
                case G_UNICODE_LETTER_NUMBER:
                case G_UNICODE_OTHER_NUMBER:
                    current_word_type = WordNumbers;
                    last_word_letter = wc;
                    attrs[i].is_word_start = TRUE;
                    break;

                default:
                    /* No word here */
                    break;
            }
        }

        /* ---- Sentence breaks ---- */

        /* The Unicode spec specifies sentence breakpoints, so that a piece of
         * text would be partitioned into sentences, and all characters would
         * be inside some sentence. This code implements that for is_sentence_boundary,
         * but tries to keep leading/trailing whitespace out of sentences for
         * the start/end flags
         */

        /* The Unicode spec seems to say that one trailing line/para
         * separator can be tacked on to a sentence ending in ! or ?,
         * but not a sentence ending in period; I think they're on crack
         * so am allowing one to be tacked onto a sentence ending in period.
         */

#define MAYBE_START_NEW_SENTENCE                                \
        switch ((int) type)                               \
        {                                               \
            case G_UNICODE_LINE_SEPARATOR:                  \
            case G_UNICODE_PARAGRAPH_SEPARATOR:             \
            case G_UNICODE_CONTROL:                         \
            case G_UNICODE_FORMAT:                          \
            case G_UNICODE_SPACE_SEPARATOR:                 \
                                                            sentence_state = STATE_SENTENCE_OUTSIDE;      \
            break;                                        \
            \
            default:                                        \
                                                            sentence_state = STATE_SENTENCE_BODY;         \
            attrs[i].is_sentence_start = TRUE;            \
            break;                                        \
        }

        /* No sentence break at the start of the text */

        /* default to not a sentence breakpoint */
        attrs[i].is_sentence_boundary = FALSE;
        attrs[i].is_sentence_start = FALSE;
        attrs[i].is_sentence_end = FALSE;

        /* FIXME the Unicode spec lumps control/format chars with
         * line/para separators in descriptive text, but not in the
         * character class specs, in table 5-6, so who knows whether you
         * are actually supposed to break on control/format
         * characters. Seems semi-broken to break on tabs...
         */

        /* Break after line/para separators except carriage return
         * followed by newline
         */
        switch ((int) prev_type)
        {
            case G_UNICODE_LINE_SEPARATOR:
            case G_UNICODE_PARAGRAPH_SEPARATOR:
            case G_UNICODE_CONTROL:
            case G_UNICODE_FORMAT:
                if (wc == '\r')
                {
                    if (next_wc != '\n')
                        attrs[i].is_sentence_boundary = TRUE;
                }
                else
                    attrs[i].is_sentence_boundary = TRUE;
                break;

            default:
                break;
        }

        /* break before para/line separators except newline following
         * carriage return
         */
        switch ((int) type)
        {
            case G_UNICODE_LINE_SEPARATOR:
            case G_UNICODE_PARAGRAPH_SEPARATOR:
            case G_UNICODE_CONTROL:
            case G_UNICODE_FORMAT:
                if (wc == '\n')
                {
                    if (prev_wc != '\r')
                        attrs[i].is_sentence_boundary = TRUE;
                }
                else
                    attrs[i].is_sentence_boundary = TRUE;
                break;

            default:
                break;
        }

        switch (sentence_state)
        {
            case STATE_SENTENCE_OUTSIDE:
                /* Start sentence if we have non-whitespace/format/control */
                switch ((int) type)
                {
                    case G_UNICODE_LINE_SEPARATOR:
                    case G_UNICODE_PARAGRAPH_SEPARATOR:
                    case G_UNICODE_CONTROL:
                    case G_UNICODE_FORMAT:
                    case G_UNICODE_SPACE_SEPARATOR:
                        break;

                    default:
                        attrs[i].is_sentence_start = TRUE;
                        sentence_state = STATE_SENTENCE_BODY;
                        break;
                }
                break;

            case STATE_SENTENCE_BODY:
                /* If we already broke here due to separators, end the sentence. */
                if (attrs[i].is_sentence_boundary)
                {
                    attrs[i].is_sentence_end = TRUE;

                    MAYBE_START_NEW_SENTENCE;
                }
                else
                {
                    if (wc == '.')
                        sentence_state = STATE_SENTENCE_DOT;
                    else if (wc == '?' || wc == '!')
                        sentence_state = STATE_SENTENCE_TERM;
                }
                break;

            case STATE_SENTENCE_TERM:
                /* End sentence on anything but close punctuation and some
                 * loosely-specified OTHER_PUNCTUATION such as period,
                 * comma, etc.; follow Unicode rules for breaks
                 */
                switch ((int) type)
                {
                    case G_UNICODE_OTHER_PUNCTUATION:
                    case G_UNICODE_CLOSE_PUNCTUATION:
                        if (type == G_UNICODE_CLOSE_PUNCTUATION ||
                                wc == '.' ||
                                wc == ',' ||
                                wc == '?' ||
                                wc == '!')
                            sentence_state = STATE_SENTENCE_POST_TERM_CLOSE;
                        else
                        {
                            attrs[i].is_sentence_end = TRUE;
                            attrs[i].is_sentence_boundary = TRUE;

                            MAYBE_START_NEW_SENTENCE;
                        }
                        break;

                    case G_UNICODE_SPACE_SEPARATOR:
                        attrs[i].is_sentence_end = TRUE;
                        sentence_state = STATE_SENTENCE_POST_TERM_SPACE;
                        break;

                    case G_UNICODE_LINE_SEPARATOR:
                    case G_UNICODE_PARAGRAPH_SEPARATOR:
                        attrs[i].is_sentence_end = TRUE;
                        sentence_state = STATE_SENTENCE_POST_TERM_SEP;
                        break;

                    default:
                        attrs[i].is_sentence_end = TRUE;
                        attrs[i].is_sentence_boundary = TRUE;

                        MAYBE_START_NEW_SENTENCE;

                        break;
                }
                break;

            case STATE_SENTENCE_POST_TERM_CLOSE:
                /* End sentence on anything besides more punctuation; follow
                 * rules for breaks
                 */
                switch ((int) type)
                {
                    case G_UNICODE_OTHER_PUNCTUATION:
                    case G_UNICODE_CLOSE_PUNCTUATION:
                        if (type == G_UNICODE_CLOSE_PUNCTUATION ||
                                wc == '.' ||
                                wc == ',' ||
                                wc == '?' ||
                                wc == '!')
                            /* continue in this state */
                            ;
                        else
                        {
                            attrs[i].is_sentence_end = TRUE;
                            attrs[i].is_sentence_boundary = TRUE;

                            MAYBE_START_NEW_SENTENCE;
                        }
                        break;

                    case G_UNICODE_SPACE_SEPARATOR:
                        attrs[i].is_sentence_end = TRUE;
                        sentence_state = STATE_SENTENCE_POST_TERM_SPACE;
                        break;

                    case G_UNICODE_LINE_SEPARATOR:
                    case G_UNICODE_PARAGRAPH_SEPARATOR:
                        attrs[i].is_sentence_end = TRUE;
                        /* undo the unconditional break-at-all-line/para-separators
                         * from above; I'm not sure this is what the Unicode spec
                         * intends, but it seems right - we get to include
                         * a single line/para separator in the sentence according
                         * to their rules
                         */
                        attrs[i].is_sentence_boundary = FALSE;
                        sentence_state = STATE_SENTENCE_POST_TERM_SEP;
                        break;

                    default:
                        attrs[i].is_sentence_end = TRUE;
                        attrs[i].is_sentence_boundary = TRUE;

                        MAYBE_START_NEW_SENTENCE;

                        break;
                }
                break;

            case STATE_SENTENCE_POST_TERM_SPACE:

                /* Sentence is definitely already ended; to enter this state
                 * we had to see a space, which ends the sentence.
                 */

                switch ((int) type)
                {
                    case G_UNICODE_SPACE_SEPARATOR:
                        /* continue in this state */
                        break;

                    case G_UNICODE_LINE_SEPARATOR:
                    case G_UNICODE_PARAGRAPH_SEPARATOR:
                        /* undo the unconditional break-at-all-line/para-separators
                         * from above; I'm not sure this is what the Unicode spec
                         * intends, but it seems right
                         */
                        attrs[i].is_sentence_boundary = FALSE;
                        sentence_state = STATE_SENTENCE_POST_TERM_SEP;
                        break;

                    default:
                        attrs[i].is_sentence_boundary = TRUE;

                        MAYBE_START_NEW_SENTENCE;

                        break;
                }
                break;

            case STATE_SENTENCE_POST_TERM_SEP:
                /* Break is forced at this point, unless we're a newline
                 * after a CR, then we will break after the newline on the
                 * next iteration. Only a single Sep can be in the
                 * sentence.
                 */
                if (!(prev_wc == '\r' && wc == '\n'))
                    attrs[i].is_sentence_boundary = TRUE;

                MAYBE_START_NEW_SENTENCE;

                break;

            case STATE_SENTENCE_DOT:
                switch ((int) type)
                {
                    case G_UNICODE_CLOSE_PUNCTUATION:
                        sentence_state = STATE_SENTENCE_POST_DOT_CLOSE;
                        break;

                    case G_UNICODE_SPACE_SEPARATOR:
                        possible_sentence_end = i;
                        sentence_state = STATE_SENTENCE_POST_DOT_SPACE;
                        break;

                    default:
                        /* If we broke on a control/format char, end the
                         * sentence; else this was not a sentence end, since
                         * we didn't enter the POST_DOT_SPACE state.
                         */
                        if (attrs[i].is_sentence_boundary)
                        {
                            attrs[i].is_sentence_end = TRUE;

                            MAYBE_START_NEW_SENTENCE;
                        }
                        else
                            sentence_state = STATE_SENTENCE_BODY;
                        break;
                }
                break;

            case STATE_SENTENCE_POST_DOT_CLOSE:
                switch ((int) type)
                {
                    case G_UNICODE_SPACE_SEPARATOR:
                        possible_sentence_end = i;
                        sentence_state = STATE_SENTENCE_POST_DOT_SPACE;
                        break;

                    default:
                        /* If we broke on a control/format char, end the
                         * sentence; else this was not a sentence end, since
                         * we didn't enter the POST_DOT_SPACE state.
                         */
                        if (attrs[i].is_sentence_boundary)
                        {
                            attrs[i].is_sentence_end = TRUE;

                            MAYBE_START_NEW_SENTENCE;
                        }
                        else
                            sentence_state = STATE_SENTENCE_BODY;
                        break;
                }
                break;

            case STATE_SENTENCE_POST_DOT_SPACE:

                possible_sentence_boundary = i;

                switch ((int) type)
                {
                    case G_UNICODE_SPACE_SEPARATOR:
                        /* remain in current state */
                        break;

                    case G_UNICODE_OPEN_PUNCTUATION:
                        sentence_state = STATE_SENTENCE_POST_DOT_OPEN;
                        break;

                    case G_UNICODE_LOWERCASE_LETTER:
                        /* wasn't a sentence-ending period; so re-enter the sentence
                         * body
                         */
                        sentence_state = STATE_SENTENCE_BODY;
                        break;

                    default:
                        /* End the sentence, break, maybe start a new one */

                        g_assert (possible_sentence_end >= 0);
                        g_assert (possible_sentence_boundary >= 0);

                        attrs[possible_sentence_boundary].is_sentence_boundary = TRUE;
                        attrs[possible_sentence_end].is_sentence_end = TRUE;

                        possible_sentence_end = -1;
                        possible_sentence_boundary = -1;

                        MAYBE_START_NEW_SENTENCE;

                        break;
                }
                break;

            case STATE_SENTENCE_POST_DOT_OPEN:
                switch ((int) type)
                {
                    case G_UNICODE_OPEN_PUNCTUATION:
                        /* continue in current state */
                        break;

                    case G_UNICODE_LOWERCASE_LETTER:
                        /* wasn't a sentence-ending period; so re-enter the sentence
                         * body
                         */
                        sentence_state = STATE_SENTENCE_BODY;
                        break;

                    default:
                        /* End the sentence, break, maybe start a new one */

                        g_assert (possible_sentence_end >= 0);
                        g_assert (possible_sentence_boundary >= 0);

                        attrs[possible_sentence_boundary].is_sentence_boundary = TRUE;
                        attrs[possible_sentence_end].is_sentence_end = TRUE;

                        possible_sentence_end = -1;
                        possible_sentence_boundary = -1;

                        MAYBE_START_NEW_SENTENCE;

                        break;
                }
                break;

            case STATE_SENTENCE_POST_DOT_SEP:
                /* Break is forced at this point, unless we're a newline
                 * after a CR, then we will break after the newline on the
                 * next iteration. Only a single Sep can be in the
                 * sentence.
                 */
                if (!(prev_wc == '\r' && wc == '\n'))
                    attrs[i].is_sentence_boundary = TRUE;

                g_assert (possible_sentence_end >= 0);
                g_assert (possible_sentence_boundary >= 0);

                attrs[possible_sentence_end].is_sentence_end = TRUE;

                possible_sentence_end = -1;
                possible_sentence_boundary = -1;

                MAYBE_START_NEW_SENTENCE;

                break;

            default:
                g_assert_not_reached ();
                break;
        }

        prev_type = type;
        prev_wc = wc;

        /* wc might not be a valid Unicode base character, but really all we
         * need to know is the last non-combining character */
        if (type != G_UNICODE_SPACING_MARK &&
                type != G_UNICODE_ENCLOSING_MARK &&
                type != G_UNICODE_NON_SPACING_MARK)
            base_character = wc;
    }
    i--;

    attrs[i].is_cursor_position = TRUE;  /* Rule GB2 */
    attrs[0].is_cursor_position = TRUE;  /* Rule GB1 */

    attrs[i].is_word_boundary = TRUE;  /* Rule WB2 */
    attrs[0].is_word_boundary = TRUE;  /* Rule WB1 */

    attrs[i].is_line_break = TRUE;  /* Rule LB3 */
    attrs[0].is_line_break = FALSE; /* Rule LB2 */

}

Vector<String> FilterBase::splitUTF8(const char* source, const char* sourceEnd)
{
    Vector<String> result;
    for (int sourceOffset = 0; sourceOffset < sourceEnd - source; ) {
        int begin = sourceOffset;

        UChar32 character;
        U8_NEXT(reinterpret_cast<const uint8_t*>(source), sourceOffset, sourceEnd - source, character);
        if (character < 0)
            break;
        result.append(String(source + begin, sourceOffset - begin));
    }
    return result;
}

#if 0
Vector<Row> FilterBase::doFilter(Vector<Row> rowVec, String param)
{
    return rowVec;
}
#endif

UCharBreaker::UCharBreaker(const char* text)
 : m_text(text)
 , m_uchar(NULL)
 , m_ucharLen(0)
 , m_breakAttrs(NULL)
 , m_breakAttrsCount(0)
{
}

UCharBreaker::~UCharBreaker()
{
    if (m_uchar)
        g_free(m_uchar);

    if (m_breakAttrs) {
        free(m_breakAttrs);
    }
}

void UCharBreaker::doUStrGetBreaks()
{
    if (!m_text)
        return;

    int textLen = strlen(m_text);
    if (textLen <= 0)
        return;

    m_breakAttrsCount = textLen;
    m_breakAttrs = (struct UCharBreakAttr*) calloc(m_breakAttrsCount,
            sizeof(struct UCharBreakAttr));
    m_uchar = g_utf8_to_ucs4_fast(m_text, -1, &m_ucharLen);
    if(!m_uchar || m_ucharLen <=0)
        return;

    genBreaks(m_text, textLen, m_breakAttrs);
}


} // namespace PurCFetcher

IGNORE_WARNINGS_END
