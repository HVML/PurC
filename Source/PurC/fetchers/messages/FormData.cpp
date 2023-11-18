/*
 * Copyright (C) 2004, 2006, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2012 Digia Plc. and/or its subsidiary(-ies)
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "FormData.h"

#include "SharedBuffer.h"
#include <wtf/FileSystem.h>
#include <wtf/text/LineEnding.h>

namespace PurCFetcher {

inline FormData::FormData()
{
}

inline FormData::FormData(const FormData& data)
    : RefCounted<FormData>()
    , m_elements(data.m_elements)
    , m_identifier(data.m_identifier)
    , m_alwaysStream(false)
    , m_containsPasswordData(data.m_containsPasswordData)
{
}

FormData::~FormData()
{
}

Ref<FormData> FormData::create()
{
    return adoptRef(*new FormData);
}

Ref<FormData> FormData::create(const void* data, size_t size)
{
    auto result = create();
    result->appendData(data, size);
    return result;
}

Ref<FormData> FormData::create(const CString& string)
{
    return create(string.data(), string.length());
}

Ref<FormData> FormData::create(const Vector<char>& vector)
{
    return create(vector.data(), vector.size());
}

Ref<FormData> FormData::create(Vector<uint8_t>&& vector)
{
    auto data = create();
    data->m_elements.append(WTFMove(vector));
    return data;
}

Ref<FormData> FormData::create(const Vector<uint8_t>& vector)
{
    return create(vector.data(), vector.size());
}

Ref<FormData> FormData::copy() const
{
    return adoptRef(*new FormData(*this));
}

Ref<FormData> FormData::isolatedCopy() const
{
    // FIXME: isolatedCopy() does not copy m_identifier, m_boundary, or m_containsPasswordData.
    // Is all of that correct and intentional?

    auto formData = create();

    formData->m_alwaysStream = m_alwaysStream;

    formData->m_elements.reserveInitialCapacity(m_elements.size());
    for (auto& element : m_elements)
        formData->m_elements.uncheckedAppend(element.isolatedCopy());

    return formData;
}

uint64_t FormDataElement::lengthInBytes(const Function<uint64_t(const URL&)>& blobSize) const
{
    return switchOn(data,
        [] (const Vector<uint8_t>& bytes) {
            return static_cast<uint64_t>(bytes.size());
        }, [] (const FormDataElement::EncodedFileData& fileData) {
            if (fileData.fileLength != -1)
                return static_cast<uint64_t>(fileData.fileLength);
            return FileSystem::fileSize(fileData.filename).value_or(0);
        }, [&blobSize] (const FormDataElement::EncodedBlobData& blobData) {
            return blobSize(blobData.url);
        }
    );
}

uint64_t FormDataElement::lengthInBytes() const
{
    return 0;
}

FormDataElement FormDataElement::isolatedCopy() const
{
    return switchOn(data,
        [] (const Vector<uint8_t>& bytes) {
            Vector<uint8_t> copy;
            copy.append(bytes.data(), bytes.size());
            return FormDataElement(WTFMove(copy));
        }, [] (const FormDataElement::EncodedFileData& fileData) {
            return FormDataElement(fileData.isolatedCopy());
        }, [] (const FormDataElement::EncodedBlobData& blobData) {
            return FormDataElement(blobData.url.isolatedCopy());
        }
    );
}

void FormData::appendData(const void* data, size_t size)
{
    m_lengthInBytes = std::nullopt;
    if (!m_elements.isEmpty()) {
        if (auto* vector = PurCWTF::get_if<Vector<uint8_t>>(m_elements.last().data)) {
            vector->append(reinterpret_cast<const uint8_t*>(data), size);
            return;
        }
    }
    Vector<uint8_t> vector;
    vector.append(reinterpret_cast<const char*>(data), size);
    m_elements.append(WTFMove(vector));
}

void FormData::appendFile(const String& filename)
{
    m_elements.append(FormDataElement(filename, 0, -1, std::nullopt));
    m_lengthInBytes = std::nullopt;
}

void FormData::appendFileRange(const String& filename, long long start, long long length, std::optional<WallTime> expectedModificationTime)
{
    m_elements.append(FormDataElement(filename, start, length, expectedModificationTime));
    m_lengthInBytes = std::nullopt;
}

void FormData::appendBlob(const URL& blobURL)
{
    m_elements.append(FormDataElement(blobURL));
    m_lengthInBytes = std::nullopt;
}

void FormData::appendMultiPartStringValue(const String&, Vector<uint8_t>&, TextEncoding&)
{
}

Vector<uint8_t> FormData::flatten() const
{
    // Concatenate all the byte arrays, but omit any files.
    Vector<uint8_t> data;
    for (auto& element : m_elements) {
        if (auto* vector = PurCWTF::get_if<Vector<uint8_t>>(element.data))
            data.append(vector->data(), vector->size());
    }
    return data;
}

String FormData::flattenToString() const
{
    auto bytes = flatten();
//    return Latin1Encoding().decode(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return String(bytes.data(), bytes.size());
}

FormDataForUpload FormData::prepareForUpload()
{
    Vector<String> generatedFiles;
    for (auto& element : m_elements) {
        auto* fileData = PurCWTF::get_if<FormDataElement::EncodedFileData>(element.data);
        if (!fileData)
            continue;
        if (FileSystem::fileTypeFollowingSymlinks(fileData->filename) != FileSystem::FileType::Directory)
            continue;
        if (fileData->fileStart || fileData->fileLength != -1)
            continue;
        if (!fileData->fileModificationTimeMatchesExpectation())
            continue;

        auto generatedFilename = FileSystem::createTemporaryZipArchive(fileData->filename);
        if (!generatedFilename)
            continue;
        fileData->filename = generatedFilename;
        generatedFiles.append(WTFMove(generatedFilename));
    }

    return { *this, WTFMove(generatedFiles) };
}

FormDataForUpload::FormDataForUpload(FormData& data, Vector<String>&& temporaryZipFiles)
    : m_data(data)
    , m_temporaryZipFiles(WTFMove(temporaryZipFiles))
{
}

FormDataForUpload::~FormDataForUpload()
{
    ASSERT(isMainThread());
    for (auto& file : m_temporaryZipFiles)
        FileSystem::deleteFile(file);
}

uint64_t FormData::lengthInBytes() const
{
    if (!m_lengthInBytes) {
        uint64_t length = 0;
        for (auto& element : m_elements)
            length += element.lengthInBytes();
        m_lengthInBytes = length;
    }
    return *m_lengthInBytes;
}

RefPtr<SharedBuffer> FormData::asSharedBuffer() const
{
    for (auto& element : m_elements) {
        if (!PurCWTF::holds_alternative<Vector<uint8_t>>(element.data))
            return nullptr;
    }
    return SharedBuffer::create(flatten());
}

URL FormData::asBlobURL() const
{
    if (m_elements.size() != 1)
        return { };

    if (auto* blobData = PurCWTF::get_if<FormDataElement::EncodedBlobData>(m_elements.first().data))
        return blobData->url;
    return { };
}

bool FormDataElement::EncodedFileData::fileModificationTimeMatchesExpectation() const
{
    if (!expectedFileModificationTime)
        return true;

    auto fileModificationTime = FileSystem::fileModificationTime(filename);
    if (!fileModificationTime)
        return false;

    if (fileModificationTime->secondsSinceEpoch().secondsAs<time_t>() != expectedFileModificationTime->secondsSinceEpoch().secondsAs<time_t>())
        return false;

    return true;
}

} // namespace PurCFetcher
