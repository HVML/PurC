/*
 * @file parser.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of parameter lparser.
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
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"

#include "parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>

#ifdef gengyue
enum parse_state {
    PARSE_STATE_START,
    PARSE_STATE_SQUOTE,             // '\''
    PARSE_STATE_DQUOTE,             // '\"'
    PARSE_STATE_SPACE,              // ' '
    PARSE_STATE_LOGICAL_AND_START,  // '&'
    PARSE_STATE_LOGICAL_OR,         // '|'
    PARSE_STATE_GROUP,              // '[', ']'
    PARSE_STATE_PARENTHESIS,        // '(', ')'
//    PARSE_STATE_DOLLAR,             // '$'
//    PARSE_STATE_POINT,              // '.'
//    PARSE_STATE_STAR,               // '*'
    PARSE_STATE_SEMICOLON,          // ';', as space now
    PARSE_STATE_LOGICAL_ONLY,
    PARSE_STATE_TOKEN,
    PARSE_STATE_END,
    PARSE_STATE_MAX,
};

struct pcdvobjs_tokenizer;
typedef const unsigned char* (*parse_param_fn) (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end);

// 
struct pcdvobjs_tokenizer {
    parse_param_fn      parse_fn;

    int                 status[50];
    int                 current;

    const unsigned char * begin;          // token start address
    const unsigned char * end;            // token end address

    pcdvobjs_node_t     root;
};

static const unsigned char * get_start (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end);
static const unsigned char * get_space (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end);
static const unsigned char * get_token (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end);
static const unsigned char * get_logical_and (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end);
static const unsigned char * get_logical_only (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end);
static const unsigned char * get_group (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end);

static const unsigned char * get_start (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end)
{
    while (data != end) {
        switch (*data) {
            case '\'':
            case '\"':
                tkz->status = PARSE_STATE_START;
                break;
            case ' ':
                tkz->status = PARSE_STATE_SPACE;
                tkz->parse_fn = get_space;
                return ++data;
            case '[':
                tkz->status = PARSE_STATE_GROUP;
                tkz->parse_fn = get_group;
                return ++data;
            case 0x00:
                return end;
            default:
                tkz->status = PARSE_STATE_TOKEN;
                tkz->parse_fn = get_token;
                tkz->begin = data;
                return ++data;
        }
        data ++;
    }

    return data;
}

static const unsigned char * get_space (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end)
{
    while (data != end) {
        switch (*data) {
            case '\'':
            case '\"':
                tkz->status = PARSE_STATE_START;
                tkz->parse_fn = get_start;
                return ++data;
            case ' ':
                tkz->status = PARSE_STATE_SPACE;
                break;
            case '[':
                tkz->status = PARSE_STATE_GROUP;
                tkz->parse_fn = get_group;
                return ++data;
            case '&':
                tkz->status = PARSE_STATE_LOGICAL_AND_START;
                tkz->parse_fn = get_logical_and;
                return ++data;
            case '|':
                tkz->status = PARSE_STATE_LOGICAL_ONLY;
                tkz->parse_fn = get_logical_only;
                return ++data;
            case '(':
                tkz->status = PARSE_STATE_PARENTHESIS;
                tkz->parse_fn = get_parenthesis;
                return ++data;
            case 0x00:
                return end;
            default:
                tkz->status = PARSE_STATE_TOKEN;
                tkz->parse_fn = get_token;
                tkz->begin = data;
                return ++data;
        }
        data ++;
    }

    return data;
}

static const unsigned char * get_parenthesis (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end)
{
    while (data != end) {
        switch (*data) {
            case '\'':
            case '\"':
                tkz->status = PARSE_STATE_START;
                tkz->parse_fn = get_start;
                return ++data;
            case ' ':
                tkz->status = PARSE_STATE_SPACE;
                break;
            case '[':
                tkz->status = PARSE_STATE_GROUP;
                tkz->parse_fn = get_group;
                return ++data;
            case '&':
                tkz->status = PARSE_STATE_LOGICAL_AND_START;
                tkz->parse_fn = get_logical_and;
                return ++data;
            case '|':
                tkz->status = PARSE_STATE_LOGICAL_ONLY;
                tkz->parse_fn = get_logical_only;
                return ++data;
            case ')':
                tkz->status = PARSE_STATE_START;
                tkz->parse_fn = get_start;
                return ++data;
            case 0x00:
                return end;
            default:
                tkz->status = PARSE_STATE_TOKEN;
                tkz->parse_fn = get_token;
                tkz->begin = data;
                return ++data;
        }
        data ++;
    }

    return data;
}

static const unsigned char * get_token (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end)
{
    while (data != end) {
        switch (*data) {
            case ' ':
                // create a token node
                tkz->status = PARSE_STATE_SPACE;
                tkz->end = data - 1;
                return ++data;
            case '[':
                // create a group node
                tkz->status = PARSE_STATE_GROUP;
                tkz->parse_fn = get_group;
                return ++data;
            case '&':
                tkz->status = PARSE_STATE_LOGICAL_AND_START;
                tkz->parse_fn = get_logical_and;
                return ++data;
            case '|':
                tkz->status = PARSE_STATE_LOGICAL_ONLY;
                tkz->parse_fn = get_logical_only;
                return ++data;
            case '+':
                if (*(++data) == ' ') {
                }
                else if (*(++data) == '+') {
                }
                else {
                }
                break;
            case '-':
                if (*(++data) == ' ') {
                }
                else if (*(++data) == '-') {
                }
                else {
                }
                break;
            case '*'
                break;
            case '/':
                break;
            case '%':
                break;
            case 0x00:
                return end;
            default:
                tkz->status = PARSE_STATE_TOKEN;
                tkz->parse_fn = get_token;
                tkz->begin = data;
                return ++data;
        }
        data ++;
    }

    return data;
}

static const unsigned char * get_logical_and (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end)
{
    while (data != end) {
        switch (*data) {
            case '\'':
            case '\"':
                tkz->status = PARSE_STATE_START;
                break;
            case ' ':
                tkz->status = PARSE_STATE_SPACE;
                break;
            case '[':
                tkz->status = PARSE_STATE_GROUP;
                tkz->parse_fn = get_group;
                return (data + 1);
            case '&':
                tkz->status = PARSE_STATE_LOGICAL_AND_START;
                tkz->parse_fn = get_logical_and;
                return (data + 1);
            case '|':
                tkz->status = PARSE_STATE_LOGICAL_ONLY;
                tkz->parse_fn = get_logical_only;
                return (data + 1);
            case 0x00:
                return end;
            default:
                tkz->status = PARSE_STATE_TOKEN;
                tkz->parse_fn = get_token;
                tkz->begin = data;
                return ++data;
        }
        data ++;
    }

    return data;
}

static const unsigned char * get_logical_only (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end)
{
    while (data != end) {
        switch (*data) {
            case '\'':
            case '\"':
                tkz->status = PARSE_STATE_START;
                break;
            case ' ':
                tkz->status = PARSE_STATE_SPACE;
                break;
            case '[':
                tkz->status = PARSE_STATE_GROUP;
                tkz->parse_fn = get_group;
                return (data + 1);
            case '&':
                tkz->status = PARSE_STATE_LOGICAL_AND_START;
                tkz->parse_fn = get_logical_and;
                return ++data;
            case '|':
                tkz->status = PARSE_STATE_LOGICAL_ONLY;
                tkz->parse_fn = get_logical_only;
                return (data + 1);
            case 0x00:
                return end;
            default:
                tkz->status = PARSE_STATE_TOKEN;
                tkz->parse_fn = get_token;
                tkz->begin = data;
                return ++data;
        }
        data ++;
    }

    return data;
}

static const unsigned char * get_group (struct pcdvobjs_tokenizer * tkz,
                    const unsigned char * data, const unsigned char * end)
{
    while (data != end) {
        switch (*data) {
            case '\'':
            case '\"':
                tkz->status = PARSE_STATE_START;
                break;
            case ' ':
                tkz->status = PARSE_STATE_SPACE;
                break;
            case '[':
                tkz->status = PARSE_STATE_GROUP;
                tkz->parse_fn = get_group;
                return (data + 1);
            case '&':
                tkz->status = PARSE_STATE_LOGICAL_AND_START;
                tkz->parse_fn = get_logical_and;
                return (data + 1);
            case '|':
                tkz->status = PARSE_STATE_LOGICAL_ONLY;
                tkz->parse_fn = get_logical_only;
                return (data + 1);
            case 0x00:
                return end;
            default:
                tkz->status = PARSE_STATE_TOKEN;
                tkz->parse_fn = get_token;
                tkz->begin = data;
                return ++data;
        }
        data ++;
    }

    return data;
}

pcdvobjs_node_t pcdvobjs_parse (const unsigned char * data)
{
    size_t length = strlen ((char *)data);
    const unsigned char * end = data + length;
    struct pcdvobjs_tokenizer tkz;

    tkz.parse_fn = get_start;
    tkz.current = 0;
    tkz.status[current] = PARSE_STATE_START;
    tkz.begin = NULL;
    tkz.end = NULL;
    tkz.root = NULL;

    while (data < end) {
        data = tkz.parse_fn (&tkz, data, end); 
    }

    return tkz.root;
}

bool destroy_tree (pcdvobjs_node_t root)
{
    UNUSED_PARAM (root);
    return true;
}

#endif
