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
#include "private/debug.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#define MAX_NR_SIZE_LENGTHES    2
#define MAX_NR_POS_VALUES       4

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
    int                         nr_lengthes;
    struct window_length_value  values[MAX_NR_SIZE_LENGTHES];
};

enum window_position_x_type {
    WPX_left,
    WPX_right,
    WPX_center,
    WPX_left_offset,
    WPX_right_offset,
    WPX_center_offset,
    WPX_length,
};

enum window_position_y_type {
    WPY_top,
    WPY_bottom,
    WPY_center,
    WPY_top_offset,
    WPY_bottom_offset,
    WPY_center_offset,
    WPY_length,
};

struct window_position {
    enum window_position_x_type x_type;
    struct window_length_value  x_value;

    enum window_position_y_type y_type;
    struct window_length_value  y_value;
};

enum window_position_value_type {
    // WPV_auto,
    WPV_left,
    WPV_right,
    WPV_top,
    WPV_bottom,
    WPV_center,
    WPV_length,
};

struct window_position_values {
    int nr_values;
    enum window_position_value_type types[MAX_NR_POS_VALUES];
    struct window_length_value      values[MAX_NR_POS_VALUES];
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
    PC_DEBUG("token: %s (%f), unit length: %u\n", token, f, (unsigned)unit_len);

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
    else {
        return false;
    }

    return true;
}

static bool
like_a_nonnegative_real_number(const char *token, size_t token_len)
{
    if (token[0] >= '0' && token[0] <= '9')
        return true;

    if (token_len > 1 && token[0] == '+' &&
            token[1] >= '0' && token[1] <= '9')
        return true;

    return false;
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

    size->nr_lengthes = 0;
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

    for_each_token(value, value_len, token, token_len) {
        PC_DEBUG("token: %s, token_len: %u\n", token, (unsigned)token_len);
        if (strncasecmp2ltr(token, "auto", token_len) == 0) {
            size->values[size->nr_lengthes].type = WLV_auto;
        }
        else if (like_a_nonnegative_real_number(token, token_len)) {
            if (!parse_length_value(token, token_len,
                        size->values + size->nr_lengthes))
                goto failed;
        }
        else {
            goto failed;
        }

        size->nr_lengthes++;
        if (size->nr_lengthes == MAX_NR_SIZE_LENGTHES)
            break;
    }

    if (size->nr_lengthes == 1) {
        if (size->type == WSV_aspect_ratio) {
            size->values[size->nr_lengthes].type = WLV_number;
            size->values[size->nr_lengthes].value = 1.0;
        }
        else {
            size->values[size->nr_lengthes].type = WLV_auto;
        }
    }

done:
    return true;

failed:
    return false;
}

static bool
like_a_real_number(const char *token, size_t token_len)
{
    if (token[0] >= '0' && token[0] <= '9')
        return true;

    if (token_len > 1 && (token[0] == '+' || token[0] == '-') &&
            token[1] >= '0' && token[1] <= '9')
        return true;

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
        /* if (strncasecmp2ltr(token, "auto", token_len) == 0) {
            pos_values->types[pos_values->nr_values] = WPV_auto;
        } */
        if (strncasecmp2ltr(token, "left", token_len) == 0) {
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
        else if (like_a_real_number(token, token_len)) {
            pos_values->types[pos_values->nr_values] = WPV_length;
            if (!parse_length_value(token, token_len,
                        pos_values->values + pos_values->nr_values))
                goto failed;
        }
        else if (token_len == 1 && token[0] == STYLE_DELIMITER) {
            break;
        }
        else {
            goto failed;
        }

        pos_values->nr_values++;
        if (pos_values->nr_values == MAX_NR_POS_VALUES)
            break;
    }

    return true;

failed:
    return false;
}

