/*
 * Copyright (C) 2007, 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(ICU)
#include <unicode/utypes.h>
#endif
#include <wtf/text/UChar.h>

namespace PurCWTF {
namespace Unicode {

// Names here are taken from the Unicode standard.

// Most of these are UChar constants, not UChar32, which makes them
// more convenient for xCore code that mostly uses UTF-16.

const UChar AppleLogo = 0xF8FF;
const UChar HiraganaLetterSmallA = 0x3041;
const UChar32 aegeanWordSeparatorDot = 0x10101;
const UChar32 aegeanWordSeparatorLine = 0x10100;
const UChar apostrophe = 0x0027;
const UChar blackCircle = 0x25CF;
const UChar blackSquare = 0x25A0;
const UChar blackUpPointingTriangle = 0x25B2;
const UChar bullet = 0x2022;
const UChar bullseye = 0x25CE;
const UChar carriageReturn = 0x000D;
const UChar combiningEnclosingKeycap = 0x20E3;
const UChar ethiopicPrefaceColon = 0x1366;
const UChar ethiopicWordspace = 0x1361;
const UChar firstStrongIsolate = 0x2068;
const UChar fisheye = 0x25C9;
const UChar hebrewPunctuationGeresh = 0x05F3;
const UChar hebrewPunctuationGershayim = 0x05F4;
const UChar horizontalEllipsis = 0x2026;
const UChar hyphen = 0x2010;
const UChar hyphenMinus = 0x002D;
const UChar ideographicComma = 0x3001;
const UChar ideographicFullStop = 0x3002;
const UChar ideographicSpace = 0x3000;
const UChar leftDoubleQuotationMark = 0x201C;
const UChar leftLowDoubleQuotationMark = 0x201E;
const UChar leftSingleQuotationMark = 0x2018;
const UChar leftLowSingleQuotationMark = 0x201A;
const UChar leftToRightEmbed = 0x202A;
const UChar leftToRightIsolate = 0x2066;
const UChar leftToRightMark = 0x200E;
const UChar leftToRightOverride = 0x202D;
const UChar minusSign = 0x2212;
const UChar narrowNoBreakSpace = 0x202F;
const UChar narrowNonBreakingSpace = 0x202F;
const UChar newlineCharacter = 0x000A;
const UChar noBreakSpace = 0x00A0;
const UChar objectReplacementCharacter = 0xFFFC;
const UChar optionKey = 0x2325;
const UChar popDirectionalFormatting = 0x202C;
const UChar popDirectionalIsolate = 0x2069;
const UChar quotationMark = 0x0022;
const UChar replacementCharacter = 0xFFFD;
const UChar rightDoubleQuotationMark = 0x201D;
const UChar rightSingleQuotationMark = 0x2019;
const UChar rightToLeftEmbed = 0x202B;
const UChar rightToLeftIsolate = 0x2067;
const UChar rightToLeftMark = 0x200F;
const UChar rightToLeftOverride = 0x202E;
const UChar sesameDot = 0xFE45;
const UChar smallLetterSharpS = 0x00DF;
const UChar softHyphen = 0x00AD;
const UChar space = 0x0020;
const UChar tabCharacter = 0x0009;
const UChar tibetanMarkDelimiterTshegBstar = 0x0F0C;
const UChar tibetanMarkIntersyllabicTsheg = 0x0F0B;
const UChar32 ugariticWordDivider = 0x1039F;
const UChar upArrowhead = 0x2303;
const UChar whiteBullet = 0x25E6;
const UChar whiteCircle = 0x25CB;
const UChar whiteSesameDot = 0xFE46;
const UChar whiteUpPointingTriangle = 0x25B3;
const UChar wordJoiner = 0x2060;
const UChar yenSign = 0x00A5;
const UChar zeroWidthJoiner = 0x200D;
const UChar zeroWidthNoBreakSpace = 0xFEFF;
const UChar zeroWidthNonJoiner = 0x200C;
const UChar zeroWidthSpace = 0x200B;

} // namespace Unicode
} // namespace PurCWTF

using PurCWTF::Unicode::AppleLogo;
using PurCWTF::Unicode::HiraganaLetterSmallA;
using PurCWTF::Unicode::aegeanWordSeparatorDot;
using PurCWTF::Unicode::aegeanWordSeparatorLine;
using PurCWTF::Unicode::blackCircle;
using PurCWTF::Unicode::blackSquare;
using PurCWTF::Unicode::blackUpPointingTriangle;
using PurCWTF::Unicode::bullet;
using PurCWTF::Unicode::bullseye;
using PurCWTF::Unicode::carriageReturn;
using PurCWTF::Unicode::combiningEnclosingKeycap;
using PurCWTF::Unicode::ethiopicPrefaceColon;
using PurCWTF::Unicode::ethiopicWordspace;
using PurCWTF::Unicode::firstStrongIsolate;
using PurCWTF::Unicode::fisheye;
using PurCWTF::Unicode::hebrewPunctuationGeresh;
using PurCWTF::Unicode::hebrewPunctuationGershayim;
using PurCWTF::Unicode::horizontalEllipsis;
using PurCWTF::Unicode::hyphen;
using PurCWTF::Unicode::hyphenMinus;
using PurCWTF::Unicode::ideographicComma;
using PurCWTF::Unicode::ideographicFullStop;
using PurCWTF::Unicode::ideographicSpace;
using PurCWTF::Unicode::leftDoubleQuotationMark;
using PurCWTF::Unicode::leftLowDoubleQuotationMark;
using PurCWTF::Unicode::leftSingleQuotationMark;
using PurCWTF::Unicode::leftLowSingleQuotationMark;
using PurCWTF::Unicode::leftToRightEmbed;
using PurCWTF::Unicode::leftToRightIsolate;
using PurCWTF::Unicode::leftToRightMark;
using PurCWTF::Unicode::leftToRightOverride;
using PurCWTF::Unicode::minusSign;
using PurCWTF::Unicode::narrowNoBreakSpace;
using PurCWTF::Unicode::narrowNonBreakingSpace;
using PurCWTF::Unicode::newlineCharacter;
using PurCWTF::Unicode::noBreakSpace;
using PurCWTF::Unicode::objectReplacementCharacter;
using PurCWTF::Unicode::popDirectionalFormatting;
using PurCWTF::Unicode::popDirectionalIsolate;
using PurCWTF::Unicode::replacementCharacter;
using PurCWTF::Unicode::rightDoubleQuotationMark;
using PurCWTF::Unicode::rightSingleQuotationMark;
using PurCWTF::Unicode::rightToLeftEmbed;
using PurCWTF::Unicode::rightToLeftIsolate;
using PurCWTF::Unicode::rightToLeftMark;
using PurCWTF::Unicode::rightToLeftOverride;
using PurCWTF::Unicode::sesameDot;
using PurCWTF::Unicode::softHyphen;
using PurCWTF::Unicode::space;
using PurCWTF::Unicode::tabCharacter;
using PurCWTF::Unicode::tibetanMarkDelimiterTshegBstar;
using PurCWTF::Unicode::tibetanMarkIntersyllabicTsheg;
using PurCWTF::Unicode::ugariticWordDivider;
using PurCWTF::Unicode::upArrowhead;
using PurCWTF::Unicode::whiteBullet;
using PurCWTF::Unicode::whiteCircle;
using PurCWTF::Unicode::whiteSesameDot;
using PurCWTF::Unicode::whiteUpPointingTriangle;
using PurCWTF::Unicode::wordJoiner;
using PurCWTF::Unicode::yenSign;
using PurCWTF::Unicode::zeroWidthJoiner;
using PurCWTF::Unicode::zeroWidthNoBreakSpace;
using PurCWTF::Unicode::zeroWidthNonJoiner;
using PurCWTF::Unicode::zeroWidthSpace;
