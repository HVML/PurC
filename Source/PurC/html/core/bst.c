/**
 * @file bst.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of bst.
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"

#include "html/core/bst.h"


pchtml_bst_t *
pchtml_bst_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_bst_t));
}

unsigned int
pchtml_bst_init(pchtml_bst_t *bst, size_t size)
{
    unsigned int status;

    if (bst == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (size == 0) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    bst->dobject = pchtml_dobject_create();
    status = pchtml_dobject_init(bst->dobject, size,
                                 sizeof(pchtml_bst_entry_t));
    if (status != PCHTML_STATUS_OK) {
        return status;
    }

    bst->root = 0;
    bst->tree_length = 0;

    return PCHTML_STATUS_OK;
}

void
pchtml_bst_clean(pchtml_bst_t *bst)
{
    pchtml_dobject_clean(bst->dobject);

    bst->root = 0;
    bst->tree_length = 0;
}

pchtml_bst_t *
pchtml_bst_destroy(pchtml_bst_t *bst, bool self_destroy)
{
    if (bst == NULL) {
        return NULL;
    }

    bst->dobject = pchtml_dobject_destroy(bst->dobject, true);

    if (self_destroy) {
        return pchtml_free(bst);
    }

    return bst;
}

pchtml_bst_entry_t *
pchtml_bst_entry_make(pchtml_bst_t *bst, size_t size)
{
    pchtml_bst_entry_t *new_entry = pchtml_dobject_calloc(bst->dobject);
    if (new_entry == NULL) {
        return NULL;
    }

    new_entry->size = size;

    bst->tree_length++;

    return new_entry;
}

pchtml_bst_entry_t *
pchtml_bst_insert(pchtml_bst_t *bst, pchtml_bst_entry_t **scope,
                  size_t size, void *value)
{
    pchtml_bst_entry_t *new_entry, *entry;

    new_entry = pchtml_dobject_calloc(bst->dobject);
    if (new_entry == NULL) {
        return NULL;
    }

    new_entry->size  = size;
    new_entry->value = value;

    bst->tree_length++;

    if (*scope == NULL) {
        *scope = new_entry;
        return new_entry;
    }

    entry = *scope;

    while (entry != NULL) {
        if (size == entry->size) {
            if (entry->next) {
                new_entry->next = entry->next;
            }

            entry->next = new_entry;
            new_entry->parent = entry->parent;

            return new_entry;
        }
        else if (size > entry->size) {
            if (entry->right == NULL) {
                entry->right = new_entry;
                new_entry->parent = entry;

                return new_entry;
            }

            entry = entry->right;
        }
        else {
            if (entry->left == NULL) {
                entry->left = new_entry;
                new_entry->parent = entry;

                return new_entry;
            }

            entry = entry->left;
        }
    }

    return NULL;
}

pchtml_bst_entry_t *
pchtml_bst_insert_not_exists(pchtml_bst_t *bst, pchtml_bst_entry_t **scope,
                             size_t size)
{
    pchtml_bst_entry_t *entry;

    if (*scope == NULL) {
        *scope = pchtml_bst_entry_make(bst, size);

        return *scope;
    }

    entry = *scope;

    while (entry != NULL) {
        if (size == entry->size) {
            return entry;
        }
        else if (size > entry->size) {
            if (entry->right == NULL) {
                entry->right = pchtml_bst_entry_make(bst, size);
                entry->right->parent = entry;

                return entry->right;
            }

            entry = entry->right;
        }
        else {
            if (entry->left == NULL) {
                entry->left = pchtml_bst_entry_make(bst, size);
                entry->left->parent = entry;

                return entry->left;
            }

            entry = entry->left;
        }
    }

    return NULL;
}

pchtml_bst_entry_t *
pchtml_bst_search(pchtml_bst_t *bst, pchtml_bst_entry_t *scope, size_t size)
{
    UNUSED_PARAM(bst);

    while (scope != NULL) {
        if (scope->size == size) {
            return scope;
        }
        else if (size > scope->size) {
            scope = scope->right;
        }
        else {
            scope = scope->left;
        }
    }

    return NULL;
}

pchtml_bst_entry_t *
pchtml_bst_search_close(pchtml_bst_t *bst, pchtml_bst_entry_t *scope,
                        size_t size)
{
    UNUSED_PARAM(bst);

    pchtml_bst_entry_t *max = NULL;

    while (scope != NULL) {
        if (scope->size == size) {
            return scope;
        }
        else if (size > scope->size) {
            scope = scope->right;
        }
        else {
            max = scope;
            scope = scope->left;
        }
    }

    return max;
}

void *
pchtml_bst_remove(pchtml_bst_t *bst, pchtml_bst_entry_t **scope, size_t size)
{
    pchtml_bst_entry_t *entry = *scope;

    while (entry != NULL) {
        if (entry->size == size) {
            return pchtml_bst_remove_by_pointer(bst, entry, scope);
        }
        else if (size > entry->size) {
            entry = entry->right;
        }
        else {
            entry = entry->left;
        }
    }

    return NULL;
}

void *
pchtml_bst_remove_close(pchtml_bst_t *bst, pchtml_bst_entry_t **scope,
                        size_t size, size_t *found_size)
{
    pchtml_bst_entry_t *entry = *scope;
    pchtml_bst_entry_t *max = NULL;

    while (entry != NULL) {
        if (entry->size == size) {
            if (found_size) {
                *found_size = entry->size;
            }

            return pchtml_bst_remove_by_pointer(bst, entry, scope);
        }
        else if (size > entry->size) {
            entry = entry->right;
        }
        else {
            max = entry;
            entry = entry->left;
        }
    }

    if (max != NULL) {
        if (found_size != NULL) {
            *found_size = max->size;
        }

        return pchtml_bst_remove_by_pointer(bst, max, scope);
    }

    if (found_size != NULL) {
        *found_size = 0;
    }

    return NULL;
}

void *
pchtml_bst_remove_by_pointer(pchtml_bst_t *bst, pchtml_bst_entry_t *entry,
                             pchtml_bst_entry_t **root)
{
    void *value;
    pchtml_bst_entry_t *next, *right, *left;

    bst->tree_length--;

    if (entry->next != NULL) {
        next = entry->next;
        entry->next = entry->next->next;

        value = next->value;

        pchtml_dobject_free(bst->dobject, next);

        return value;
    }

    value = entry->value;

    if (entry->left == NULL && entry->right == NULL) {
        if (entry->parent != NULL) {
            if (entry->parent->left == entry)  entry->parent->left  = NULL;
            if (entry->parent->right == entry) entry->parent->right = NULL;
        }
        else {
            *root = NULL;
        }

        pchtml_dobject_free(bst->dobject, entry);
    }
    else if (entry->left == NULL) {
        if (entry->parent == NULL) {
            entry->right->parent = NULL;

            *root = entry->right;

            pchtml_dobject_free(bst->dobject, entry);

            entry = *root;
        }
        else {
            right = entry->right;
            right->parent = entry->parent;

            memcpy(entry, right, sizeof(pchtml_bst_entry_t));

            pchtml_dobject_free(bst->dobject, right);
        }

        if (entry->right != NULL) {
            entry->right->parent = entry;
        }

        if (entry->left != NULL) {
            entry->left->parent = entry;
        }
    }
    else if (entry->right == NULL) {
        if (entry->parent == NULL) {
            entry->left->parent = NULL;

            *root = entry->left;

            pchtml_dobject_free(bst->dobject, entry);

            entry = *root;
        }
        else {
            left = entry->left;
            left->parent = entry->parent;

            memcpy(entry, left, sizeof(pchtml_bst_entry_t));

            pchtml_dobject_free(bst->dobject, left);
        }

        if (entry->right != NULL) {
            entry->right->parent = entry;
        }

        if (entry->left != NULL) {
            entry->left->parent = entry;
        }
    }
    else {
        left = entry->right;

        while (left->left != NULL) {
            left = left->left;
        }

        /* Swap */
        entry->size  = left->size;
        entry->next  = left->next;
        entry->value = left->value;

        /* Change parrent */
        if (entry->right == left) {
            entry->right = left->right;

            if (entry->right != NULL) {
                left->right->parent = entry;
            }
        }
        else {
            left->parent->left = left->right;

            if (left->right != NULL) {
                left->right->parent = left->parent;
            }
        }

        pchtml_dobject_free(bst->dobject, left);
    }

    return value;
}