static bool
normalize_window_position_1(const struct window_position_values *values,
        struct window_position *position)
{
    switch (values->types[0]) {
        case WPV_center:
            position->x_type = WPX_center;
            position->y_type = WPY_center;
            break;

        case WPV_left:
            position->x_type = WPX_left;
            position->y_type = WPY_length;
            position->y_value.type = WLV_percentage;
            position->y_value.value = 50;
            break;

        case WPV_right:
            position->x_type = WPX_right;
            position->y_type = WPY_length;
            position->y_value.type = WLV_percentage;
            position->y_value.value = 50;
            break;

        case WPV_top:
            position->y_type = WPY_top;
            position->x_type = WPX_length;
            position->x_value.type = WLV_percentage;
            position->x_value.value = 50;
            break;

        case WPV_bottom:
            position->y_type = WPY_bottom;
            position->x_type = WPX_length;
            position->x_value.type = WLV_percentage;
            position->x_value.value = 50;
            break;

        case WPV_length:
            position->x_type = WPX_length;
            position->x_value.type = values->values[0].type;
            position->x_value.value = values->values[0].value;
            position->y_type = WPY_length;
            position->y_value.type = WLV_percentage;
            position->y_value.value = 50;
            break;
    }

    return true;
}

static bool
normalize_window_position_2(const struct window_position_values *values,
        struct window_position *position)
{
    bool x_defined = false;
    bool y_defined = false;

    position->x_type = WPX_left;
    position->y_type = WPY_top;

    PC_DEBUG("First position value type: %d\n", values->types[0]);

    switch (values->types[0]) {
        case WPV_left:
            position->x_type = WPX_left;
            x_defined = true;
            break;

        case WPV_right:
            position->x_type = WPX_right;
            x_defined = true;
            break;

        case WPV_top:
            position->y_type = WPY_top;
            y_defined = true;
            break;

        case WPV_bottom:
            position->y_type = WPY_bottom;
            y_defined = true;
            break;

        case WPV_length:
            position->x_type = WPX_length;
            position->x_value.type = values->values[0].type;
            position->x_value.value = values->values[0].value;
            x_defined = true;
            break;

        default:
            goto failed;
    }

    PC_DEBUG("First position value type: %d\n", values->types[1]);
    switch (values->types[1]) {
        case WPV_left:
            if (x_defined)
                goto failed;
            position->x_type = WPX_left;
            x_defined = true;
            break;

        case WPV_right:
            if (x_defined)
                goto failed;
            position->x_type = WPX_right;
            x_defined = true;
            break;

        case WPV_top:
            if (y_defined)
                goto failed;
            position->y_type = WPY_top;
            y_defined = true;
            break;

        case WPV_bottom:
            if (y_defined)
                goto failed;
            position->y_type = WPY_bottom;
            y_defined = true;
            break;

        case WPV_length:
            if (!x_defined) {
                position->x_type = WPX_length;
                position->x_value.type = values->values[1].type;
                position->x_value.value = values->values[1].value;
                x_defined = true;
            }
            else if (!y_defined) {
                position->y_type = WPY_length;
                position->y_value.type = values->values[1].type;
                position->y_value.value = values->values[1].value;
                y_defined = true;
            }
            break;

        default:
            goto failed;
    }

    if (!(x_defined && y_defined))
        goto failed;

    return true;

failed:
    return false;
}

enum position_proceding {
    PP_INV = 0,
    PP_X,
    PP_Y,
};

static bool
normalize_window_position_3(const struct window_position_values *values,
        struct window_position *position)
{
    enum position_proceding procedings[2] = { };

    switch (values->types[0]) {
        case WPV_left:
            procedings[0] = PP_X;
            break;

        case WPV_right:
            procedings[0] = PP_X;
            break;

        case WPV_top:
            procedings[0] = PP_Y;
            break;

        case WPV_bottom:
            procedings[0] = PP_Y;
            break;

        case WPV_center:
            procedings[0] = PP_X;
            break;

        default:
            goto failed;
            break;
    }

    switch (values->types[1]) {
        case WPV_left:
            if (procedings[0] == PP_X)
                goto failed;
            procedings[1] = PP_X;
            break;

        case WPV_right:
            if (procedings[0] == PP_X)
                goto failed;
            procedings[1] = PP_X;
            break;

        case WPV_top:
            if (procedings[0] == PP_Y)
                goto failed;
            procedings[1] = PP_Y;
            break;

        case WPV_bottom:
            if (procedings[0] == PP_Y)
                goto failed;
            procedings[1] = PP_Y;
            break;

        case WPV_center:
            if (procedings[0] == PP_X)
                procedings[1] = PP_Y;
            else
                procedings[1] = PP_X;
            break;

        case WPV_length:
            if (procedings[0] == PP_X) {
                position->x_type = WPX_left_offset;
                position->x_type += (values->types[0] - WPV_left);
                position->x_value.type = values->values[1].type;
                position->x_value.value = values->values[1].value;
            }
            else if (procedings[0] == PP_Y) {
                position->y_type = WPY_top_offset;
                position->y_type += (values->types[0] - WPV_top);
                position->y_value.type = values->values[1].type;
                position->y_value.value = values->values[1].value;
            }
            else {
                assert(0);
            }
            break;

        default:
            goto failed;
    }

