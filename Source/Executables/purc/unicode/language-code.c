/*
** @file language-code.c
** @author Vincent Wei
** @date 2022/11/09
** @brief Implemetation of foil_langcode_from_iso639_1() and
**  foil_langcode_to_iso639_1().
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
#include "foil.h"
#include "unicode.h"

static const uint16_t iso639_1_codes[] =
{
#define PACK(a,b) ((uint16_t)((((uint8_t)(a))<<8)|((uint8_t)(b))))
    PACK('a', 'a'), //  Afar
    PACK('a', 'b'), //  Abkhazian
    PACK('a', 'f'), //  Afrikaans
    PACK('a', 'm'), //  Amharic
    PACK('a', 'r'), //  Arabic
    PACK('a', 's'), //  Assamese
    PACK('a', 'y'), //  Aymara
    PACK('a', 'z'), //  Azerbaijani
    PACK('b', 'a'), //  Bashkir
    PACK('b', 'e'), //  Byelorussian
    PACK('b', 'g'), //  Bulgarian
    PACK('b', 'h'), //  Bihari
    PACK('b', 'i'), //  Bislama
    PACK('b', 'n'), //  Bengali
    PACK('b', 'o'), //  Tibetan
    PACK('b', 'r'), //  Breton
    PACK('c', 'a'), //  Catalan
    PACK('c', 'o'), //  Corsican
    PACK('c', 's'), //  Czech
    PACK('c', 'y'), //  Welch
    PACK('d', 'a'), //  Danish
    PACK('d', 'e'), //  German
    PACK('d', 'v'), //  Divehi
    PACK('d', 'z'), //  Bhutani
    PACK('e', 'l'), //  Greek
    PACK('e', 'n'), //  English
    PACK('e', 'o'), //  Esperanto
    PACK('e', 's'), //  Spanish
    PACK('e', 't'), //  Estonian
    PACK('e', 'u'), //  Basque
    PACK('f', 'a'), //  Persian
    PACK('f', 'i'), //  Finnish
    PACK('f', 'j'), //  Fiji
    PACK('f', 'o'), //  Faeroese
    PACK('f', 'r'), //  French
    PACK('f', 'y'), //  Frisian
    PACK('g', 'a'), //  Irish
    PACK('g', 'd'), //  Scots Gaelic
    PACK('g', 'l'), //  Galician
    PACK('g', 'n'), //  Guarani
    PACK('g', 'u'), //  Gujarati
    PACK('h', 'a'), //  Hausa
    PACK('h', 'i'), //  Hindi
    PACK('h', 'e'), //  Hebrew
    PACK('h', 'r'), //  Croatian
    PACK('h', 'u'), //  Hungarian
    PACK('h', 'y'), //  Armenian
    PACK('i', 'a'), //  Interlingua
    PACK('i', 'd'), //  Indonesian
    PACK('i', 'e'), //  Interlingue
    PACK('i', 'k'), //  Inupiak
    PACK('i', 'n'), //  former Indonesian
    PACK('i', 's'), //  Icelandic
    PACK('i', 't'), //  Italian
    PACK('i', 'u'), //  Inuktitut (Eskimo)
    PACK('i', 'w'), //  former Hebrew
    PACK('j', 'a'), //  Japanese
    PACK('j', 'i'), //  former Yiddish
    PACK('j', 'w'), //  Javanese
    PACK('k', 'a'), //  Georgian
    PACK('k', 'k'), //  Kazakh
    PACK('k', 'l'), //  Greenlandic
    PACK('k', 'm'), //  Cambodian
    PACK('k', 'n'), //  Kannada
    PACK('k', 'o'), //  Korean
    PACK('k', 's'), //  Kashmiri
    PACK('k', 'u'), //  Kurdish
    PACK('k', 'y'), //  Kirghiz
    PACK('l', 'a'), //  Latin
    PACK('l', 'n'), //  Lingala
    PACK('l', 'o'), //  Laothian
    PACK('l', 't'), //  Lithuanian
    PACK('l', 'v'), //  Latvian, Lettish
    PACK('m', 'g'), //  Malagasy
    PACK('m', 'i'), //  Maori
    PACK('m', 'k'), //  Macedonian
    PACK('m', 'l'), //  Malayalam
    PACK('m', 'n'), //  Mongolian
    PACK('m', 'o'), //  Moldavian
    PACK('m', 'r'), //  Marathi
    PACK('m', 's'), //  Malay
    PACK('m', 't'), //  Maltese
    PACK('m', 'y'), //  Burmese
    PACK('n', 'a'), //  Nauru
    PACK('n', 'e'), //  Nepali
    PACK('n', 'l'), //  Dutch
    PACK('n', 'o'), //  Norwegian
    PACK('o', 'c'), //  Occitan
    PACK('o', 'm'), //  (Afan) Oromo
    PACK('o', 'r'), //  Oriya
    PACK('p', 'a'), //  Punjabi
    PACK('p', 'l'), //  Polish
    PACK('p', 's'), //  Pashto, Pushto
    PACK('p', 't'), //  Portuguese
    PACK('q', 'u'), //  Quechua
    PACK('r', 'm'), //  Rhaeto-Romance
    PACK('r', 'n'), //  Kirundi
    PACK('r', 'o'), //  Romanian
    PACK('r', 'u'), //  Russian
    PACK('r', 'w'), //  Kinyarwanda
    PACK('s', 'a'), //  Sanskrit
    PACK('s', 'd'), //  Sindhi
    PACK('s', 'g'), //  Sangro
    PACK('s', 'h'), //  Serbo-Croatian
    PACK('s', 'i'), //  Singhalese
    PACK('s', 'k'), //  Slovak
    PACK('s', 'l'), //  Slovenian
    PACK('s', 'm'), //  Samoan
    PACK('s', 'n'), //  Shona
    PACK('s', 'o'), //  Somali
    PACK('s', 'q'), //  Albanian
    PACK('s', 'r'), //  Serbian
    PACK('s', 's'), //  Siswati
    PACK('s', 't'), //  Sesotho
    PACK('s', 'u'), //  Sudanese
    PACK('s', 'v'), //  Swedish
    PACK('s', 'w'), //  Swahili
    PACK('t', 'a'), //  Tamil
    PACK('t', 'e'), //  Tegulu
    PACK('t', 'g'), //  Tajik
    PACK('t', 'h'), //  Thai
    PACK('t', 'i'), //  Tigrinya
    PACK('t', 'k'), //  Turkmen
    PACK('t', 'l'), //  Tagalog
    PACK('t', 'n'), //  Setswana
    PACK('t', 'o'), //  Tonga
    PACK('t', 'r'), //  Turkish
    PACK('t', 's'), //  Tsonga
    PACK('t', 't'), //  Tatar
    PACK('t', 'w'), //  Twi
    PACK('u', 'g'), //  Uigur
    PACK('u', 'k'), //  Ukrainian
    PACK('u', 'r'), //  Urdu
    PACK('u', 'z'), //  Uzbek
    PACK('v', 'i'), //  Vietnamese
    PACK('v', 'o'), //  Volapuk
    PACK('w', 'o'), //  Wolof
    PACK('x', 'h'), //  Xhosa
    PACK('y', 'i'), //  Yiddish
    PACK('y', 'o'), //  Yoruba
    PACK('z', 'a'), //  Zhuang
    PACK('z', 'h'), //  Chinese
    PACK('z', 'u'), //  Zulu
#undef PACK
};

foil_langcode_t foil_langcode_from_iso639_1(const char *iso639_1_code)
{
    uint16_t iso639_1;
    iso639_1 = (uint16_t)(uint8_t)iso639_1_code[1];
    iso639_1 |= (uint16_t)((uint16_t)(iso639_1_code[0]) << 8);

    if (!iso639_1)
        return FOIL_LANGCODE_unknown;

    unsigned int lower = 0;
    unsigned int upper = PCA_TABLESIZE (iso639_1_codes) - 1;
    unsigned int mid = PCA_TABLESIZE (iso639_1_codes) / 2;

    do {
        if (iso639_1 < iso639_1_codes[mid])
            upper = mid - 1;
        else if (iso639_1 > iso639_1_codes[mid])
            lower = mid + 1;
        else
            return (foil_langcode_t)mid;

        mid = (lower + upper) / 2;
    } while (lower <= upper);

    return FOIL_LANGCODE_unknown;
}

static const char* iso639_1_codes_str[] =
{
    "aa", //  Afar
    "ab", //  Abkhazian
    "af", //  Afrikaans
    "am", //  Amharic
    "ar", //  Arabic
    "as", //  Assamese
    "ay", //  Aymara
    "az", //  Azerbaijani
    "ba", //  Bashkir
    "be", //  Byelorussian
    "bg", //  Bulgarian
    "bh", //  Bihari
    "bi", //  Bislama
    "bn", //  Bengali
    "bo", //  Tibetan
    "br", //  Breton
    "ca", //  Catalan
    "co", //  Corsican
    "cs", //  Czech
    "cy", //  Welch
    "da", //  Danish
    "de", //  German
    "dv", //  Divehi
    "dz", //  Bhutani
    "el", //  Greek
    "en", //  English
    "eo", //  Esperanto
    "es", //  Spanish
    "et", //  Estonian
    "eu", //  Basque
    "fa", //  Persian
    "fi", //  Finnish
    "fj", //  Fiji
    "fo", //  Faeroese
    "fr", //  French
    "fy", //  Frisian
    "ga", //  Irish
    "gd", //  Scots Gaelic
    "gl", //  Galician
    "gn", //  Guarani
    "gu", //  Gujarati
    "ha", //  Hausa
    "hi", //  Hindi
    "he", //  Hebrew
    "hr", //  Croatian
    "hu", //  Hungarian
    "hy", //  Armenian
    "ia", //  Interlingua
    "id", //  Indonesian
    "ie", //  Interlingue
    "ik", //  Inupiak
    "in", //  former Indonesian
    "is", //  Icelandic
    "it", //  Italian
    "iu", //  Inuktitut (Eskimo)
    "iw", //  former Hebrew
    "ja", //  Japanese
    "ji", //  former Yiddish
    "jw", //  Javanese
    "ka", //  Georgian
    "kk", //  Kazakh
    "kl", //  Greenlandic
    "km", //  Cambodian
    "kn", //  Kannada
    "ko", //  Korean
    "ks", //  Kashmiri
    "ku", //  Kurdish
    "ky", //  Kirghiz
    "la", //  Latin
    "ln", //  Lingala
    "lo", //  Laothian
    "lt", //  Lithuanian
    "lv", //  Latvian, Lettish
    "mg", //  Malagasy
    "mi", //  Maori
    "mk", //  Macedonian
    "ml", //  Malayalam
    "mn", //  Mongolian
    "mo", //  Moldavian
    "mr", //  Marathi
    "ms", //  Malay
    "mt", //  Maltese
    "my", //  Burmese
    "na", //  Nauru
    "ne", //  Nepali
    "nl", //  Dutch
    "no", //  Norwegian
    "oc", //  Occitan
    "om", //  (Afan) Oromo
    "or", //  Oriya
    "pa", //  Punjabi
    "pl", //  Polish
    "ps", //  Pashto, Pushto
    "pt", //  Portuguese
    "qu", //  Quechua
    "rm", //  Rhaeto-Romance
    "rn", //  Kirundi
    "ro", //  Romanian
    "ru", //  Russian
    "rw", //  Kinyarwanda
    "sa", //  Sanskrit
    "sd", //  Sindhi
    "sg", //  Sangro
    "sh", //  Serbo-Croatian
    "si", //  Singhalese
    "sk", //  Slovak
    "sl", //  Slovenian
    "sm", //  Samoan
    "sn", //  Shona
    "so", //  Somali
    "sq", //  Albanian
    "sr", //  Serbian
    "ss", //  Siswati
    "st", //  Sesotho
    "su", //  Sudanese
    "sv", //  Swedish
    "sw", //  Swahili
    "ta", //  Tamil
    "te", //  Tegulu
    "tg", //  Tajik
    "th", //  Thai
    "ti", //  Tigrinya
    "tk", //  Turkmen
    "tl", //  Tagalog
    "tn", //  Setswana
    "to", //  Tonga
    "tr", //  Turkish
    "ts", //  Tsonga
    "tt", //  Tatar
    "tw", //  Twi
    "ug", //  Uigur
    "uk", //  Ukrainian
    "ur", //  Urdu
    "uz", //  Uzbek
    "vi", //  Vietnamese
    "vo", //  Volapuk
    "wo", //  Wolof
    "xh", //  Xhosa
    "yi", //  Yiddish
    "yo", //  Yoruba
    "za", //  Zhuang
    "zh", //  Chinese
    "zu", //  Zulu
};

const char* foil_langcode_to_iso639_1(foil_langcode_t lc)
{
    switch (lc) {
    case FOIL_LANGCODE_unknown:
        return "";
    default:
        return iso639_1_codes_str[lc];
    }

    return "";
}

