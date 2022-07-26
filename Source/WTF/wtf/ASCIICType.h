/*
 * Copyright (C) 2007-2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/Assertions.h>
#include <wtf/text/LChar.h>

// The behavior of many of the functions in the <ctype.h> header is dependent
// on the current locale. But in the xGUI project, all uses of those functions
// are in code processing something that's not locale-specific. These equivalents
// for some of the <ctype.h> functions are named more explicitly, not dependent
// on the C library locale, and we should also optimize them as needed.

// All functions return false or leave the character unchanged if passed a character
// that is outside the range 0-7F. So they can be used on Unicode strings or
// characters if the intent is to do processing only if the character is ASCII.

namespace PurCWTF {

template<typename CharacterType> constexpr bool isASCII(CharacterType);
template<typename CharacterType> bool isASCIIAlpha(CharacterType);
template<typename CharacterType> bool isASCIIAlphanumeric(CharacterType);
template<typename CharacterType> bool isASCIIBinaryDigit(CharacterType);
template<typename CharacterType> bool isASCIIDigit(CharacterType);
template<typename CharacterType> bool isASCIIHexDigit(CharacterType);
template<typename CharacterType> bool isASCIILower(CharacterType);
template<typename CharacterType> bool isASCIIOctalDigit(CharacterType);
template<typename CharacterType> bool isASCIIPrintable(CharacterType);
template<typename CharacterType> bool isASCIISpace(CharacterType);
template<typename CharacterType> bool isASCIIUpper(CharacterType);

template<typename CharacterType> CharacterType toASCIILower(CharacterType);
template<typename CharacterType> CharacterType toASCIIUpper(CharacterType);

template<typename CharacterType> uint8_t toASCIIHexValue(CharacterType);
template<typename CharacterType> uint8_t toASCIIHexValue(CharacterType firstCharacter, CharacterType secondCharacter);

char lowerNibbleToASCIIHexDigit(uint8_t);
char upperNibbleToASCIIHexDigit(uint8_t);
char lowerNibbleToLowercaseASCIIHexDigit(uint8_t);
char upperNibbleToLowercaseASCIIHexDigit(uint8_t);

template<typename CharacterType> bool isASCIIAlphaCaselessEqual(CharacterType, char expectedASCIILowercaseLetter);

// The toASCIILowerUnchecked function can be used for comparing any input character
// to a lowercase English character. The isASCIIAlphaCaselessEqual function should
// be used for regular comparison of ASCII alpha characters, but switch statements
// in the CSS tokenizer, for example, instead make direct use toASCIILowerUnchecked.
template<typename CharacterType> CharacterType toASCIILowerUnchecked(CharacterType);

extern WTF_EXPORT_PRIVATE const unsigned char asciiCaseFoldTable[256];

template<typename CharacterType> constexpr bool isASCII(CharacterType character)
{
    return !(character & ~0x7F);
}

template<typename CharacterType> inline bool isASCIILower(CharacterType character)
{
    return character >= 'a' && character <= 'z';
}

template<typename CharacterType> inline CharacterType toASCIILowerUnchecked(CharacterType character)
{
    // This function can be used for comparing any input character
    // to a lowercase English character. The isASCIIAlphaCaselessEqual
    // below should be used for regular comparison of ASCII alpha
    // characters, but switch statements in CSS tokenizer instead make
    // direct use of this function.
    return character | 0x20;
}

template<typename CharacterType> inline bool isASCIIAlpha(CharacterType character)
{
    return isASCIILower(toASCIILowerUnchecked(character));
}

template<typename CharacterType> inline bool isASCIIDigit(CharacterType character)
{
    return character >= '0' && character <= '9';
}

template<typename CharacterType> inline bool isASCIIAlphanumeric(CharacterType character)
{
    return isASCIIDigit(character) || isASCIIAlpha(character);
}

template<typename CharacterType> inline bool isASCIIHexDigit(CharacterType character)
{
    return isASCIIDigit(character) || (toASCIILowerUnchecked(character) >= 'a' && toASCIILowerUnchecked(character) <= 'f');
}

template<typename CharacterType> inline bool isASCIIBinaryDigit(CharacterType character)
{
    return character == '0' || character == '1';
}

template<typename CharacterType> inline bool isASCIIOctalDigit(CharacterType character)
{
    return character >= '0' && character <= '7';
}

template<typename CharacterType> inline bool isASCIIPrintable(CharacterType character)
{
    return character >= ' ' && character <= '~';
}

/*
    Statistics from a run of Apple's page load test for callers of isASCIISpace:

    character          count
    ---------          -----
    non-spaces         689383
    20  space          294720
    0A  \n             89059
    09  \t             28320
    0D  \r             0
    0C  \f             0
    0B  \v             0

    Because of those, we first check to quickly return false for non-control characters,
    then check for space itself to quickly return true for that case, then do the rest.
*/
template<typename CharacterType> inline bool isASCIISpace(CharacterType character)
{
    return character <= ' ' && (character == ' ' || (character <= 0xD && character >= 0x9));
}

