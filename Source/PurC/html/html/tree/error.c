/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "private/errors.h"
#include "html/html/tree/error.h"


lxb_html_tree_error_t *
lxb_html_tree_error_add(lexbor_array_obj_t *parse_errors,
                        lxb_html_token_t *token, lxb_html_tree_error_id_t id)
{
    UNUSED_PARAM(token);

    if (parse_errors == NULL) {
        return NULL;
    }

    lxb_html_tree_error_t *entry = lexbor_array_obj_push(parse_errors);
    if (entry == NULL) {
        return NULL;
    }

    entry->id = id;

    return entry;
}
