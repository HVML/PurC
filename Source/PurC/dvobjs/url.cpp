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
#include "purc-variant.h"
#include "wtf/URL.h"
#include "helper.h"

#ifdef __cplusplus
extern "C" {
#endif

void get_url ()
{
    std::unique_ptr<WTF::URL> url = makeUnique<URL>(URL(), "http://gengyue:hello@www.minigui.org:8080/abd.html?a=b&c=d#zzz");
    bool a = url->isValid();

    if (a) {
        String string = url->string();
        printf ("=============== string: %s\n", string.latin1().data());

        StringView protocol = url->protocol();
        printf ("=============== protocol: %s\n", protocol.toString().latin1().data());

        String user = url->user();
        printf ("=============== user: %s\n", user.latin1().data());

        String password = url->password();
        printf ("=============== password: %s\n", password.latin1().data());

        StringView host = url->host();
        printf ("=============== host: %s\n", host.toString().latin1().data());

        Optional<uint16_t> port = url->port();
        if (port) {
            printf ("================= port: %d\n", *port);
        }

        StringView path = url->path();
        printf ("=============== path: %s\n", path.toString().latin1().data());

        StringView query = url->query();
        printf ("=============== query: %s\n", query.toString().latin1().data());

        StringView fragmentIdentifier = url->fragmentIdentifier();
        printf ("=============== fragmentIdentifier: %s\n", fragmentIdentifier.toString().latin1().data());
    }
    return;
}

#ifdef __cplusplus
};
#endif
