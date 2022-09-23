/////////////////////////////////////////////////////////////////////////////// //
//                          IMPORTANT NOTICE
//
// The following open source license statement does not apply to any
// entity in the Exception List published by FMSoft.
//
// For more information, please visit:
//
// https://www.fmsoft.cn/exception-list
//
//////////////////////////////////////////////////////////////////////////////
/**
 \verbatim

    This file is part of DOM Ruler. DOM Ruler is a library to
    maintain a DOM tree, lay out and stylize the DOM nodes by
    using CSS (Cascaded Style Sheets).

    Copyright (C) 2021 Beijing FMSoft Technologies Co., Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General License for more details.

    You should have received a copy of the GNU Lesser General License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Or,

    As this program is a library, any link to this program must follow
    GNU Lesser General License version 3 (LGPLv3). If you cannot accept
    LGPLv3, you need to be licensed from FMSoft.

    If you have got a commercial license of this program, please use it
    under the terms and conditions of the commercial license.

    For more information about the commercial license, please refer to
    <http://www.minigui.com/blog/minigui-licensing-policy/>.

 \endverbatim
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_grid_template_columns(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_GRID_TEMPLATE_COLUMNS_INHERIT;
    css_fixed* values = NULL;
    css_unit* units = NULL;
    int32_t n_values = 0;

    if (isInherit(opv) == false) {
        uint32_t v = getValue(opv);

        while (v != GRID_TEMPLATE_COLUMNS_END) {
            css_fixed length;
            css_unit unit;

            switch (v) {
                case GRID_TEMPLATE_COLUMNS_SET:
                    value = CSS_GRID_TEMPLATE_COLUMNS_SET;
                    length = *((css_fixed *) style->bytecode);
                    advance_bytecode(style, sizeof(length));
                    unit = *((uint32_t *) style->bytecode);
                    advance_bytecode(style, sizeof(unit));
                    unit = css__to_css_unit(unit);
                    break;

                default:
                    continue;
            }

            css_fixed* temp_values = (css_fixed*)realloc(values, (n_values + 1) * sizeof(css_fixed));
            if (temp_values == NULL)
            {
                if (values != NULL)
                {
                    free(values);
                }
                return CSS_NOMEM;
            }
            values= temp_values;
            values[n_values] = length;

            css_unit* temp_units= (css_unit*)realloc(units, (n_values + 1) * sizeof(css_unit));
            if (temp_units == NULL)
            {
                if (units != NULL)
                {
                    free(units);
                }
                if (values != NULL) {
                    free(values);
                }
                return CSS_NOMEM;
            }
            units= temp_units;
            units[n_values] = unit;
            n_values++;
            v = getValue(*((uint32_t *) style->bytecode));
            advance_bytecode(style, sizeof(v));
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
            isInherit(opv))) {
        css_error error = set_grid_template_columns(state->computed, value, n_values, values, units);
        if (values != NULL)
        {
            free(values);
        }

        if (units != NULL)
        {
            free(units);
        }
        return error;
    }

    return CSS_OK;
}

css_error css__set_grid_template_columns_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    (void)hint;
    return set_grid_template_columns(style, CSS_GRID_TEMPLATE_COLUMNS_AUTO, 0, NULL, NULL);
}

css_error css__initial_grid_template_columns(css_select_state *state)
{
    return set_grid_template_columns(state->computed, CSS_GRID_TEMPLATE_COLUMNS_AUTO,  0, NULL, NULL);
}

css_error css__compose_grid_template_columns(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    int32_t size = 0;
    css_error error;
    css_fixed* values = NULL;
    css_unit* units = NULL;

    uint8_t type = get_grid_template_columns(child, &size, &values, &units);
    if (type == CSS_GRID_TEMPLATE_COLUMNS_INHERIT) {
        if (values != NULL)
        {
            free(values);
        }
        if (units != NULL)
        {
            free(units);
        }
        type = get_grid_template_columns(parent, &size, &values, &units);
    }

    error = set_grid_template_columns(result, type, size, values, units);
    if (values != NULL)
    {
        free(values);
    }
    if (units != NULL)
    {
        free(units);
    }
    return error;
}
