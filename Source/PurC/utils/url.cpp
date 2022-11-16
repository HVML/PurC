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
#include "purc-errors.h"
#include "purc-rwstream.h"

#include "private/variant.h"
#include "private/url.h"
#include "wtf/URL.h"
#include "wtf/ASCIICType.h"
#include "wtf/text/StringBuilder.h"

#define BUFF_MIN                1024
#define BUFF_KEY                1024

static int
build_query(purc_rwstream_t rws, const char *k, purc_variant_t v,
        const char *numeric_prefix, char arg_separator,
        unsigned int flags);

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

static int
encode_string(purc_rwstream_t rws, const char *s,
        unsigned int flags)
{
    char space[4];
    size_t nr_space;
    if (flags & PCUTILS_URL_OPT_RFC1738) {
        space[0] = '+';
        space[1] = 0;
        nr_space = 1;
    }
    else {
        space[0] = '%';
        space[1] = '2';
        space[2] = '0';
        space[3] = 0;
        nr_space = 3;
    }

    size_t nr = strlen(s);
    for (size_t i = 0; i < nr; i++) {
        const char byte = s[i];
        if (byte == 0x20) {
            purc_rwstream_write(rws, &space, nr_space);
        }
        else if (byte == 0x2A
            || byte == 0x2D
            || byte == 0x2E
            || (byte >= 0x30 && byte <= 0x39)
            || (byte >= 0x41 && byte <= 0x5A)
            || byte == 0x5F
            || (byte >= 0x61 && byte <= 0x7A)) {
            purc_rwstream_write(rws, &byte, 1);
        }
        else {
            purc_rwstream_write(rws, "%", 1);
            char v = upperNibbleToASCIIHexDigit(byte);
            purc_rwstream_write(rws, &v, 1);

            v = lowerNibbleToASCIIHexDigit(byte);
            purc_rwstream_write(rws, &v, 1);
        }
    }
    return 0;
}

static int
encode_object(purc_rwstream_t rws, const char *k, purc_variant_t v,
        const char *numeric_prefix, char arg_separator,
        unsigned int flags)
{
    int ret = -1;
    size_t nr_k = k ? strlen(k) : 0;
    char *key = NULL;
    purc_variant_t ok;
    purc_variant_t ov;
    ssize_t nr_size = purc_variant_object_get_size(v);
    if (nr_size == 0) {
        ret = 0;
        goto out;
    }
    foreach_key_value_in_variant_object(v, ok, ov)
        const char *sk = purc_variant_get_string_const(ok);
        size_t nr_sk = strlen(sk);
        if (k) {
            key = (char*)malloc(nr_k + nr_sk + 3);
            sprintf(key, "%s[%s]", k, sk);
        }
        else {
            key = strdup(sk);
        }
        if (!key) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            ret = PURC_ERROR_OUT_OF_MEMORY;
            goto out;
        }

        if (purc_rwstream_tell(rws) > 0) {
            purc_rwstream_write(rws, &arg_separator, 1);
        }

        ret = build_query(rws, key, ov, numeric_prefix,
                arg_separator, flags);
        free(key);
        if (ret != PURC_ERROR_OK) {
            goto out;
        }
    end_foreach;

out:
    return ret;
}

static int
encode_array(purc_rwstream_t rws, const char *k, purc_variant_t v,
        const char *numeric_prefix, char arg_separator,
        unsigned int flags)
{
    int ret = -1;
    char key[BUFF_KEY];
    purc_variant_t ov;
    size_t idx;
    ssize_t nr_size = purc_variant_array_get_size(v);
    if (nr_size == 0) {
        ret = 0;
        goto out;
    }
    foreach_value_in_variant_array(v, ov, idx)
        if (k) {
            snprintf(key, BUFF_KEY, "%s[%ld]", k, idx);
        }
        else if (numeric_prefix) {
            snprintf(key, BUFF_KEY, "%s%ld", numeric_prefix, idx);
        }
        else {
            snprintf(key, BUFF_KEY, "%ld", idx);
        }

        if (purc_rwstream_tell(rws) > 0) {
            purc_rwstream_write(rws, &arg_separator, 1);
        }

        ret = build_query(rws, key, ov, numeric_prefix,
                arg_separator, flags);
        if (ret != PURC_ERROR_OK) {
            goto out;
        }
    end_foreach;

out:
    return ret;
}

