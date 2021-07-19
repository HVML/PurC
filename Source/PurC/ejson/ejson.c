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
static inline UNUSED_FUNCTION struct pcejson* alloc_pcejson(void) {
    return (struct pcejson*)g_slice_alloc(sizeof(struct pcejson));
}

static inline UNUSED_FUNCTION struct pcejson* alloc_pcejson_0(void) {
    return (struct pcejson*)g_slice_alloc0(sizeof(struct pcejson));
}

static inline void free_pcejson(struct pcejson* v) {
    return g_slice_free1(sizeof(struct pcejson), (gpointer)v);
}
#else
static inline UNUSED_FUNCTION struct pcejson* alloc_pcejson(void) {
    return (struct pcejson*)malloc(sizeof(struct pcejson));
}

static inline UNUSED_FUNCTION struct pcejson* alloc_pcejson_0(void) {
    return (struct pcejson*)calloc(1, sizeof(struct pcejson));
}

static inline void free_pcejson(struct pcejson* v) {
    return free(v);
}
#endif

struct pcejson* pcejson_create(int32_t depth, uint32_t flags)
{
    struct pcejson* parser = alloc_pcejson_0();
    parser->state = ejson_init_state;
    parser->depth = depth;
    parser->flags = flags;
    return parser;
}

void pcejson_destroy(struct pcejson* parser)
{
    free_pcejson(parser);
}

void pcejson_reset(struct pcejson* parser, int32_t depth, uint32_t flags)
{
    parser->state = ejson_init_state;
    parser->depth = depth;
    parser->flags = flags;
}
