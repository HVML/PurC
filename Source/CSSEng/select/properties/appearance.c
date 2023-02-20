/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *          http://www.opensource.org/licenses/mit-license.php
 * Copyright 2023 FMSoft <https://www.fmsoft.cn>
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_appearance(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_APPEARANCE_INHERIT;

    UNUSED(style);

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case APPEARANCE_NONE:
            value = CSS_APPEARANCE_NONE;
            break;
        case APPEARANCE_AUTO:
            value = CSS_APPEARANCE_AUTO;
            break;
        case APPEARANCE_MENULIST:
            value = CSS_APPEARANCE_MENULIST;
            break;
        case APPEARANCE_MENULIST_BUTTON:
            value = CSS_APPEARANCE_MENULIST_BUTTON;
            break;
        case APPEARANCE_TEXTFIELD:
            value = CSS_APPEARANCE_TEXTFIELD;
            break;
        case APPEARANCE_TEXTAREA:
            value = CSS_APPEARANCE_TEXTAREA;
            break;
        case APPEARANCE_PROGRESS_BAR:
            value = CSS_APPEARANCE_PROGRESS_BAR;
            break;
        case APPEARANCE_PROGRESS_BKGND:
            value = CSS_APPEARANCE_PROGRESS_BKGND;
            break;
        case APPEARANCE_PROGRESS_MARK:
            value = CSS_APPEARANCE_PROGRESS_MARK;
            break;
        case APPEARANCE_METER:
            value = CSS_APPEARANCE_METER;
            break;
        case APPEARANCE_METER_BAR:
            value = CSS_APPEARANCE_METER_BAR;
            break;
        case APPEARANCE_METER_BKGND:
            value = CSS_APPEARANCE_METER_BKGND;
            break;
        case APPEARANCE_METER_MARK:
            value = CSS_APPEARANCE_METER_MARK;
            break;
        case APPEARANCE_SLIDER_HORIZONTAL:
            value = CSS_APPEARANCE_SLIDER_HORIZONTAL;
            break;
        case APPEARANCE_SLIDER_VERTICAL:
            value = CSS_APPEARANCE_SLIDER_VERTICAL;
            break;
        case APPEARANCE_BUTTON:
            value = CSS_APPEARANCE_BUTTON;
            break;
        case APPEARANCE_CHECKBOX:
            value = CSS_APPEARANCE_CHECKBOX;
            break;
        case APPEARANCE_LISTBOX:
            value = CSS_APPEARANCE_LISTBOX;
            break;
        case APPEARANCE_RADIO:
            value = CSS_APPEARANCE_RADIO;
            break;
        case APPEARANCE_SEARCHFIELD:
            value = CSS_APPEARANCE_SEARCHFIELD;
            break;
        case APPEARANCE_PUSH_BUTTON:
            value = CSS_APPEARANCE_PUSH_BUTTON;
            break;
        case APPEARANCE_SQUARE_BUTTON:
            value = CSS_APPEARANCE_SQUARE_BUTTON;
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
        return set_appearance(state->computed, value);
    }

    return CSS_OK;
}

css_error css__set_appearance_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_appearance(style, hint->status);
}

css_error css__initial_appearance(css_select_state *state)
{
    return set_appearance(state->computed, CSS_APPEARANCE_NONE);
}

css_error css__compose_appearance(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    uint8_t type = get_appearance(child);

    if (type == CSS_APPEARANCE_INHERIT) {
        type = get_appearance(parent);
    }

    return set_appearance(result, type);
}