    if (procedings[0] != PP_INV && procedings[1] != PP_INV &&
            values->types[2] != WPV_length) {
        goto failed;
    }

    switch (values->types[2]) {
        case WPV_left:
            if (procedings[0] == PP_X)
                goto failed;
            position->x_type =  WPX_left;
            procedings[1] = PP_X;
            break;

        case WPV_right:
            if (procedings[0] == PP_X)
                goto failed;
            position->x_type =  WPX_right;
            procedings[1] = PP_X;
            break;

        case WPV_top:
            if (procedings[0] == PP_Y)
                goto failed;
            position->y_type =  WPY_top;
            procedings[1] = PP_Y;
            break;

        case WPV_bottom:
            if (procedings[0] == PP_Y)
                goto failed;
            position->y_type =  WPY_bottom;
            procedings[1] = PP_Y;
            break;

        case WPV_center:
            if (procedings[0] == PP_X) {
                position->y_type =  WPY_center;
                procedings[1] = PP_Y;
            }
            else {
                position->x_type =  WPX_center;
                procedings[1] = PP_X;
            }
            break;

        case WPV_length:
            if (procedings[0] == PP_X) {
                /* left top 20px */
                position->x_type = WPX_left_offset;
                position->x_type += (values->types[0] - WPV_left);
                position->x_value.type = values->values[2].type;
                position->x_value.value = values->values[2].value;

                position->y_type = WPY_top_offset;
                position->y_type += (values->types[1] - WPV_top);
                position->y_value.type = values->values[2].type;
                position->y_value.value = values->values[2].value;
            }
            else {
                /* top left 20px */
                position->x_type = WPX_left_offset;
                position->x_type += (values->types[1] - WPV_left);
                position->x_value.type = values->values[2].type;
                position->x_value.value = values->values[2].value;

                position->y_type = WPY_top_offset;
                position->y_type += (values->types[0] - WPV_top);
                position->y_value.type = values->values[2].type;
                position->y_value.value = values->values[2].value;
            }
            break;

        default:
            goto failed;
    }

    return true;

failed:
    return false;
}

static bool
normalize_window_position_4(const struct window_position_values *values,
        struct window_position *position)
{
    bool x_defined = false;
    bool y_defined = false;

    switch (values->types[0]) {
        case WPV_left:
            position->x_type = WPX_left_offset;
            x_defined = true;
            break;

        case WPV_right:
            position->x_type = WPX_right_offset;
            x_defined = true;
            break;

        case WPV_center:
            position->x_type = WPX_center_offset;
            x_defined = true;
            break;

        case WPV_top:
            position->y_type = WPY_top_offset;
            y_defined = true;
            break;

        case WPV_bottom:
            position->y_type = WPY_bottom_offset;
            y_defined = true;
            break;

        default:
            goto failed;
    }

    if (values->types[1] != WPV_length)
        goto failed;

    if (x_defined) {
        position->x_value.type = values->values[1].type;
        position->x_value.value = values->values[1].value;
    }
    else if (y_defined) {
        position->y_value.type = values->values[1].type;
        position->y_value.value = values->values[1].value;
    }

    bool second_is_x = y_defined;
    switch (values->types[2]) {
        case WPV_left:
            if (x_defined)
                goto failed;
            position->x_type = WPX_left_offset;
            x_defined = true;
            break;

        case WPV_right:
            if (x_defined)
                goto failed;
            position->x_type = WPX_right_offset;
            x_defined = true;
            break;

        case WPV_top:
            if (y_defined)
                goto failed;
            position->y_type = WPY_top_offset;
            y_defined = true;
            break;

        case WPV_bottom:
            if (y_defined)
                goto failed;
            position->y_type = WPY_bottom_offset;
            y_defined = true;
            break;

        case WPV_center:
            if (x_defined) {
                position->y_type = WPY_center_offset;
                y_defined = true;
            }
            else {
                position->x_type = WPX_center_offset;
                x_defined = true;
            }
            break;

        default:
            goto failed;
    }

