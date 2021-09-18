/*
 * @file hvml-parser.c
 * @author Xu Xiaohong
 * @date 2021/09/01
 * @brief The interfaces for hvml token.
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
 */

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "hvml-parser.h"

#ifndef VTT
#define VTT(x)  PCHVML_TOKEN##x
#endif // VTT

#ifndef VDC
#define VDC(x)  PCVDOM_CONSTRUCTION##x
#endif // VDC

struct pcvdom_construction_stack*
pcvdom_construction_stack_create(void)
{
    struct pcvdom_construction_stack *stack;
    stack = (struct pcvdom_construction_stack*)calloc(1, sizeof(*stack));
    if (!stack)
        return NULL;

    return stack;
}

struct pcvdom_document*
pcvdom_construction_stack_end(struct pcvdom_construction_stack *stack)
{
    struct pcvdom_document *doc = stack->doc;
    stack->doc  = NULL; // transfer ownership
    stack->curr = NULL;

    stack->eof = 1;

    return doc;
}

void
pcvdom_construction_stack_destroy(struct pcvdom_construction_stack *stack)
{
    if (stack->doc) {
        pcvdom_document_destroy(stack->doc);
        stack->doc = NULL;
    }

    stack->doc  = NULL;
    stack->curr = NULL;

    free(stack);
}

static int
_on_initial(struct pcvdom_construction_stack *stack,
        struct pchvml_token *token);

static int
_on_before_hvml(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token);

static int
_on_before_head(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token);

static int
_on_in_head(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token);

static int
_on_after_head(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token);

static int
_on_in_body(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token);

static int
_on_text(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token);

static int
_on_after_body(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token);

static int
_on_after_after_body(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token);

int
pcvdom_construction_stack_push_token(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    if (stack->eof)
        return -1;

    if (!stack->doc) {
        stack->doc = pcvdom_document_create();
        if (!stack->doc)
            return -1;
    }

    switch (stack->mode) {
        case VDC(_INITIAL):
            return _on_initial(stack, token);
        case VDC(_BEFORE_HVML):
            return _on_before_hvml(stack, token);
        case VDC(_BEFORE_HEAD):
            return _on_before_head(stack, token);
        case VDC(_IN_HEAD):
            return _on_in_head(stack, token);
        case VDC(_AFTER_HEAD):
            return _on_after_head(stack, token);
        case VDC(_IN_BODY):
            return _on_in_body(stack, token);
        case VDC(_TEXT):
            return _on_text(stack, token);
        case VDC(_AFTER_BODY):
            return _on_after_body(stack, token);
        case VDC(_AFTER_AFTER_BODY):
            return _on_after_after_body(stack, token);
    default:
        return -1;
    }
}

static int
_on_initial(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    if (pchvml_token_is_type(token, VTT(_CHARACTER))) {
        if (_is_blank(token))
            return 0;
    }
    else if (pchvml_token_is_type(token, VTT(_COMMENT))) {
        return _append_comment(stack, token);
    }
    else if (pchvml_token_is_type(token, VTT(_DOCTYPE))) {
        return _set_doctype(stack, token);
    }
    _set_quirks(stack);
    _set_empty_doctype(stack);
    return 0;
}

static int
_on_before_hvml(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    if (pchvml_token_is_type(token, VTT(_DOCTYPE))) {
        return 0;
    }
    else if (pchvml_token_is_type(token, VTT(_COMMENT))) {
        return _append_comment(stack, token);
    }
    else if (pchvml_token_is_type(token, VTT(_START_TAG))) {
        const char *tag = _get_tag_name(token);
        if (strcmp(tag, "hvml")==0) {
            return _create_hvml_element(stack, token);
        }
    }
    else if (pchvml_token_is_type(token, VTT(_END_TAG))) {
        if (strcmp(tag, "head") &&
            strcmp(tag, "body") &&
            strcmp(tag, "hvml"))
        {
            return 0;
        }
    }
    _create_empty_hvml_element(stack);
    return 0;
}

static int
_on_before_head(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    if (pchvml_token_is_type(token, VTT(_CHARACTER))) {
        if (_is_blank(token))
            return 0;
    }
    else if (pchvml_token_is_type(token, VTT(_COMMENT))) {
        return _append_comment(stack, token);
    }
    else if (pchvml_token_is_type(token, VTT(_DOCTYPE))) {
        return 0;
    }
    else if (pchvml_token_is_type(token, VTT(_START_TAG))) {
        const char *tag = _get_tag_name(token);
        if (strcmp(tag, "hvml")==0) {
            return 0;
        }
        else if (strcmp(tag, "head")==0) {
            return _create_head_element(stack, token);
        }
    }
    else if (pchvml_token_is_type(token, VTT(_END_TAG))) {
        if (strcmp(tag, "head") &&
            strcmp(tag, "body") &&
            strcmp(tag, "hvml"))
        {
            return 0;
        }
    }
    _create_empty_head_element(stack);
    _set_reprocess(stack);
    return 0;
}

static int
_on_in_head(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    if (pchvml_token_is_type(token, VTT(_CHARACTER))) {
        if (_is_blank(token))
            return _append_text(token);
    }
    else if (pchvml_token_is_type(token, VTT(_COMMENT))) {
        return _append_comment(stack, token);
    }
    else if (pchvml_token_is_type(token, VTT(_DOCTYPE))) {
        return 0;
    }
    else if (pchvml_token_is_type(token, VTT(_START_TAG))) {
        const char *tag = _get_tag_name(token);
        if (strcmp(tag, "hvml")==0) {
            return 0;
        }
        else if (_is_foreign(tag)) {
            // TODO: a bit more explanation
            return -1;
        }
        else if (strcmp(tag, "init")==0 ||
                 strcmp(tag, "set")==0 ||
                 strcmp(tag, "bind")==0 ||
                 strcmp(tag, "connect")==0)
        {
            return _create_element(stack, token);
        }
        else if (strcmp(tag, "head")==0) {
            return 0;
        }
    }
    else if (pchvml_token_is_type(token, VTT(_END_TAG))) {
        if (strcmp(tag, "head") == 0) {
            return _pop_head_off(stack);
        }
        else if (strcmp(tag, "body") &&
                 strcmp(tag, "hvml"))
        {
            return 0;
        }
        else if (strcmp(tag, "archedata") == 0) {
            return _append_archedata(stack);
        }
        else if (strcmp(tag, "archetype") == 0) {
            return -1;
        }
        return 0;
    }

    _pop_head_off(stack);
    _set_mode(stack, VDC(_AFTER_HEAD));
    _set_reprocess(stack);
    return 0;
}

static int
_on_after_head(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    if (pchvml_token_is_type(token, VTT(_CHARACTER))) {
        if (_is_blank(token))
            return _append_text(token);
    }
    else if (pchvml_token_is_type(token, VTT(_COMMENT))) {
        return _append_comment(stack, token);
    }
    else if (pchvml_token_is_type(token, VTT(_DOCTYPE))) {
        return 0;
    }
    else if (pchvml_token_is_type(token, VTT(_START_TAG))) {
        const char *tag = _get_tag_name(token);
        if (strcmp(tag, "hvml")==0) {
            return 0;
        }
        else if (strcmp(tag, "body")==0) {
            return _create_body(stack, token);
        }
        else if (strcmp(tag, "archetype")==0) {
            return 0;
        }
    }
}

static int
_on_in_body(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_text(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_after_body(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(token);
    return -1;
}

static int
_on_after_after_body(struct pcvdom_construction_stack *stack,
    struct pchvml_token *token)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(token);
    return -1;
}

