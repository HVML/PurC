/*
 * @file url.c
 * @author Geng Yue
 * @date 2021/12/26
 * @brief The implementation of URL implementation.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
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
 */

#include "purc-utils.h"

#include "wtf/URL.h"
#include "wtf/ASCIICType.h"
#include "wtf/text/StringBuilder.h"

void
pcutils_broken_down_url_clear(struct purc_broken_down_url *broken_down)
{
    if (broken_down->schema) {
        free(broken_down->schema);
        broken_down->schema = NULL;
    }

    if (broken_down->user) {
        free(broken_down->user);
        broken_down->user = NULL;
    }

    if (broken_down->passwd) {
        free(broken_down->passwd);
        broken_down->passwd = NULL;
    }

    if (broken_down->host) {
        free(broken_down->host);
        broken_down->host = NULL;
    }

    if (broken_down->path) {
        free(broken_down->path);
        broken_down->path = NULL;
    }

    if (broken_down->query) {
        free(broken_down->query);
        broken_down->query = NULL;
    }

    if (broken_down->fragment) {
        free(broken_down->fragment);
        broken_down->fragment = NULL;
    }

    broken_down->port = 0;
}

void
pcutils_broken_down_url_delete(struct purc_broken_down_url *broken_down)
{
    assert(broken_down != NULL);

    if (broken_down->schema) {
        free(broken_down->schema);
    }

    if (broken_down->user) {
        free(broken_down->user);
    }

    if (broken_down->passwd) {
        free(broken_down->passwd);
    }

    if (broken_down->host) {
        free(broken_down->host);
    }

    if (broken_down->path) {
        free(broken_down->path);
    }

    if (broken_down->query) {
        free(broken_down->query);
    }

    if (broken_down->fragment) {
        free(broken_down->fragment);
    }

    free(broken_down);
}

char *pcutils_url_assemble(const struct purc_broken_down_url *url_struct,
        bool keep_percent_escaped)
{
    char * url_string = NULL;
    String string = "";
    PurCWTF::URL url(PurCWTF::URL(), string);

    if (url_struct->schema)
        url.setProtocol(url_struct->schema);

    if (url_struct->host)
        url.setHost(url_struct->host);

    if (url_struct->port)
        url.setPort(url_struct->port);

    if (url_struct->path)
        url.setPath(url_struct->path);

    if (url_struct->query)
        url.setQuery(url_struct->query);

    if (url_struct->fragment)
        url.setFragmentIdentifier(url_struct->fragment);

    if (url_struct->user)
        url.setUser(url_struct->user);

    if (url_struct->passwd)
        url.setPassword(url_struct->passwd);

    if (url.isValid()) {
        if (keep_percent_escaped) {
            String tempstring = url.string();
            url_string = strdup(tempstring.utf8().data());
        }
        else {
            String tempstring = PurCWTF::decodeEscapeSequencesFromParsedURL(url.string());
            url_string = strdup(tempstring.utf8().data());
        }
    }

    return url_string;
}

// escape char which is grater than 127.
// whether other char is valid, URL parser will do it.
static bool shouldEncode(unsigned char c)
{
    if (c > 127)
        return true;
    else
        return false;
}

static String percentEncodeCharacters(const unsigned char * data)
{
    PurCWTF::StringBuilder builder;
    auto length = strlen ((char *)data);
    for (unsigned j = 0; j < length; j++) {
        auto c = data[j];
        if (shouldEncode(c)) {
            builder.append('%');
            builder.append(upperNibbleToASCIIHexDigit(c));
            builder.append(lowerNibbleToASCIIHexDigit(c));
        } else
            builder.append(c);
    }
    return builder.toString();
}

