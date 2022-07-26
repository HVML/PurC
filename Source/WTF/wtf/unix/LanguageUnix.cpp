/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2016, 2017 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include <wtf/Language.h>

#include <locale.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace PurCWTF {

// Using pango_language_get_default() here is not an option, because
// it doesn't support changing the locale in runtime, so it returns
// always the same value.
static String platformLanguage()
{
    String localeDefault(setlocale(LC_CTYPE, nullptr));
    if (localeDefault.isEmpty() || equalIgnoringASCIICase(localeDefault, "C") || equalIgnoringASCIICase(localeDefault, "POSIX"))
        return "en-US"_s;

    String normalizedDefault = localeDefault;
    normalizedDefault.replace('_', '-');
    normalizedDefault.truncate(normalizedDefault.find('.'));
    return normalizedDefault;
}

Vector<String> platformUserPreferredLanguages()
{
    return { platformLanguage() };
}

} // namespace PurCWTF