static int
encode_set(purc_rwstream_t rws, const char *k, purc_variant_t v,
        const char *numeric_prefix, char arg_separator,
        unsigned int flags)
{
    int ret = -1;
    char key[BUFF_KEY];
    purc_variant_t ov;
    size_t idx = 0;
    ssize_t nr_size = purc_variant_set_get_size(v);
    if (nr_size == 0) {
        ret = 0;
        goto out;
    }
    foreach_value_in_variant_set_order(v, ov)
        if (k) {
            snprintf(key, BUFF_KEY, "%s[%ld]", k, idx);
        }
        else if (numeric_prefix) {
            snprintf(key, BUFF_KEY, "%s%ld", numeric_prefix, idx);
        }
        else {
            snprintf(key, BUFF_KEY, "%ld", idx);
        }

        if (purc_rwstream_tell(rws) > 0) {
            purc_rwstream_write(rws, &arg_separator, 1);
        }

        ret = build_query(rws, key, ov, numeric_prefix,
                arg_separator, flags);
        if (ret != PURC_ERROR_OK) {
            goto out;
        }
        idx++;
    end_foreach;

out:
    return ret;
}

static int
encode_tuple(purc_rwstream_t rws, const char *k, purc_variant_t v,
        const char *numeric_prefix, char arg_separator,
        unsigned int flags)
{
    int ret = -1;
    char key[BUFF_KEY];
    purc_variant_t ov;
    size_t idx = 0;

    purc_variant_t *members;
    size_t sz;
    members = tuple_members(v, &sz);
    if (sz == 0) {
        ret = 0;
        goto out;
    }
    for (idx = 0; idx < sz; idx++) {
        ov = members[idx];
        if (k) {
            snprintf(key, BUFF_KEY, "%s[%ld]", k, idx);
        }
        else if (numeric_prefix) {
            snprintf(key, BUFF_KEY, "%s%ld", numeric_prefix, idx);
        }
        else {
            snprintf(key, BUFF_KEY, "%ld", idx);
        }

        if (purc_rwstream_tell(rws) > 0) {
            purc_rwstream_write(rws, &arg_separator, 1);
        }

        ret = build_query(rws, key, ov, numeric_prefix,
                arg_separator, flags);
        if (ret != PURC_ERROR_OK) {
            goto out;
        }
    }

out:
    return ret;
}

