/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *          http://www.opensource.org/licenses/mit-license.php
 * Copyright 2024 FMSoft <https://www.fmsoft.cn>
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_background_size(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_BACKGROUND_SIZE_INHERIT;
    uint16_t hvalue = CSS_BACKGROUND_SIZE_INHERIT;
    uint16_t vvalue = CSS_BACKGROUND_SIZE_INHERIT;
    css_fixed hlength = 0;
    css_fixed vlength = 0;
    uint32_t hunit = UNIT_PX;
    uint32_t vunit = UNIT_PX;

    if (isInherit(opv) == false) {
        switch (getValue(opv) & 0xf0) {
        case BACKGROUND_SIZE_HORZ_SET:
            hvalue = CSS_BACKGROUND_SIZE_SIZE;
            hlength = *((css_fixed *) style->bytecode);
            advance_bytecode(style, sizeof(hlength));
            hunit = *((uint32_t *) style->bytecode);
            advance_bytecode(style, sizeof(hunit));
            break;

        case BACKGROUND_SIZE_HORZ_CONTAIN:
            hvalue = CSS_BACKGROUND_SIZE_CONTAIN;
            break;

        case BACKGROUND_SIZE_HORZ_COVER:
            hvalue = CSS_BACKGROUND_SIZE_COVER;
            break;

        case BACKGROUND_SIZE_HORZ_AUTO:
            hvalue = CSS_BACKGROUND_SIZE_AUTO;
            break;
        }

        switch (getValue(opv) & 0x0f) {
        case BACKGROUND_SIZE_VERT_SET:
            vvalue = CSS_BACKGROUND_SIZE_SIZE;
            vlength = *((css_fixed *) style->bytecode);
            advance_bytecode(style, sizeof(vlength));
            vunit = *((uint32_t *) style->bytecode);
            advance_bytecode(style, sizeof(vunit));
            break;

        case BACKGROUND_SIZE_VERT_CONTAIN:
            vvalue = CSS_BACKGROUND_SIZE_CONTAIN;
            break;

        case BACKGROUND_SIZE_VERT_COVER:
            vvalue = CSS_BACKGROUND_SIZE_COVER;
            break;

        case BACKGROUND_SIZE_VERT_AUTO:
            vvalue = CSS_BACKGROUND_SIZE_AUTO;
            break;
        }
    }

    if (hvalue == vvalue) {
        value = hvalue;
    }
    else if (hvalue == CSS_BACKGROUND_SIZE_SIZE
            || vvalue == CSS_BACKGROUND_SIZE_SIZE) {
        value = CSS_BACKGROUND_SIZE_SIZE;
    }
    else {
        value = hvalue;
    }

    hunit = css__to_css_unit(hunit);
    vunit = css__to_css_unit(vunit);

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
            isInherit(opv))) {
        return set_background_size(state->computed, value,
                hlength, hunit, vlength, vunit);
    }

    return CSS_OK;
}

css_error css__set_background_size_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_background_size(style, hint->status,
        hint->data.position.h.value, hint->data.position.h.unit,
        hint->data.position.v.value, hint->data.position.v.unit);
}

css_error css__initial_background_size(css_select_state *state)
{
    return set_background_size(state->computed,
            CSS_BACKGROUND_SIZE_AUTO,
            0, CSS_UNIT_PCT, 0, CSS_UNIT_PCT);
}

css_error css__compose_background_size(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    css_fixed hlength = 0, vlength = 0;
    css_unit hunit = CSS_UNIT_PX, vunit = CSS_UNIT_PX;
    uint8_t type = get_background_size(child, &hlength, &hunit,
            &vlength, &vunit);

    if (type == CSS_BACKGROUND_SIZE_INHERIT) {
        type = get_background_size(parent,
                &hlength, &hunit, &vlength, &vunit);
    }

    return set_background_size(result, type, hlength, hunit,
                vlength, vunit);
}