    if (!(x_defined && y_defined))
        goto failed;

    if (values->types[3] != WPV_length)
        goto failed;

    if (second_is_x) {
        position->x_value.type = values->values[3].type;
        position->x_value.value = values->values[3].value;
    }
    else {
        position->y_value.type = values->values[3].type;
        position->y_value.value = values->values[3].value;
    }

    return true;

failed:
    return false;
}

static bool
normalize_window_position(const struct window_position_values *values,
        struct window_position *position)
{
    bool ret = false;

    PC_DEBUG("position value: %d\n", values->nr_values);

    switch (values->nr_values) {
        case 1:
            ret = normalize_window_position_1(values, position);
            break;
        case 2:
            ret = normalize_window_position_2(values, position);
            break;
        case 3:
            ret = normalize_window_position_3(values, position);
            break;
        case 4:
            ret = normalize_window_position_4(values, position);
            break;
        default:
            break;
    }

    return ret;
}

static bool
calc_size_for_aspect_ratio(const struct purc_screen_info *screen_info,
        const struct window_size *size,
        struct purc_window_geometry *geometry)
{
    float ratio_expected, ratio_screen;

    if (size->values[0].type != WLV_number ||
            size->values[1].type != WLV_number)
        return false;

    if (size->values[0].value <= 0 || size->values[0].value <= 0)
        return false;

    ratio_expected = size->values[0].value / size->values[1].value;
    ratio_screen = screen_info->width * 1.0f / screen_info->height;

    PC_DEBUG("ratio_expected: %f, ratio_screen: %f\n",
            ratio_expected, ratio_screen);

    if (ratio_expected >= ratio_screen) {
        geometry->width = screen_info->width;
        geometry->height = (int)roundf(geometry->width / ratio_expected);
    }
    else {
        geometry->height = screen_info->height;
        geometry->width = (int)roundf(geometry->height * ratio_expected);
    }

    return true;
}

static float
calc_dots_for_length(const struct purc_screen_info *screen_info,
        const struct window_length_value *length, bool for_x)
{
    float v;

    switch (length->type) {
    case WLV_auto:
        v = for_x ? screen_info->width : screen_info->height;
        break;
    case WLV_number:
        /* assume as px */
        v = length->value;
        break;
    case WLV_percentage:
        if (for_x)
            v = length->value * screen_info->width/100.f;
        else
            v = length->value * screen_info->height/100.f;
        break;
    case WLV_unit_px:
        v = length->value;
        break;
    case WLV_unit_cm:
        v = length->value * screen_info->dpi/2.54f;
        break;
    case WLV_unit_mm:
        v = length->value * screen_info->dpi/2.54f/10;
        break;
    case WLV_unit_q:
        v = length->value * screen_info->dpi/2.54f/40;
        break;
    case WLV_unit_in:
        v = length->value * screen_info->dpi;
        break;
    case WLV_unit_pc:
        v = length->value * screen_info->dpi/6.0f;
        break;
    case WLV_unit_pt:
        v = length->value * screen_info->dpi/72.0f;
        break;

    case WLV_unit_vw:
        v = length->value * screen_info->width/100.f;
        break;
    case WLV_unit_vh:
        v = length->value * screen_info->height/100.f;
        break;
    case WLV_unit_vmax:
        if (screen_info->width > screen_info->height)
            v = length->value * screen_info->width/100.f;
        else
            v = length->value * screen_info->height/100.f;
        break;

    case WLV_unit_vmin:
        if (screen_info->width < screen_info->height)
            v = length->value * screen_info->width/100.f;
        else
            v = length->value * screen_info->height/100.f;
        break;
    }

    return v * screen_info->density;
}

static bool
calc_size_for_lengthes(const struct purc_screen_info *screen_info,
        const struct window_size *size,
        struct purc_window_geometry *geometry)
{
    if (size->values[0].type == WLV_number ||
            size->values[1].type == WLV_number)
        return false;

    float w, h;

    w = calc_dots_for_length(screen_info, size->values, true);
    h = calc_dots_for_length(screen_info, size->values + 1, false);
    if (w < 0 || h < 0)
        return false;

