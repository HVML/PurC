/*
 * @file ejson.c
 * @author XueShuming
 * @date 2021/07/19
 * @brief The impl for eJSON parser
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

#include "private/ejson.h"
#include "private/errors.h"
#include "config.h"

#if HAVE(GLIB)
#include <gmodule.h>
#endif

#if HAVE(GLIB)
#define    ejson_alloc(sz)   g_slice_alloc0(sz)
#define    ejson_free(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    ejson_alloc(sz)   calloc(1, sz)
#define    ejson_free(p)     free(p)
#endif

struct pcejson* pcejson_create(int32_t depth, uint32_t flags)
{
    struct pcejson* parser = (struct pcejson*)ejson_alloc(sizeof(struct pcejson));
    parser->state = ejson_init_state;
    parser->depth = depth;
    parser->flags = flags;
    return parser;
}

void pcejson_destroy(struct pcejson* parser)
{
    ejson_free(parser);
}

void pcejson_reset(struct pcejson* parser, int32_t depth, uint32_t flags)
{
    parser->state = ejson_init_state;
    parser->depth = depth;
    parser->flags = flags;
}

// TODO
int pcejson_parse(pcvcm_tree_t vcm_tree, purc_rwstream_t rwstream)
{
    UNUSED_PARAM(vcm_tree);
    UNUSED_PARAM(rwstream);
    pcinst_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return -1;
}

// eJSON tokenizer
struct pcejson_token* pcejson_token_new(enum ejson_token_type type,
        size_t sz_min, size_t sz_max)
{
    struct pcejson_token* token = ejson_alloc(sizeof(struct pcejson_token));
    token->type = type;
    token->rws = purc_rwstream_new_buffer(sz_min, sz_max);
    return token;
}

void pcejson_token_destroy(struct pcejson_token* token)
{
    if (token->rws) {
        purc_rwstream_destroy(token->rws);
    }
    ejson_free(token);
}
