/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc/purc.h"
#include "private/hvml.h"
#include "private/utils.h"
#include "private/tkz-helper.h"
#include "purc/purc-rwstream.h"
#include "hvml/hvml-token.h"

#include <gtest/gtest.h>


TEST(hvml, basic)
{
    const char* hvml = "<hvml><head a='b'/></hvml>";

    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_init", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    size_t sz = strlen (hvml);
    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)hvml, sz);

    struct pchvml_parser* parser = pchvml_create(0, 32, rws);

    struct tkz_buffer* buffer = tkz_buffer_new();

    struct pchvml_token* token = NULL;
    while((token = pchvml_next_token(parser)) != NULL) {
        struct tkz_buffer* token_buff = pchvml_token_to_string(token);
        if (token_buff) {
            tkz_buffer_append_another(buffer, token_buff);
            tkz_buffer_destroy(token_buff);
        }
        enum pchvml_token_type type = pchvml_token_get_type(token);
        pchvml_token_destroy(token);
        token = NULL;
        if (type == PCHVML_TOKEN_EOF) {
            break;
        }
    }

    const char* serial = tkz_buffer_get_bytes(buffer);
    fprintf(stderr, "input: [%s]\n", hvml);
    fprintf(stderr, "parsed: [%s]\n", serial);

    purc_rwstream_destroy(rws);
    tkz_buffer_destroy(buffer);
    pchvml_destroy(parser);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

