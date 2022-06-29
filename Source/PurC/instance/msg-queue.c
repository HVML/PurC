/*
 * @file msg-queue.c
 * @author XueShuming
 * @date 2022/06/28
 * @brief The impl for message queue.
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

#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"
#include "private/msg-queue.h"

#if HAVE(GLIB)
    #include <gmodule.h>
#endif

struct pcinst_msg_queue *
pcinst_msg_queue_create(void)
{
    int errcode = 0;
    struct pcinst_msg_queue *queue = NULL;

    if ((queue = malloc(sizeof(*queue))) == NULL) {
        errcode = PURC_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    purc_rwlock_init(&queue->lock);
    if (queue->lock.native_impl == NULL) {
        errcode = PURC_ERROR_BAD_SYSTEM_CALL;
        goto done;
    }

    queue->state = 0;
    queue->nr_msgs = 0;
    list_head_init(&queue->req_msgs);
    list_head_init(&queue->res_msgs);
    list_head_init(&queue->event_msgs);
    list_head_init(&queue->void_msgs);

done:

    if (errcode) {
        if (queue) {
            if (queue->lock.native_impl) {
                purc_rwlock_clear(&queue->lock);
            }

            free(queue);
        }

        purc_set_error(errcode);
        return NULL;
    }
    return queue;
}

static void
grind_message(pcrdr_msg *msg)
{
    for (int i = 0; i < PCRDR_NR_MSG_VARIANTS; i++) {
        if (msg->variants[i])
            purc_variant_unref(msg->variants[i]);
    }

#if HAVE(GLIB)
    g_slice_free1(sizeof(pcrdr_msg), (gpointer)msg);
#else
    free(msg);
#endif
}

static ssize_t
grind_msg_list(struct list_head *msgs)
{
    ssize_t nr = 0;
    struct list_head *p, *n;
    list_for_each_safe(p, n, msgs) {
        struct pcinst_msg_hdr *hdr;
        hdr = list_entry(p, struct pcinst_msg_hdr, ln);
        list_del(p);
        grind_message((pcrdr_msg *)hdr);
        nr++;
    }
    return nr;
}

ssize_t
pcinst_msg_queue_destroy(struct pcinst_msg_queue *queue)
{
    ssize_t nr = 0;
    purc_rwlock_writer_lock(&queue->lock);

    nr += grind_msg_list(&queue->req_msgs);
    nr += grind_msg_list(&queue->res_msgs);
    nr += grind_msg_list(&queue->event_msgs);
    nr += grind_msg_list(&queue->void_msgs);
    queue->nr_msgs -= nr;

    purc_rwlock_writer_unlock(&queue->lock);

    purc_rwlock_clear(&queue->lock);
    free(queue);

    return nr;
}

int
pcinst_msg_queue_append(struct pcinst_msg_queue *queue, pcrdr_msg *msg)
{
    struct pcinst_msg_hdr *hdr = (struct pcinst_msg_hdr *)msg;

    purc_rwlock_writer_lock(&queue->lock);

    switch (msg->type) {
    case PCRDR_MSG_TYPE_VOID:
        list_add_tail(&hdr->ln, &queue->void_msgs);
        queue->state |= MSG_QS_VOID;
        queue->nr_msgs++;
        break;

    case PCRDR_MSG_TYPE_REQUEST:
        list_add_tail(&hdr->ln, &queue->req_msgs);
        queue->state |= MSG_QS_REQ;
        queue->nr_msgs++;
        break;

    case PCRDR_MSG_TYPE_RESPONSE:
        list_add_tail(&hdr->ln, &queue->res_msgs);
        queue->state |= MSG_QS_RES;
        queue->nr_msgs++;
        break;

    case PCRDR_MSG_TYPE_EVENT:
        queue->state |= MSG_QS_EVENT;
        break;

    default:
        list_add_tail(&hdr->ln, &queue->void_msgs);
        queue->state |= MSG_QS_VOID;
        queue->nr_msgs++;
        break;
    }

    purc_rwlock_writer_unlock(&queue->lock);
    return 0;
}

int
pcinst_msg_queue_prepend(struct pcinst_msg_queue *queue, pcrdr_msg *msg)
{
    struct pcinst_msg_hdr *hdr = (struct pcinst_msg_hdr *)msg;

    purc_rwlock_writer_lock(&queue->lock);

    switch (msg->type) {
    case PCRDR_MSG_TYPE_VOID:
        list_add(&hdr->ln, &queue->void_msgs);
        queue->state |= MSG_QS_VOID;
        queue->nr_msgs++;
        break;

    case PCRDR_MSG_TYPE_REQUEST:
        list_add(&hdr->ln, &queue->req_msgs);
        queue->state |= MSG_QS_REQ;
        queue->nr_msgs++;
        break;

    case PCRDR_MSG_TYPE_RESPONSE:
        list_add(&hdr->ln, &queue->res_msgs);
        queue->state |= MSG_QS_RES;
        queue->nr_msgs++;
        break;

    case PCRDR_MSG_TYPE_EVENT:
        queue->state |= MSG_QS_EVENT;
        break;

    default:
        list_add(&hdr->ln, &queue->void_msgs);
        queue->state |= MSG_QS_VOID;
        queue->nr_msgs++;
        break;
    }

    purc_rwlock_writer_unlock(&queue->lock);
    return 0;
}

static pcrdr_msg *
get_msg(struct pcinst_msg_queue *queue, struct list_head *msgs)
{
    if (list_empty(msgs)) {
        return NULL;
    }
    struct pcinst_msg_hdr *hdr = list_first_entry(&queue->void_msgs,
            struct pcinst_msg_hdr, ln);
    pcrdr_msg *msg = (pcrdr_msg *)hdr;
    list_del(&hdr->ln);
    if (list_empty(msgs)) {
        queue->state &= ~MSG_QS_RES;
    }
    return msg;
}

pcrdr_msg *
pcinst_msg_queue_get_msg(struct pcinst_msg_queue *queue)
{
    pcrdr_msg *msg = NULL;
    if (queue->state & MSG_QS_RES) {
        msg = get_msg(queue, &queue->res_msgs);
        if (msg) {
            goto done;
        }
    }

    if (queue->state & MSG_QS_REQ) {
        msg = get_msg(queue, &queue->req_msgs);
        if (msg) {
            goto done;
        }
    }

    if (queue->state & MSG_QS_EVENT) {
        msg = get_msg(queue, &queue->event_msgs);
        if (msg) {
            goto done;
        }
    }

    if (queue->state & MSG_QS_VOID) {
        msg = get_msg(queue, &queue->void_msgs);
        if (msg) {
            goto done;
        }
    }

done:
    return msg;
}

