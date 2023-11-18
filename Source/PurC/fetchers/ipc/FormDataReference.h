/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Decoder.h"
#include "Encoder.h"
#include "SandboxExtension.h"
#include "FormData.h"

namespace IPC {

class FormDataReference {
public:
    FormDataReference() = default;
    explicit FormDataReference(RefPtr<PurCFetcher::FormData>&& data)
        : m_data(WTFMove(data))
    {
    }

    RefPtr<PurCFetcher::FormData> takeData() { return WTFMove(m_data); }

    void encode(Encoder& encoder) const
    {
        encoder << !!m_data;
        if (!m_data)
            return;

        encoder << *m_data;

        auto& elements = m_data->elements();
        size_t fileCount = std::count_if(elements.begin(), elements.end(), [](auto& element) {
            return PurCWTF::holds_alternative<PurCFetcher::FormDataElement::EncodedFileData>(element.data);
        });

        PurCFetcher::SandboxExtension::HandleArray sandboxExtensionHandles;
        sandboxExtensionHandles.allocate(fileCount);
        size_t extensionIndex = 0;
        for (auto& element : elements) {
            if (auto* fileData = PurCWTF::get_if<PurCFetcher::FormDataElement::EncodedFileData>(element.data)) {
                const String& path = fileData->filename;
                PurCFetcher::SandboxExtension::createHandle(path, PurCFetcher::SandboxExtension::Type::ReadOnly, sandboxExtensionHandles[extensionIndex++]);
            }
        }
        encoder << sandboxExtensionHandles;
    }

    static std::optional<FormDataReference> decode(Decoder& decoder)
    {
        std::optional<bool> hasFormData;
        decoder >> hasFormData;
        if (!hasFormData)
            return std::nullopt;
        if (!hasFormData.value())
            return FormDataReference { };

        auto formData = PurCFetcher::FormData::decode(decoder);
        if (!formData)
            return std::nullopt;

        std::optional<PurCFetcher::SandboxExtension::HandleArray> sandboxExtensionHandles;
        decoder >> sandboxExtensionHandles;
        if (!sandboxExtensionHandles)
            return std::nullopt;

        PurCFetcher::SandboxExtension::consumePermanently(*sandboxExtensionHandles);

        return FormDataReference { formData.releaseNonNull() };
    }

private:
    RefPtr<PurCFetcher::FormData> m_data;
};

} // namespace IPC
