/*
 * @file tokenizer.c
 * @author Xue Shuming
 * @date 2022/02/08
 * @brief The implementation of hvml parser.
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
 *
 */

#include "config.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dom.h"
#include "private/hvml.h"
#include "private/tkz-helper.h"
#include "private/ejson.h"

#include "hvml-token.h"
#include "hvml-attr.h"
#include "hvml-tag.h"

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#include "hvml_err_msgs.inc"

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(hvml_err_msgs) == PCHVML_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _hvml_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_HVML,
    PURC_ERROR_FIRST_HVML + PCA_TABLESIZE(hvml_err_msgs) - 1,
    hvml_err_msgs
};

static int hvml_init_once(void)
{
    pcinst_register_error_message_segment(&_hvml_err_msgs_seg);
    return 0;
}

struct pcmodule _module_hvml = {
    .id              = PURC_HAVE_HVML,
    .module_inited   = 0,

    .init_once       = hvml_init_once,
    .init_instance   = NULL,
};


#define PURC_ENVV_HVML_LOG_ENABLE   "PURC_HVML_LOG_ENABLE"
#define EJSON_PARSER_MAX_DEPTH      512
#define EJSON_PARSER_FLAGS          1
#define HVML_PARSER_LC_SIZE         3

struct pchvml_parser* pchvml_create(uint32_t flags, size_t queue_size,
        purc_rwstream_t rws)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(queue_size);

    struct pchvml_parser* parser = (struct pchvml_parser*) PCHVML_ALLOC(
            sizeof(struct pchvml_parser));
    parser->state = 0;
    parser->reader = tkz_reader_new();
    tkz_reader_set_data_source_rws(parser->reader, rws);
    parser->lc = tkz_lc_new (HVML_PARSER_LC_SIZE);
    tkz_reader_set_lc(parser->reader, parser->lc);
    parser->temp_buffer = tkz_buffer_new ();
    parser->tag_name = tkz_buffer_new ();
    parser->string_buffer = tkz_buffer_new ();
    parser->temp_ucs = tkz_ucs_new();
    parser->ejson_stack = pcutils_stack_new(0);
    parser->char_ref_code = 0;
    parser->prev_separator = 0;
    parser->nr_single_quoted = 0;
    parser->nr_double_quoted = 0;
    parser->tag_is_operation = 0;
    parser->tag_has_raw_attr = 0;
    parser->is_in_file_header = 1;
    parser->ejson_parser_max_depth = EJSON_PARSER_MAX_DEPTH;
    parser->ejson_parser_flags = PCEJSON_FLAG_ALL;
    parser->ejson_parser = pcejson_create(parser->ejson_parser_max_depth,
            parser->ejson_parser_flags);

    const char *env_value = getenv(PURC_ENVV_HVML_LOG_ENABLE);
    parser->enable_log = ((env_value != NULL) &&
            (*env_value == '1' || pcutils_strcasecmp(env_value, "true") == 0));

    return parser;
}

void pchvml_reset(struct pchvml_parser* parser, uint32_t flags,
        size_t queue_size)
{
    UNUSED_PARAM(flags);
    UNUSED_PARAM(queue_size);

    parser->state = 0;
    tkz_reader_destroy (parser->reader);
    parser->reader = tkz_reader_new();
    tkz_lc_reset (parser->lc);
    tkz_reader_set_lc(parser->reader, parser->lc);
    tkz_buffer_reset (parser->temp_buffer);
    tkz_buffer_reset (parser->tag_name);
    tkz_buffer_reset (parser->string_buffer);
    tkz_ucs_reset (parser->temp_ucs);

    pcutils_stack_destroy(parser->ejson_stack);
    parser->ejson_stack = pcutils_stack_new(0);
    if (parser->token) {
        pchvml_token_destroy(parser->token);
        parser->token = NULL;
    }
    parser->char_ref_code = 0;
    parser->prev_separator = 0;
    parser->nr_single_quoted = 0;
    parser->nr_double_quoted = 0;
    parser->tag_is_operation = false;
    parser->tag_has_raw_attr = false;
    pcejson_reset(parser->ejson_parser, parser->ejson_parser_max_depth,
            parser->ejson_parser_flags);
}

void pchvml_destroy(struct pchvml_parser* parser)
{
    if (parser) {
        tkz_reader_destroy (parser->reader);
        tkz_lc_destroy (parser->lc);
        tkz_buffer_destroy (parser->temp_buffer);
        tkz_buffer_destroy (parser->tag_name);
        tkz_buffer_destroy (parser->string_buffer);
        tkz_ucs_destroy (parser->temp_ucs);
        if (parser->sbst) {
            tkz_sbst_destroy(parser->sbst);
        }
        pcutils_stack_destroy(parser->ejson_stack);
        if (parser->token) {
            pchvml_token_destroy(parser->token);
        }
        pcejson_destroy(parser->ejson_parser);
        PCHVML_FREE(parser);
    }
}