bool pcutils_url_break_down(struct purc_broken_down_url *url_struct,
        const char *url_string)
{
    // std::unique_ptr<PurCWTF::URL> url = makeUnique<URL>(URL(), url_string);
    String encode_url = percentEncodeCharacters((const unsigned char *)url_string);
    PurCWTF::URL url(URL(), encode_url);

    bool valid = url.isValid();
    size_t length = 0;
    const char *tempstring = NULL;

    if (valid) {
        if (url_struct->schema)
            free(url_struct->schema);
        StringView protocol = url.protocol();
        CString utf8 = protocol.toString().utf8();
        tempstring = utf8.data();
        length = strlen(tempstring);
        if (length)
            url_struct->schema = strdup(tempstring);
        else
            url_struct->schema = NULL;

        if (url_struct->user)
            free(url_struct->user);
        String user = url.user();
        utf8 = user.utf8();
        tempstring = utf8.data();
        length = strlen(tempstring);
        if (length)
            url_struct->user = strdup(tempstring);
        else
            url_struct->user = NULL;

        if (url_struct->passwd)
            free(url_struct->passwd);
        String password = url.password();
        utf8 = password.utf8();
        tempstring = utf8.data();
        length = strlen(tempstring);
        if (length)
            url_struct->passwd = strdup(tempstring);
        else
            url_struct->passwd = NULL;

        if (url_struct->host)
            free(url_struct->host);
        StringView host = url.host();
        utf8 = host.toString().utf8();
        tempstring = utf8.data();
        length = strlen(tempstring);
        if (length)
            url_struct->host = strdup(tempstring);
        else
            url_struct->host = NULL;

        if (url_struct->path)
            free(url_struct->path);
        if (url.isLocalFile()) {
            utf8 = url.fileSystemPath().utf8();
        }
        else {
            StringView path = url.path();
            utf8 = path.toString().utf8();
        }

        tempstring = utf8.data();
        length = strlen(tempstring);
        if (length)
            url_struct->path = strdup(tempstring);
        else
            url_struct->path = NULL;

        if (url_struct->query)
            free(url_struct->query);
        StringView query = url.query();
        utf8 = query.toString().utf8();
        tempstring = utf8.data();
        length = strlen(tempstring);
        if (length)
            url_struct->query = strdup(tempstring);
        else
            url_struct->query = NULL;

        if (url_struct->fragment)
            free(url_struct->fragment);
        StringView fragmentIdentifier = url.fragmentIdentifier();
        utf8 = fragmentIdentifier.toString().utf8();
        tempstring = utf8.data();
        length = strlen(tempstring);
        if (length)
            url_struct->fragment = strdup(tempstring);
        else
            url_struct->fragment = NULL;

        Optional<uint16_t> port = url.port();
        if (port)
            url_struct->port = (unsigned int)(*port);
        else
            url_struct->port = 0;
    }

    return valid;
}

bool pcutils_url_is_valid(const char *url_string)
{
    String encode_url = percentEncodeCharacters((const unsigned char *)url_string);
    PurCWTF::URL url(URL(), encode_url);

    return url.isValid();
}

#define PAIR_SEPERATOR      '&'
#define KV_SEPERATOR        '='

static size_t get_key_len(const char *str)
{
    size_t len = 0;

    while (*str && *str != KV_SEPERATOR) {
        len++;
        str++;
    }

    return len;
}

static size_t get_value_len(const char *str)
{
    size_t len = 0;

    while (*str && *str != PAIR_SEPERATOR) {
        len++;
        str++;
    }

    return len;
}

static const char *locate_query_value(const char *query, const char *key)
{
    size_t key_len = strlen(key);
    if (key_len == 0)
        return NULL;

    if (query[0] == 0)
        return NULL;

    char my_key[key_len + 2];
    strcpy(my_key, key);
    my_key[key_len] = KV_SEPERATOR;
    key_len++;
    my_key[key_len] = 0;

    const char *left = query;
    while (*left) {
        if (strncasecmp(left, my_key, key_len) == 0) {
            return left + key_len;
        }
        else {
            const char *value = left + get_key_len(left);
            unsigned int value_len = get_value_len(value);
            left = value + value_len;
            if (*left == PAIR_SEPERATOR)
                left++;
        }
    }

    return NULL;
}

bool
pcutils_url_get_query_value(const struct purc_broken_down_url *broken_down,
        const char *key, char *value_buff)
{
    assert(broken_down);
    if (broken_down->query == NULL)
        return false;

    const char *value = locate_query_value(broken_down->query, key);
    if (value == NULL) {
        return false;
    }

    size_t value_len = get_value_len(value);
    if (value_len == 0) {
        return false;
    }

    strncpy(value_buff, value, value_len);
    value_buff[value_len] = 0;
    return true;
}

bool
pcutils_url_get_query_value_alloc(
        const struct purc_broken_down_url *broken_down,
        const char *key, char **value_buff)
{
    assert(broken_down);
    if (broken_down->query == NULL)
        return false;

    const char *value = locate_query_value(broken_down->query, key);
    if (value == NULL) {
        return false;
    }

    size_t value_len = get_value_len(value);
    if (value_len == 0) {
        return false;
    }

    *value_buff = strndup(value, value_len);
    return true;
}

