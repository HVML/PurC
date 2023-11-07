/*
** @file unicode-emoj.c
** @author Vincent Wei
** @date 2022/10/22
** @brief Some operators to check Emoji.
**  Note that we copied most of code from GPL'd MiniGUI:
**      <https://github.com/VincentWei/MiniGUI/>
**
**  The implementation is based on some code from HarfBuzz(licensed under MIT):
**      <https://github.com/harfbuzz/harfbuzz>
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

#include "foil.h"
#include "unicode.h"

#include <stdlib.h> /* for bsearch */
#include <assert.h>

#include "unicode-emoji-tables.h"

static int interval_compare(const void *key, const void *elt)
{
    uint32_t c = (uint32_t)(intptr_t)key;
    struct Interval *interval = (struct Interval *)elt;

    if (c < interval->start)
        return -1;
    if (c > interval->end)
        return +1;

    return 0;
}

#define DEFINE_unicode_is_(name) \
    bool foil_uchar_is_##name(uint32_t ch) \
{ \
    if (bsearch((void*)((intptr_t)ch), \
                _unicode_##name##_table, \
                PCA_TABLESIZE(_unicode_##name##_table), \
                sizeof _unicode_##name##_table[0], \
                interval_compare)) \
        return true; \
    \
    return false; \
} \
\
static UNUSED_FUNCTION inline bool _unicode_is_##name(uint32_t ch) \
{  \
    return foil_uchar_is_##name(ch);\
}

DEFINE_unicode_is_(emoji)
DEFINE_unicode_is_(emoji_presentation)
DEFINE_unicode_is_(emoji_modifier)
DEFINE_unicode_is_(emoji_modifier_base)
DEFINE_unicode_is_(extended_pictographic)

static bool _unicode_is_emoji_text_default(uint32_t ch)
{
    return foil_uchar_is_emoji(ch) && !foil_uchar_is_emoji_presentation(ch);
}

static bool _unicode_is_emoji_emoji_default(uint32_t ch)
{
    return foil_uchar_is_emoji_presentation(ch);
}

static bool _unicode_is_emoji_keycap_base(uint32_t ch)
{
    return (ch >= '0' && ch <= '9') || ch == '#' || ch == '*';
}

static bool _unicode_is_regional_indicator(uint32_t ch)
{
    return (ch >= 0x1F1E6 && ch <= 0x1F1FF);
}

/*
 * Implementation of foil_emoji_iterator is based on Chromium's Ragel-based
 * parser:
 *
 * https://chromium-review.googlesource.com/c/chromium/src/+/1264577
 *
 * The grammar file emoji_presentation_scanner.rl was just modified to
 * adapt the function signature and variables to our usecase.  The
 * grammar itself was NOT modified:
 *
 * https://chromium-review.googlesource.com/c/chromium/src/+/1264577/3/third_party/blink/renderer/platform/fonts/emoji_presentation_scanner.rl
 *
 * The emoji_presentation_scanner.c is generated from .rl file by
 * running ragel on it.
 *
 * The categorization is also based on:
 *
 * https://chromium-review.googlesource.com/c/chromium/src/+/1264577/3/third_party/blink/renderer/platform/fonts/utf16_ragel_iterator.h
 *
 * The iterator next() is based on:
 *
 * https://chromium-review.googlesource.com/c/chromium/src/+/1264577/3/third_party/blink/renderer/platform/fonts/symbols_iterator.cc
 *
 * // Copyright 2015 The Chromium Authors. All rights reserved.
 * // Use of this source code is governed by a BSD-style license that can be
 * // found in the LICENSE file.
 */

const uint32_t kCombiningEnclosingCircleBackslashCharacter = 0x20E0;
const uint32_t kCombiningEnclosingKeycapCharacter = 0x20E3;
const uint32_t kVariationSelector15Character = 0xFE0E;
const uint32_t kVariationSelector16Character = 0xFE0F;
const uint32_t kZeroWidthJoinerCharacter = 0x200D;

enum EmojiScannerCategory {
    EMOJI = 0,
    EMOJI_TEXT_PRESENTATION = 1,
    EMOJI_EMOJI_PRESENTATION = 2,
    EMOJI_MODIFIER_BASE = 3,
    EMOJI_MODIFIER = 4,
    EMOJI_VS_BASE = 5,
    REGIONAL_INDICATOR = 6,
    KEYCAP_BASE = 7,
    COMBINING_ENCLOSING_KEYCAP = 8,
    COMBINING_ENCLOSING_CIRCLE_BACKSLASH = 9,
    ZWJ = 10,
    VS15 = 11,
    VS16 = 12,
    TAG_BASE = 13,
    TAG_SEQUENCE = 14,
    TAG_TERM = 15,
    kMaxEmojiScannerCategory = 16
};

static unsigned char emojiSegmentationCategory (uint32_t codepoint)
{
    /* Specific ones first. */
    if (codepoint == kCombiningEnclosingKeycapCharacter)
        return COMBINING_ENCLOSING_KEYCAP;
    if (codepoint == kCombiningEnclosingCircleBackslashCharacter)
        return COMBINING_ENCLOSING_CIRCLE_BACKSLASH;
    if (codepoint == kZeroWidthJoinerCharacter)
        return ZWJ;
    if (codepoint == kVariationSelector15Character)
        return VS15;
    if (codepoint == kVariationSelector16Character)
        return VS16;
    if (codepoint == 0x1F3F4)
        return TAG_BASE;
    if ((codepoint >= 0xE0030 && codepoint <= 0xE0039) ||
            (codepoint >= 0xE0061 && codepoint <= 0xE007A))
        return TAG_SEQUENCE;
    if (codepoint == 0xE007F)
        return TAG_TERM;

    if (_unicode_is_emoji_modifier_base (codepoint))
        return EMOJI_MODIFIER_BASE;
    if (_unicode_is_emoji_modifier (codepoint))
        return EMOJI_MODIFIER;
    if (_unicode_is_regional_indicator (codepoint))
        return REGIONAL_INDICATOR;
    if (_unicode_is_emoji_keycap_base (codepoint))
        return KEYCAP_BASE;

    if (_unicode_is_emoji_emoji_default (codepoint))
        return EMOJI_EMOJI_PRESENTATION;
    if (_unicode_is_emoji_text_default (codepoint))
        return EMOJI_TEXT_PRESENTATION;
    if (_unicode_is_emoji (codepoint))
        return EMOJI;

    /* Ragel state machine will interpret unknown category as "any". */
    return kMaxEmojiScannerCategory;
}


typedef unsigned char *emoji_text_iter_t;

#include "emoji_presentation_scanner.inc"

foil_emoji_iterator * foil_emoji_iterator_init (foil_emoji_iterator *iter,
        const uint32_t* ucs, int nr_ucs, uint8_t* types_buff)
{
    int i;
    uint8_t *types = types_buff;
    const uint32_t *p;

    assert (nr_ucs > 0);

    p = ucs;
    for (i = 0; i < nr_ucs; i++) {
        types[i] = emojiSegmentationCategory (*p);
        p++;
    }

    iter->text_start = iter->start = iter->end = ucs;
    iter->text_end = ucs + nr_ucs;
    iter->is_emoji = false;

    iter->types = types;
    iter->n_chars = nr_ucs;
    iter->cursor = 0;

    foil_emoji_iterator_next (iter);
    return iter;
}

bool foil_emoji_iterator_next (foil_emoji_iterator *iter)
{
    unsigned int old_cursor, cursor;
    bool is_emoji;

    if (iter->end >= iter->text_end)
        return false;

    iter->start = iter->end;

    old_cursor = cursor = iter->cursor;
    cursor = scan_emoji_presentation (iter->types + cursor,
            iter->types + iter->n_chars,
            &is_emoji) - iter->types;

    do {
        iter->cursor = cursor;
        iter->is_emoji = is_emoji;

        if (cursor == iter->n_chars)
            break;

        cursor = scan_emoji_presentation (iter->types + cursor,
                iter->types + iter->n_chars,
                &is_emoji) - iter->types;
    }
    while (iter->is_emoji == is_emoji);

    iter->end = iter->start + (iter->cursor - old_cursor);
    return true;
}
