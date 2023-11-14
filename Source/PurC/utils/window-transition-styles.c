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

static struct transition_funcs {
    const char *name;
    purc_window_transition_function function;
} trans_funcs[] = {
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_COSINECURVE,
        PURC_WINDOW_TRANSITION_FUNCTION_COSINECURVE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INBACK,
        PURC_WINDOW_TRANSITION_FUNCTION_INBACK
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INBOUNCE,
        PURC_WINDOW_TRANSITION_FUNCTION_INBOUNCE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INCIRC,
        PURC_WINDOW_TRANSITION_FUNCTION_INCIRC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INCUBIC,
        PURC_WINDOW_TRANSITION_FUNCTION_INCUBIC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INCURVE,
        PURC_WINDOW_TRANSITION_FUNCTION_INCURVE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INELASTIC,
        PURC_WINDOW_TRANSITION_FUNCTION_INELASTIC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INEXPO,
        PURC_WINDOW_TRANSITION_FUNCTION_INEXPO
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTBACK,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTBACK
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTBOUNCE,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTBOUNCE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTCIRC,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTCIRC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTCUBIC,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTCUBIC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTELASTIC,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTELASTIC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTEXPO,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTEXPO
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTQUAD,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTQUART,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUART
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTQUINT,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUINT
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTSINE,
        PURC_WINDOW_TRANSITION_FUNCTION_INOUTSINE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INQUAD,
        PURC_WINDOW_TRANSITION_FUNCTION_INQUAD
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INQUART,
        PURC_WINDOW_TRANSITION_FUNCTION_INQUART
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INQUINT,
        PURC_WINDOW_TRANSITION_FUNCTION_INQUINT
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_INSINE,
        PURC_WINDOW_TRANSITION_FUNCTION_INSINE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_LINEAR,
        PURC_WINDOW_TRANSITION_FUNCTION_LINEAR
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_NONE,
        PURC_WINDOW_TRANSITION_FUNCTION_NONE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTBACK,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTBACK
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTBOUNCE,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTBOUNCE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTCIRC,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTCIRC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTCUBIC,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTCUBIC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTCURVE,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTCURVE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTELASTIC,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTELASTIC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTEXPO,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTEXPO
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINBACK,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINBACK
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINBOUNCE,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINBOUNCE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINCIRC,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINCIRC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINCUBIC,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINCUBIC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINELASTIC,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINELASTIC
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINEXPO,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINEXPO
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINQUAD,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINQUAD
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINQUART,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINQUART
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINQUINT,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINQUINT
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINSINE,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTINSINE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTQUAD,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTQUART,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTQUART
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTQUINT,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTQUINT
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTSINE,
        PURC_WINDOW_TRANSITION_FUNCTION_OUTSINE
    },
    {
        PURC_WINDOW_TRANSITION_FUNCTION_NAME_SINECURVE,
        PURC_WINDOW_TRANSITION_FUNCTION_SINECURVE
    },
};

/* Make sure the number of handlers matches the number of operations */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(hdl,
    sizeof(trans_funcs)/sizeof(trans_funcs[0]) == PURC_NR_WINDOW_TRANSITION_FUNCTION);
#undef _COMPILE_TIME_ASSERT


static purc_window_transition_function
find_transition_function(const char* operation)
{
    static ssize_t max = sizeof(trans_funcs)/sizeof(trans_funcs[0]) - 1;

    ssize_t low = 0, high = max, mid;
    while (low <= high) {
        int cmp;

        mid = (low + high) / 2;
        cmp = strcasecmp(operation, trans_funcs[mid].name);
        if (cmp == 0) {
            goto found;
        }
        else {
            if (cmp < 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
    }

    return PURC_WINDOW_TRANSITION_FUNCTION_NONE;

found:
    return trans_funcs[mid].function;
}

static bool
parse_window_move(const char *value, size_t value_len,
        struct purc_window_transition *transition)
{
    const char *token;
    size_t token_len;

    token = pcutils_get_next_token_len(value,
            value_len, TOKEN_DELIMITERS, &token_len);
    if (token && token_len > 0) {
        char name[token_len + 1];
        strncpy(name, token, token_len);
        name[token_len] = 0;
        transition->move_func = find_transition_function(name);
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
            transition->move_func = PURC_WINDOW_TRANSITION_FUNCTION_NONE;
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