    geometry->width = (int)roundf(w);
    geometry->height = (int)roundf(h);
    return true;
}

static int
calc_window_position(const struct purc_screen_info *screen_info,
        struct purc_window_geometry *geometry,
        const struct window_length_value *length, bool for_x)
{
    int pos;

    PC_DEBUG("%s length type: %d, value: %f\n",
            for_x ? "X" : "Y", length->type, length->value);

    if (length->type == WLV_auto) {
        if (for_x)
            pos =
                (int)roundf((screen_info->width - geometry->width) * 0.5f);
        else
            pos =
                (int)roundf((screen_info->height - geometry->height) * 0.5f);
    }
    else if (length->type == WLV_percentage) {

        if (for_x)
            pos = (int)roundf((screen_info->width - geometry->width) *
                    length->value / 100.f);
        else
            pos = (int)roundf((screen_info->height - geometry->height) *
                    length->value / 100.f);
    }
    else {
        pos = (int)roundf(calc_dots_for_length(screen_info, length, for_x));
    }

    return pos;
}

static int
evaluate_window_geometry(const struct purc_screen_info *screen_info,
        const struct window_size *size, const struct window_position *position,
        struct purc_window_geometry *geometry)
{
    switch (size->type) {
        case WSV_screen:
            geometry->width = screen_info->width;
            geometry->height = screen_info->height;
            break;

        case WSV_square:
            geometry->width = (screen_info->width > screen_info->height) ?
                screen_info->height : screen_info->width;
            geometry->height = geometry->width;
            break;

        case WSV_aspect_ratio:
            if (!calc_size_for_aspect_ratio(screen_info, size, geometry))
                goto failed;
            break;

        case WSV_lengthes:
            if (!calc_size_for_lengthes(screen_info, size, geometry))
                goto failed;
            break;
    }

    geometry->x = 0;
    geometry->y = 0;

    int dots;

    PC_DEBUG("X position type: %d\n", position->x_type);
    switch (position->x_type) {
        case WPX_left:
            geometry->x = 0;
            break;

        case WPX_right:
            geometry->x = screen_info->width - geometry->width;
            break;

        case WPX_center:
            geometry->x =
                (int)roundf((screen_info->width - geometry->width) * 0.5f);
            break;

        case WPX_left_offset:
            dots = (int)roundf(calc_dots_for_length(screen_info,
                    &position->x_value, true));
            PC_DEBUG("left offset: %d\n", dots);
            geometry->x = dots;
            break;

        case WPX_right_offset:
            dots = (int)roundf(calc_dots_for_length(screen_info,
                    &position->x_value, true));
            geometry->x = screen_info->width - dots - geometry->width;
            break;

        case WPX_center_offset:
            dots = (int)roundf(calc_dots_for_length(screen_info,
                    &position->x_value, true));
            geometry->x =
                (int)roundf((screen_info->width - geometry->width) * 0.5f);
            geometry->x += dots;
            break;

        case WPX_length:
            dots = calc_window_position(screen_info, geometry,
                    &position->x_value, true);
            geometry->x = dots;
            break;
    }

    PC_DEBUG("Y position type: %d\n", position->y_type);
    switch (position->y_type) {
        case WPY_top:
            geometry->y = 0;
            break;

        case WPY_bottom:
            geometry->y = screen_info->height - geometry->height;
            break;

        case WPY_center:
            geometry->y =
                (int)roundf((screen_info->height - geometry->height) * 0.5f);
            break;

        case WPY_top_offset:
            dots = (int)roundf(calc_dots_for_length(screen_info,
                    &position->y_value, false));
            geometry->y = dots;
            break;

        case WPY_bottom_offset:
            dots = (int)roundf(calc_dots_for_length(screen_info,
                    &position->x_value, false));
            geometry->y = screen_info->height - dots - geometry->height;
            break;

        case WPY_center_offset:
            dots = (int)roundf(calc_dots_for_length(screen_info,
                    &position->y_value, false));
            geometry->y =
                (int)roundf((screen_info->height - geometry->height) * 0.5f);
            geometry->y += dots;
            break;

        case WPY_length:
            dots = calc_window_position(screen_info, geometry,
                    &position->y_value, false);
            geometry->y = dots;
            break;
    }

    return 0;

failed:
    return -1;
}

int
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

