/*
 * Copyright (C) 2006-2016 Apple Inc. All rights reserved.
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
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

namespace PurCFetcher {

struct TypeExtensionPair {
    ASCIILiteral type;
    ASCIILiteral extension;
};

PURCFETCHER_EXPORT const std::initializer_list<TypeExtensionPair>& commonMediaTypes();

struct MIMETypeRegistryThreadGlobalData {
    WTF_MAKE_NONCOPYABLE(MIMETypeRegistryThreadGlobalData);
    WTF_MAKE_FAST_ALLOCATED;
public:
    MIMETypeRegistryThreadGlobalData(HashSet<String, ASCIICaseInsensitiveHash>&& supportedImageMIMETypesForEncoding)
        : m_supportedImageMIMETypesForEncoding(supportedImageMIMETypesForEncoding)
    { }

    const HashSet<String, ASCIICaseInsensitiveHash>& supportedImageMIMETypesForEncoding() const { return m_supportedImageMIMETypesForEncoding; }

private:
    HashSet<String, ASCIICaseInsensitiveHash> m_supportedImageMIMETypesForEncoding;
};

class MIMETypeRegistry {
public:
    PURCFETCHER_EXPORT static String getMIMETypeForExtension(const String& extension);

    // FIXME: PurCFetcher coding style says we should not have the word "get" in the names of these functions.
    static Vector<String> getExtensionsForMIMEType(const String& type);
    PURCFETCHER_EXPORT static String getPreferredExtensionForMIMEType(const String& type);
    PURCFETCHER_EXPORT static String getMediaMIMETypeForExtension(const String& extension);
    static Vector<String> getMediaMIMETypesForExtension(const String& extension);

    static String getMIMETypeForPath(const String& path);

    static std::unique_ptr<MIMETypeRegistryThreadGlobalData> createMIMETypeRegistryThreadGlobalData();

    // Check to see if a MIME type is suitable for being loaded inline as an
    // image (e.g., <img> tags).
    PURCFETCHER_EXPORT static bool isSupportedImageMIMEType(const String& mimeType);

    // Check to see if a MIME type is suitable for being loaded as an image, including SVG and Video (where supported).
    PURCFETCHER_EXPORT static bool isSupportedImageVideoOrSVGMIMEType(const String& mimeType);

    // Check to see if a MIME type is suitable for being encoded.
    static bool isSupportedImageMIMETypeForEncoding(const String& mimeType);

    // Check to see if a MIME type is suitable for being loaded as a JavaScript or JSON resource.
    PURCFETCHER_EXPORT static bool isSupportedJavaScriptMIMEType(const String& mimeType);
    PURCFETCHER_EXPORT static bool isSupportedJSONMIMEType(const String& mimeType);

    // Check to see if a MIME type is suitable for being loaded as a style sheet.
    static bool isSupportedStyleSheetMIMEType(const String& mimeType);

    // Check to see if a MIME type is suitable for being loaded as a font.
    static bool isSupportedFontMIMEType(const String& mimeType);

    // Check to see if a MIME type is a text media playlist type, such as an m3u8.
    static bool isTextMediaPlaylistMIMEType(const String& mimeType);

    // Check to see if a non-image MIME type is suitable for being loaded as a
    // document in a frame. Does not include supported JavaScript and JSON MIME types.
    PURCFETCHER_EXPORT static bool isSupportedNonImageMIMEType(const String& mimeType);

    // Check to see if a MIME type is suitable for being loaded using <video> and <audio>.
    PURCFETCHER_EXPORT static bool isSupportedMediaMIMEType(const String& mimeType);

    // Check to see if a MIME type is suitable for being loaded using <track>>.
    PURCFETCHER_EXPORT static bool isSupportedTextTrackMIMEType(const String& mimeType);

    // Check to see if a MIME type is a valid Java applet mime type.
    PURCFETCHER_EXPORT static bool isJavaAppletMIMEType(const String& mimeType);

    // Check to see if a MIME type is a plugin implemented by the browser.
    static bool isApplicationPluginMIMEType(const String& mimeType);

    // Check to see if a MIME type is one of the common PDF/PS types.
    static bool isPDFMIMEType(const String& mimeType);
    static bool isPostScriptMIMEType(const String& mimeType);
    PURCFETCHER_EXPORT static bool isPDFOrPostScriptMIMEType(const String& mimeType);

    PURCFETCHER_EXPORT static bool isSystemPreviewMIMEType(const String& mimeType);

    // Check to see if a MIME type is suitable for being shown inside a page.
    // Returns true if any of isSupportedImageMIMEType(), isSupportedNonImageMIMEType(),
    // isSupportedMediaMIMEType(), isSupportedJavaScriptMIMEType(), isSupportedJSONMIMEType(),
    // returns true or if the given MIME type begins with "text/" and
    // isUnsupportedTextMIMEType() returns false.
    PURCFETCHER_EXPORT static bool canShowMIMEType(const String& mimeType);

    // Check to see if a MIME type is one where an XML document should be created
    // rather than an HTML document.
    PURCFETCHER_EXPORT static bool isXMLMIMEType(const String& mimeType);

    // Check to see if a MIME type is for an XML external entity resource.
    PURCFETCHER_EXPORT static bool isXMLEntityMIMEType(StringView mimeType);

    // Used in page load algorithm to decide whether to display as a text
    // document in a frame. Not a good idea to use elsewhere, because that code
    // makes this test is after many other tests are done on the MIME type.
    PURCFETCHER_EXPORT static bool isTextMIMEType(const String& mimeType);

    // FIXME: Would be nice to find a way to avoid exposing these sets, even worse exposing non-const references.
    PURCFETCHER_EXPORT static const HashSet<String, ASCIICaseInsensitiveHash>& supportedImageMIMETypes();
    static HashSet<String, ASCIICaseInsensitiveHash>& additionalSupportedImageMIMETypes();
    PURCFETCHER_EXPORT static HashSet<String, ASCIICaseInsensitiveHash>& supportedNonImageMIMETypes();
    PURCFETCHER_EXPORT static const HashSet<String, ASCIICaseInsensitiveHash>& supportedMediaMIMETypes();
    PURCFETCHER_EXPORT static const HashSet<String, ASCIICaseInsensitiveHash>& pdfMIMETypes();
    PURCFETCHER_EXPORT static const HashSet<String, ASCIICaseInsensitiveHash>& unsupportedTextMIMETypes();
    PURCFETCHER_EXPORT static const HashSet<String, ASCIICaseInsensitiveHash>& systemPreviewMIMETypes();

    // FIXME: PurCFetcher coding style says we should not have the word "get" in the name of this function.
    // FIXME: Unclear what the concept of a normalized MIME type is; currently it's a platform-specific notion.
    static String getNormalizedMIMEType(const String&);

    PURCFETCHER_EXPORT static String appendFileExtensionIfNecessary(const String& filename, const String& mimeType);

private:
    // Check to see if the MIME type is not suitable for being loaded as a text
    // document in a frame. Only valid for MIME types begining with "text/".
    static bool isUnsupportedTextMIMEType(const String& mimeType);
};

PURCFETCHER_EXPORT const String& defaultMIMEType();

} // namespace PurCFetcher
