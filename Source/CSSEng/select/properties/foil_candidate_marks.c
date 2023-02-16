/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *          http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade__foil_candidate_marks(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_FOIL_CANDIDATE_MARKS_INHERIT;
    lwc_string *marks = NULL;

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case FOIL_CANDIDATE_MARKS_AUTO:
            value = CSS_FOIL_CANDIDATE_MARKS_AUTO;
            break;
        case FOIL_CANDIDATE_MARKS_SET:
            value = CSS_FOIL_CANDIDATE_MARKS_SET;
            css__stylesheet_string_get(style->sheet, *((css_code_t *) style->bytecode), &marks);
            advance_bytecode(style, sizeof(css_code_t));
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv),
            isImportant(opv), state, isInherit(opv))) {
        return set__foil_candidate_marks(state->computed, value, marks);
    }

    return CSS_OK;
}

css_error css__set__foil_candidate_marks_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    css_error error;

    error = set__foil_candidate_marks(style, hint->status, hint->data.string);

    if (hint->data.string != NULL)
        lwc_string_unref(hint->data.string);

    return error;
}

css_error css__initial__foil_candidate_marks(css_select_state *state)
{
    return set__foil_candidate_marks(state->computed,
            CSS_FOIL_CANDIDATE_MARKS_AUTO, NULL);
}

css_error css__compose__foil_candidate_marks(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    lwc_string *marks;
    uint8_t type = get__foil_candidate_marks(child, &marks);

    if (type == CSS_FOIL_CANDIDATE_MARKS_INHERIT) {
        type = get__foil_candidate_marks(parent, &marks);
    }

    return set__foil_candidate_marks(result, type, marks);
}

