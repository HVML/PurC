/*
 * @file ejson.h
 * @author XueShuming
 * @date 2021/07/19
 * @brief The interfaces eJSON parser.
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

#ifndef PURC_PRIVATE_EJSON_H
#define PURC_PRIVATE_EJSON_H

#include "purc-rwstream.h"
#include "private/stack.h"
#include "private/vcm.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PCEJSON_DEFAULT_DEPTH 32

#define PCEJSON_FLAG_NONE               0x0000
#define PCEJSON_FLAG_MULTI_JSONEE       0x0001
#define PCEJSON_FLAG_GET_VARIABLE       0x0002
#define PCEJSON_FLAG_KEEP_LAST_CHAR     0x0004
#define PCEJSON_FLAG_ALL                0xFFFF

struct pcejson;
struct tkz_reader;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * Create ejson parser.
 */
struct pcejson* pcejson_create (uint32_t depth, uint32_t flags);

/*
 * Destroy ejson parser.
 */
void pcejson_destroy (struct pcejson* parser);

/*
 * Reset ejson parser.
 */
void pcejson_reset (struct pcejson* parser, uint32_t depth, uint32_t flags);

/*
 * Parse ejson.
 */
int pcejson_parse (struct pcvcm_node** vcm_tree, struct pcejson** parser,
                   purc_rwstream_t rwstream, uint32_t depth);

typedef bool (*pcejson_parse_is_finished_fn)(struct pcejson *parser,
        uint32_t character);
int pcejson_parse_full (struct pcvcm_node** vcm_tree, struct pcejson** parser,
                   struct tkz_reader *reader, uint32_t depth,
                   pcejson_parse_is_finished_fn is_finished);

int pcejson_set_state(struct pcejson *parser, int state);

int pcejson_set_state_param_string(struct pcejson *parser);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_EJSON_H */