static int
build_query(purc_rwstream_t rws, const char *k, purc_variant_t v,
        const char *numeric_prefix, char arg_separator,
        unsigned int flags)
{
    enum purc_variant_type type;
    int ret = -1;
    char *key = NULL;
    size_t len_expected = 0;
    unsigned int serialize_flags;
    if (flags & PCUTILS_URL_OPT_REAL_EJSON) {
        serialize_flags = PCVARIANT_SERIALIZE_OPT_REAL_EJSON;
    }
    else {
        serialize_flags = PCVARIANT_SERIALIZE_OPT_REAL_JSON;
    }

    type = purc_variant_get_type(v);
    switch (type) {
    case PURC_VARIANT_TYPE_UNDEFINED:
    case PURC_VARIANT_TYPE_NULL:
    case PURC_VARIANT_TYPE_BOOLEAN:
    case PURC_VARIANT_TYPE_NUMBER:
    case PURC_VARIANT_TYPE_LONGINT:
    case PURC_VARIANT_TYPE_ULONGINT:
    case PURC_VARIANT_TYPE_LONGDOUBLE:
    case PURC_VARIANT_TYPE_BSEQUENCE:
    case PURC_VARIANT_TYPE_DYNAMIC:
    case PURC_VARIANT_TYPE_NATIVE:
        {
            if (k) {
                key = strdup(k);
            }
            else if (numeric_prefix) {
                key = (char*)malloc(strlen(numeric_prefix) + 2);
                sprintf(key, "%s0", numeric_prefix);
            }
            else {
                key = (char*)malloc(2);
                key[0] = '0';
                key[1] = 0;
            }

            if (!key) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto out;
            }

            encode_string(rws, key, flags);
            purc_rwstream_write(rws, "=", 1);
            ssize_t r = purc_variant_serialize(v, rws, 0, serialize_flags,
                    &len_expected);
            ret = r != -1 ? 0 : -1;
        }
        break;

    case PURC_VARIANT_TYPE_EXCEPTION:
        {
            if (k) {
                key = strdup(k);
            }
            else if (numeric_prefix) {
                key = (char*)malloc(strlen(numeric_prefix) + 2);
                sprintf(key, "%s0", numeric_prefix);
            }
            else {
                key = (char*)malloc(2);
                key[0] = '0';
            }

            if (!key) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto out;
            }

            encode_string(rws, key, flags);
            purc_rwstream_write(rws, "=", 1);
            const char *s = purc_variant_get_exception_string_const(v);
            encode_string(rws, s, flags);
        }
        break;

    case PURC_VARIANT_TYPE_ATOMSTRING:
        {
            if (k) {
                key = strdup(k);
            }
            else if (numeric_prefix) {
                key = (char*)malloc(strlen(numeric_prefix) + 2);
                sprintf(key, "%s0", numeric_prefix);
            }
            else {
                key = (char*)malloc(2);
                key[0] = '0';
                key[1] = 0;
            }

            if (!key) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto out;
            }

            encode_string(rws, key, flags);
            purc_rwstream_write(rws, "=", 1);
            const char *s = purc_variant_get_atom_string_const(v);
            encode_string(rws, s, flags);
        }
        break;

    case PURC_VARIANT_TYPE_STRING:
        {
            if (k) {
                key = strdup(k);
            }
            else if (numeric_prefix) {
                key = (char*)malloc(strlen(numeric_prefix) + 2);
                sprintf(key, "%s0", numeric_prefix);
            }
            else {
                key = (char*)malloc(2);
                key[0] = '0';
                key[1] = 0;
            }

            if (!key) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto out;
            }

            encode_string(rws, key, flags);
            purc_rwstream_write(rws, "=", 1);
            const char *s = purc_variant_get_string_const(v);
            ret = encode_string(rws, s, flags);
        }
        break;


    case PURC_VARIANT_TYPE_OBJECT:
        ret = encode_object(rws, k, v, numeric_prefix, arg_separator,
                flags);
        break;
    case PURC_VARIANT_TYPE_ARRAY:
        ret = encode_array(rws, k, v, numeric_prefix, arg_separator,
                flags);
        break;
    case PURC_VARIANT_TYPE_SET:
        ret = encode_set(rws, k, v, numeric_prefix, arg_separator,
                flags);
        break;
    case PURC_VARIANT_TYPE_TUPLE:
        ret = encode_tuple(rws, k, v, numeric_prefix, arg_separator,
                flags);
        break;
    default:
        break;
    }

out:
    if (key) {
        free(key);
    }
    return ret;
}

purc_variant_t
pcutils_url_build_query(purc_variant_t v, const char *numeric_prefix,
        char arg_separator, unsigned int flags)
{
    int err = 0;
    purc_rwstream_t rws  = NULL;
    purc_variant_t ret = PURC_VARIANT_INVALID;
    if (!v) {
        goto out;
    }

    rws = purc_rwstream_new_buffer(BUFF_MIN, 0);
    if(!rws) {
        goto out;
    }

    err = build_query(rws, NULL, v, numeric_prefix, arg_separator, flags);
    if (err == PURC_ERROR_OK) {
        size_t sz_buffer = 0;
        size_t sz_content = 0;
        char *content = NULL;
        content = (char*)purc_rwstream_get_mem_buffer_ex(rws, &sz_content,
                &sz_buffer, true);
        ret = purc_variant_make_string_reuse_buff(content, sz_buffer, false);
    }

    purc_rwstream_destroy(rws);

out:
    return ret;
}


