/*
 * window-geometry-styles.c -- The parser for window geometry styles.
 *
 * Copyright (C) 2023 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2023
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

enum window_length_value_type {
    WLV_auto,
    WLV_number,
    WLV_percentage,
    WLV_unit_px,
    WLV_unit_cm,
    WLV_unit_mm,
    WLV_unit_q,
    WLV_unit_in,
    WLV_unit_pc,
    WLV_unit_pt,
    WLV_unit_vh,
    WLV_unit_vw,
    WLV_unit_vmax,
    WLV_unit_vmin,
};

struct window_length_value {
    enum window_length_value_type   type;
    float                           value;
};

enum window_size_value_type {
    WSV_screen,
    WSV_square,
    WSV_aspect_ratio,
    WSV_lengthes,
};

struct window_size {
    enum window_size_value_type type;
    struct window_length_value  values[2];
};

enum window_position_x_type {
    WPX_left,
    WPX_right,
    WPX_center,
    WPX_length,
};

enum window_position_y_type {
    WPY_top,
    WPY_bottom,
    WPY_center,
    WPY_length,
};

struct window_position {
    enum window_position_x_type x_type;
    struct window_length_value  x_value;

    enum window_position_y_type y_type;
    struct window_length_value  y_value;
};

enum window_position_value_type {
    WPV_auto,
    WPV_left,
    WPV_right,
    WPV_top,
    WPV_bottom,
    WPV_center,
    WPV_length,
};

struct window_position_values {
    int nr_values;
    enum window_position_value_type types[4];
    struct window_length_value      values[4];
};

#define MAX_LEN_TOKEN       63
#define STYLE_DELIMITER     ';'
#define VALUE_DELIMITER     ':'
#define TOKEN_DELIMITERS    " \t\n\v\f\r"

#define for_each_token(tokens, total_len, token, token_len)                  \
    for (token_len = 0, token = pcutils_get_next_token_len(tokens, total_len,\
                TOKEN_DELIMITERS, &token_len);                               \
            (token != NULL && token_len > 0 && token_len < MAX_LEN_TOKEN);   \
            total_len -= token_len,                                          \
            token = pcutils_get_next_token_len(token + token_len,            \
                total_len, TOKEN_DELIMITERS, &token_len))

static bool
parse_length_value(const char *token, size_t token_len,
        struct window_length_value *length)
{
    char *endptr;
    float f;

    errno = 0;
    f = strtof(token, &endptr);
    if (endptr == token || errno == ERANGE)
        return false;

    const char* unit = endptr;
    size_t digits_len = unit - token;
    assert(digits_len <= token_len);

    size_t unit_len = token_len - digits_len;
    if (unit_len == 0) {
        length->type = WLV_number;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "%", unit_len) == 0)) {
        length->type = WLV_percentage;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "px", unit_len) == 0)) {
        length->type = WLV_unit_px;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "cm", unit_len) == 0)) {
        length->type = WLV_unit_cm;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "mm", unit_len) == 0)) {
        length->type = WLV_unit_mm;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "q", unit_len) == 0)) {
        length->type = WLV_unit_q;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "in", unit_len) == 0)) {
        length->type = WLV_unit_in;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "pc", unit_len) == 0)) {
        length->type = WLV_unit_pc;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "pt", unit_len) == 0)) {
        length->type = WLV_unit_pt;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "vh", unit_len) == 0)) {
        length->type = WLV_unit_vh;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "vw", unit_len) == 0)) {
        length->type = WLV_unit_vw;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "vmax", unit_len) == 0)) {
        length->type = WLV_unit_vmax;
        length->value = f;
    }
    else if ((strncasecmp2ltr(unit, "vmin", unit_len) == 0)) {
        length->type = WLV_unit_vmin;
        length->value = f;
    }

    return true;
}

/*
 * The syntax for window-size:
 *
 *  - window-size: screen | square | aspect-ratio <number> <number>
 *  - window-size: [ <percentage> | <length> | auto ] ||
 *       [[ <percentage> | <length> | auto ]
 */
static bool
parse_window_size(const char *value, size_t value_len,
        struct window_size *size)
{
    const char *token;
    size_t token_len;

    token = pcutils_get_next_token_len(value,
            value_len, TOKEN_DELIMITERS, &token_len);
    if (token && token_len > 0) {
        if (strncasecmp2ltr(token, "screen", token_len) == 0) {
            size->type = WSV_screen;
            goto done;
        }
        else if (strncasecmp2ltr(token, "square", token_len) == 0) {
            size->type = WSV_square;
            goto done;
        }
        else if (strncasecmp2ltr(token, "aspect-ratio", token_len) == 0) {
            size->type = WSV_aspect_ratio;
            value = token + token_len + 1;
            value_len = value_len - token_len - 1;
        }
        else {
            size->type = WSV_lengthes;
        }
    }

