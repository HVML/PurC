/*
 * window-transition-styles.c -- The parser for window transition styles.
 *
 * Copyright (C) 2023 FMSoft (http://www.fmsoft.cn)
 *
 * Authors: XueShuming 2023
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

#include "config.h"
#include "purc/purc-helpers.h"
#include "private/utils.h"
#include "private/debug.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#define MAX_LEN_TOKEN       63
#define STYLE_DELIMITER     ';'
#define VALUE_DELIMITER     ':'
#define TOKEN_DELIMITERS    " \t\n\v\f\r"

#define WINDOW_TRANSITION_MOVE  "window-transition-move"

static bool
parse_window_move(const char *value, size_t value_len,
        struct purc_window_transition *transition)
{
    const char *token;
    size_t token_len;

    token = pcutils_get_next_token_len(value,
            value_len, TOKEN_DELIMITERS, &token_len);
    if (token && token_len > 0) {
        if (strncasecmp2ltr(token,
                    PURC_WINDOW_TRANSTION_FUNCTION_NAME_NONE, token_len) == 0) {
            transition->move_func = PURC_WINDOW_TRANSTION_FUNCTION_NONE;
        }
        else if (strncasecmp2ltr(token,
                    PURC_WINDOW_TRANSTION_FUNCTION_NAME_LINEAR, token_len) == 0) {
            transition->move_func = PURC_WINDOW_TRANSTION_FUNCTION_LINEAR;
        }
        else if (strncasecmp2ltr(token,
                    PURC_WINDOW_TRANSTION_FUNCTION_NAME_EASY, token_len) == 0) {
            transition->move_func = PURC_WINDOW_TRANSTION_FUNCTION_EASY;
        }
        else if (strncasecmp2ltr(token,
                    PURC_WINDOW_TRANSTION_FUNCTION_NAME_EASY_IN, token_len) == 0) {
            transition->move_func = PURC_WINDOW_TRANSTION_FUNCTION_EASY_IN;
        }
        else if (strncasecmp2ltr(token,
                    PURC_WINDOW_TRANSTION_FUNCTION_NAME_EASY_OUT, token_len) == 0) {
            transition->move_func = PURC_WINDOW_TRANSTION_FUNCTION_EASY_OUT;
        }
        else {
            transition->move_func = PURC_WINDOW_TRANSTION_FUNCTION_NONE;
        }
    }
    else {
        goto failed;
    }

    token = pcutils_get_next_token_len(token + token_len,
            value_len - token_len, TOKEN_DELIMITERS, &token_len);
    if (token && token_len > 0) {
        char buf[token_len + 1];
        strncpy(buf, token, token_len);
        unsigned long val = strtoul(buf, NULL, 10);
        if (val == ULONG_MAX) {
            transition->move_func = PURC_WINDOW_TRANSTION_FUNCTION_NONE;
            transition->move_duration = 0;
            goto failed;
        }
        transition->move_duration = val;
    }
    else {
        goto failed;
    }

    return true;

failed:
    return false;
}

int
purc_evaluate_standalone_window_transition_from_styles(const char *styles,
      struct purc_window_transition *transition)
{
    struct purc_window_transition trans = { };
    const char *style = styles;
    char *end_of_style;

    if (styles && styles[0] == 0) {
        *transition = trans;
        goto done;
    }

    while ((end_of_style = strchr(style, STYLE_DELIMITER)) || style[0]) {

        char *end_of_property;
        end_of_property = strchr(style, VALUE_DELIMITER);
        if (end_of_property == NULL) {
            goto done;
        }

        size_t property_len = end_of_property - style;
        const char *property = pcutils_get_next_token_len(style,
                property_len, TOKEN_DELIMITERS, &property_len);

        if (property && property_len > 0) {
            const char *value = end_of_property + 1;
            size_t value_len;
            if (end_of_style) {
                value_len = end_of_style - value;
            }
            else {
                value_len = strlen(value);
            }

            if (strncasecmp2ltr(property, WINDOW_TRANSITION_MOVE,
                        property_len) == 0) {
                if (!parse_window_move(value, value_len, transition)) {
                    goto failed;
                }
            }
            else {
                /* skip */
            }
        }

        if (end_of_style) {
            style = end_of_style + 1;
        }
        else {
            break;
        }
    }

done:
    return 0;

failed:
    return -1;
}