template<typename CharacterType> inline bool isASCIIUpper(CharacterType character)
{
    return character >= 'A' && character <= 'Z';
}

template<typename CharacterType> inline CharacterType toASCIILower(CharacterType character)
{
    return character | (isASCIIUpper(character) << 5);
}

template<> inline char toASCIILower(char character)
{
    return static_cast<char>(asciiCaseFoldTable[static_cast<uint8_t>(character)]);
}

template<> inline LChar toASCIILower(LChar character)
{
    return asciiCaseFoldTable[character];
}

template<typename CharacterType> inline CharacterType toASCIIUpper(CharacterType character)
{
    return character & ~(isASCIILower(character) << 5);
}

template<typename CharacterType> inline uint8_t toASCIIHexValue(CharacterType character)
{
    ASSERT(isASCIIHexDigit(character));
    return character < 'A' ? character - '0' : (character - 'A' + 10) & 0xF;
}

template<typename CharacterType> inline uint8_t toASCIIHexValue(CharacterType firstCharacter, CharacterType secondCharacter)
{
    return toASCIIHexValue(firstCharacter) << 4 | toASCIIHexValue(secondCharacter);
}

inline char lowerNibbleToASCIIHexDigit(uint8_t value)
{
    uint8_t nibble = value & 0xF;
    return nibble + (nibble < 10 ? '0' : 'A' - 10);
}

inline char upperNibbleToASCIIHexDigit(uint8_t value)
{
    uint8_t nibble = value >> 4;
    return nibble + (nibble < 10 ? '0' : 'A' - 10);
}

inline char lowerNibbleToLowercaseASCIIHexDigit(uint8_t value)
{
    uint8_t nibble = value & 0xF;
    return nibble + (nibble < 10 ? '0' : 'a' - 10);
}

inline char upperNibbleToLowercaseASCIIHexDigit(uint8_t value)
{
    uint8_t nibble = value >> 4;
    return nibble + (nibble < 10 ? '0' : 'a' - 10);
}

template<typename CharacterType> inline bool isASCIIAlphaCaselessEqual(CharacterType inputCharacter, char expectedASCIILowercaseLetter)
{
    // Name of this argument says this must be a lowercase letter, but it can actually be:
    //   - a lowercase letter
    //   - a numeric digit
    //   - a space
    //   - punctuation in the range 0x21-0x3F, including "-", "/", and "+"
    // It cannot be:
    //   - an uppercase letter
    //   - a non-ASCII character
    //   - other punctuation, such as underscore and backslash
    //   - a control character such as "\n"
    // FIXME: Would be nice to make both the function name and expectedASCIILowercaseLetter argument name clearer.
    ASSERT(toASCIILowerUnchecked(expectedASCIILowercaseLetter) == expectedASCIILowercaseLetter);
    return LIKELY(toASCIILowerUnchecked(inputCharacter) == expectedASCIILowercaseLetter);
}

template<typename CharacterType> inline bool isASCIIDigitOrPunctuation(CharacterType charCode)
{
    return (charCode >= '!' && charCode <= '@') || (charCode >= '[' && charCode <= '`') || (charCode >= '{' && charCode <= '~');
}

}

using PurCWTF::isASCII;
using PurCWTF::isASCIIAlpha;
using PurCWTF::isASCIIAlphaCaselessEqual;
using PurCWTF::isASCIIAlphanumeric;
using PurCWTF::isASCIIBinaryDigit;
using PurCWTF::isASCIIDigit;
using PurCWTF::isASCIIDigitOrPunctuation;
using PurCWTF::isASCIIHexDigit;
using PurCWTF::isASCIILower;
using PurCWTF::isASCIIOctalDigit;
using PurCWTF::isASCIIPrintable;
using PurCWTF::isASCIISpace;
using PurCWTF::isASCIIUpper;
using PurCWTF::lowerNibbleToASCIIHexDigit;
using PurCWTF::lowerNibbleToLowercaseASCIIHexDigit;
using PurCWTF::toASCIIHexValue;
using PurCWTF::toASCIILower;
using PurCWTF::toASCIILowerUnchecked;
using PurCWTF::toASCIIUpper;
using PurCWTF::upperNibbleToASCIIHexDigit;
using PurCWTF::upperNibbleToLowercaseASCIIHexDigit;