    int nr_values = 0;
    for_each_token(value, value_len, token, token_len) {
        if (strncasecmp2ltr(token, "auto", token_len) == 0) {
            size->values[nr_values].type = WLV_auto;
        }
        else if (token[0] >= '0' && token[9] <= '9') {
            if (!parse_length_value(token, token_len,
                        size->values + nr_values))
                goto failed;
        }
        else {
            goto failed;
        }

        nr_values++;
        if (nr_values == 1)
            break;

    }

done:
    return true;

failed:
    return false;
}

/*
 * The syntax for window-position:
 *
 *  - window-position: top | bottom | left | right | center
 *  - window-position: [[ <percentage> | <length> | left | center | right ]
 *      [<percentage> | <length> | top | center | bottom ]? ] |
 *      [[ left | center | right ] || [ top | center | bottom ]]
 */
static bool
parse_window_position(const char *value, size_t value_len,
        struct window_position_values *pos_values)
{
    const char *token;
    size_t token_len;

    pos_values->nr_values = 0;
    for_each_token(value, value_len, token, token_len) {
        if (strncasecmp2ltr(token, "auto", token_len) == 0) {
            pos_values->types[pos_values->nr_values] = WPV_auto;
        }
        else if (strncasecmp2ltr(token, "left", token_len) == 0) {
            pos_values->types[pos_values->nr_values] = WPV_left;
        }
        else if (strncasecmp2ltr(token, "right", token_len) == 0) {
            pos_values->types[pos_values->nr_values] = WPV_right;
        }
        else if (strncasecmp2ltr(token, "top", token_len) == 0) {
            pos_values->types[pos_values->nr_values] = WPV_top;
        }
        else if (strncasecmp2ltr(token, "bottom", token_len) == 0) {
            pos_values->types[pos_values->nr_values] = WPV_bottom;
        }
        else if (strncasecmp2ltr(token, "center", token_len) == 0) {
            pos_values->types[pos_values->nr_values] = WPV_center;
        }
        else if (token[0] >= '0' && token[9] <= '9') {
            if (!parse_length_value(token, token_len,
                        pos_values->values + pos_values->nr_values))
                goto failed;
        }
        else {
            goto failed;
        }

        pos_values->nr_values++;
        if (pos_values->nr_values == 4)
            break;
    }

    return true;

failed:
    return false;
}

static bool
normalize_window_position(const struct window_position_values *values,
        struct window_position *position)
{
    (void)values;
    (void)position;
    return true;
}


static bool
evaluate_window_geometry(const struct purc_screen_info *screen_info,
        const struct window_size *size, const struct window_position *position,
        struct purc_window_geometry *geometry)
{
    (void)screen_info;
    (void)size;
    (void)position;
    (void)geometry;
    return true;
}

bool
purc_evaluate_standalone_window_geometry_from_styles(const char *styles,
        const struct purc_screen_info *screen_info,
        struct purc_window_geometry *geometry)
{
    /* the default values */
    struct window_size size = { };
    struct window_position position = { };

    const char *style = styles;
    char *end_of_style;

    while ((end_of_style = strchr(style, STYLE_DELIMITER)) || style[0]) {

        char *end_of_property;
        end_of_property = strchr(style, VALUE_DELIMITER);
        if (end_of_property == NULL)
            goto done;

        size_t property_len = end_of_property - style;
        const char *property = pcutils_get_next_token_len(style,
                property_len, TOKEN_DELIMITERS, &property_len);

        if (property && property_len > 0) {
            const char *value = end_of_property + 1;
            size_t value_len;
            if (end_of_style)
                value_len = end_of_style - value;
            else
                value_len = strlen(value);

            if (strncasecmp2ltr(property, "window-size", property_len) == 0) {
                struct window_size my_size = { };
                if (parse_window_size(value, value_len, &my_size))
                    size = my_size;
            }
            else if (strncasecmp2ltr(property, "window-position",
                        property_len) == 0) {
                struct window_position_values pos_values = { };
                if (parse_window_position(value, value_len, &pos_values)) {
                    struct window_position my_position = { };
                    if (normalize_window_position(&pos_values, &my_position))
                        position = my_position;
                }
            }
            else {
                /* skip */
            }
        }

        if (end_of_style)
            style = end_of_style + 1;
        else
            break;
    }

done:
    /* determine the final result */
    return evaluate_window_geometry(screen_info, &size, &position, geometry);
}