void
pchtml_bst_serialize(pchtml_bst_t *bst, pchtml_callback_f callback, void *ctx)
{
    pchtml_bst_serialize_entry(bst->root, callback, ctx, 0);
}

void
pchtml_bst_serialize_entry(pchtml_bst_entry_t *entry,
                           pchtml_callback_f callback, void *ctx, size_t tabs)
{
    size_t buff_len;
    char buff[1024];

    if (entry == NULL) {
        return;
    }

    /* Left */
    for (size_t i = 0; i < tabs; i++) {
        callback((unsigned char *) "\t", 1, ctx);
    }
    callback((unsigned char *) "<left ", 6, ctx);

    if (entry->left) {
        buff_len = sprintf(buff, PCHTML_FORMAT_Z, entry->left->size);
        callback((unsigned char *) buff, buff_len, ctx);

        callback((unsigned char *) ">\n", 2, ctx);
        pchtml_bst_serialize_entry(entry->left, callback, ctx, (tabs + 1));

        for (size_t i = 0; i < tabs; i++) {
            callback((unsigned char *) "\t", 1, ctx);
        }
    }
    else {
        callback((unsigned char *) "NULL>", 5, ctx);
    }

    callback((unsigned char *) "</left>\n", 8, ctx);

    /* Right */
    for (size_t i = 0; i < tabs; i++) {
        callback((unsigned char *) "\t", 1, ctx);
    }
    callback((unsigned char *) "<right ", 7, ctx);

    if (entry->right) {
        buff_len = sprintf(buff, PCHTML_FORMAT_Z, entry->right->size);
        callback((unsigned char *) buff, buff_len, ctx);

        callback((unsigned char *) ">\n", 2, ctx);
        pchtml_bst_serialize_entry(entry->right, callback, ctx, (tabs + 1));

        for (size_t i = 0; i < tabs; i++) {
            callback((unsigned char *) "\t", 1, ctx);
        }
    }
    else {
        callback((unsigned char *) "NULL>", 5, ctx);
    }

    callback((unsigned char *) "</right>\n", 9, ctx);
}
