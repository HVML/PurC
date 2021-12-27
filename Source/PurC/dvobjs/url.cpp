/*
 * @file t.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of T dynamic variant object.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/dvobjs.h"
#include "private/vdom.h"
#include "purc-variant.h"
#include "wtf/URL.h"
#include "helper.h"

#ifdef __cplusplus
extern "C" {
#endif

char * pcdvobjs_get_url (const struct purc_broken_down_url *url_struct)
{
    char * url_string = NULL;
    String string = "";
    std::unique_ptr<WTF::URL> url = makeUnique<URL>(URL(), string);

    if (url_struct->schema)
        url->setProtocol (url_struct->schema);

    if (url_struct->host)
        url->setHost (url_struct->host);

    if (url_struct->port)
        url->setPort (url_struct->port);

    if (url_struct->path)
        url->setPath (url_struct->path);

    if (url_struct->query)
        url->setQuery (url_struct->query);

    if (url_struct->fragment)
        url->setFragmentIdentifier (url_struct->fragment);

    if (url_struct->user)
        url->setUser (url_struct->user);

    if (url_struct->passwd)
        url->setPassword (url_struct->passwd);

    if (url->isValid()) {
        String tempstring = url->string();
        url_string = strdup (tempstring.latin1().data());
    }

    return url_string;
}

bool pcdvobjs_set_url (struct purc_broken_down_url *url_struct, const char *url_string)
{
    std::unique_ptr<WTF::URL> url = makeUnique<URL>(URL(), url_string);
    bool valid = url->isValid();
    size_t length = 0;
    const char *tempstring = NULL;

    if (valid) {
        if (url_struct->schema)
            free (url_struct->schema);
        StringView protocol = url->protocol();
        tempstring = protocol.toString().latin1().data();
        length = strlen (tempstring);
        if (length)
            url_struct->schema = strdup (tempstring);
        else
            url_struct->schema = NULL;

        if (url_struct->user)
            free (url_struct->user);
        String user = url->user();
        tempstring = user.latin1().data();
        length = strlen (tempstring);
        if (length)
            url_struct->user = strdup (tempstring);
        else
            url_struct->user = NULL;

        if (url_struct->passwd)
            free (url_struct->passwd);
        String password = url->password();
        tempstring = password.latin1().data();
        length = strlen (tempstring);
        if (length)
            url_struct->passwd = strdup (tempstring);
        else
            url_struct->passwd = NULL;

        if (url_struct->host)
            free (url_struct->host);
        StringView host = url->host();
        tempstring = host.toString().latin1().data();
        length = strlen (tempstring);
        if (length)
            url_struct->host = strdup (tempstring);
        else
            url_struct->host = NULL;

        if (url_struct->path)
            free (url_struct->path);
        StringView path = url->path();
        tempstring = path.toString().latin1().data();
        length = strlen (tempstring);
        if (length)
            url_struct->path = strdup (tempstring);
        else
            url_struct->path = NULL;

        if (url_struct->query)
            free (url_struct->query);
        StringView query = url->query();
        tempstring = query.toString().latin1().data();
        length = strlen (tempstring);
        if (length)
            url_struct->query = strdup (tempstring);
        else
            url_struct->query = NULL;

        if (url_struct->fragment)
            free (url_struct->fragment);
        StringView fragmentIdentifier = url->fragmentIdentifier();
        tempstring = fragmentIdentifier.toString().latin1().data();
        length = strlen (tempstring);
        if (length)
            url_struct->fragment = strdup (tempstring);
        else
            url_struct->fragment = NULL;

        Optional<uint16_t> port = url->port();
        if (port)
            url_struct->port = (unsigned int) (*port);
        else
            url_struct->port = 0;
    }

    return valid;
}

#ifdef __cplusplus
};
#endif
