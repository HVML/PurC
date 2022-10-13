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

#include <sys/time.h>

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

static ssize_t
grind_msg_list(struct list_head *msgs)
{
    ssize_t nr = 0;
    struct list_head *p, *n;
    list_for_each_safe(p, n, msgs) {
        struct pcinst_msg_hdr *hdr;
        hdr = list_entry(p, struct pcinst_msg_hdr, ln);
        list_del(p);
        pcrdr_release_message((pcrdr_msg *)hdr);
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

bool
is_event_match(pcrdr_msg *left, pcrdr_msg *right)
{
    if ((left->target == right->target) &&
            (left->targetValue == right->targetValue) &&
            (purc_variant_is_equal_to(left->eventName, right->eventName)) &&
            (purc_variant_is_equal_to(left->elementValue, right->elementValue))
            ) {
        return true;
    }
    return false;
}

static uint64_t
get_timestamp_us(void)
{
    struct timeval now;
    gettimeofday(&now, 0);
    return (uint64_t)now.tv_sec * 1000000 + now.tv_usec;
}

int
reduce_event(struct pcinst_msg_queue *queue, pcrdr_msg *msg, bool tail)
{
    struct pcinst_msg_hdr *hdr;
    struct list_head *p, *n;
    list_for_each_safe(p, n, &queue->event_msgs) {
        hdr = list_entry(p, struct pcinst_msg_hdr, ln);
        pcrdr_msg *orig = (pcrdr_msg*) hdr;
        if (is_event_match(orig, msg)) {
            if (msg->reduceOpt == PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE) {
                pcrdr_release_message(msg);
                return 0;
            }
            // OVERLAY : data
            if (orig->data) {
                purc_variant_unref(orig->data);
                orig->data = PURC_VARIANT_INVALID;
            }
            if (msg->data) {
                orig->data = msg->data;
                purc_variant_ref(orig->data);
            }
            pcrdr_release_message(msg);
            return 0;
        }
    }

    hdr = (struct pcinst_msg_hdr *)msg;
    /* keep timestamp */
    msg->resultValue = get_timestamp_us();
    if (tail) {
        list_add_tail(&hdr->ln, &queue->event_msgs);
    }
    else {
        list_add(&hdr->ln, &queue->event_msgs);
    }
    queue->state |= MSG_QS_EVENT;
    queue->nr_msgs++;

    return 0;
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
        if (msg->reduceOpt == PCRDR_MSG_EVENT_REDUCE_OPT_KEEP) {
            /* keep timestamp */
            msg->resultValue = get_timestamp_us();
            list_add_tail(&hdr->ln, &queue->event_msgs);
            queue->state |= MSG_QS_EVENT;
            queue->nr_msgs++;
        }
        else {
            reduce_event(queue, msg, true);
        }
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
        if (msg->reduceOpt == PCRDR_MSG_EVENT_REDUCE_OPT_KEEP) {
            list_add(&hdr->ln, &queue->event_msgs);
            queue->state |= MSG_QS_EVENT;
            queue->nr_msgs++;
        }
        else {
            reduce_event(queue, msg, false);
        }
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
    struct pcinst_msg_hdr *hdr = list_first_entry(msgs,
            struct pcinst_msg_hdr, ln);
    pcrdr_msg *msg = (pcrdr_msg *)hdr;
    list_del(&hdr->ln);
    queue->nr_msgs--;
    if (list_empty(msgs)) {
        queue->state &= ~MSG_QS_RES;
    }
    return msg;
}

pcrdr_msg *
pcinst_msg_queue_get_msg(struct pcinst_msg_queue *queue)
{
    purc_rwlock_writer_lock(&queue->lock);
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
    purc_rwlock_writer_unlock(&queue->lock);
    return msg;
}

pcrdr_msg *
pcinst_msg_queue_get_event_by_element(struct pcinst_msg_queue *queue,
        purc_variant_t request_id, purc_variant_t element_value,
        purc_variant_t event_name)
{
    pcrdr_msg *msg = NULL;
    purc_rwlock_writer_lock(&queue->lock);

    struct list_head *msgs = &queue->event_msgs;
    struct list_head *p, *n;
    list_for_each_safe(p, n, msgs) {
        struct pcinst_msg_hdr *hdr;
        hdr = list_entry(p, struct pcinst_msg_hdr, ln);
        pcrdr_msg *m = (pcrdr_msg*) hdr;
        if (purc_variant_is_equal_to(m->requestId, request_id) &&
                purc_variant_is_equal_to(m->elementValue, element_value) &&
                purc_variant_is_equal_to(m->eventName, event_name)) {
            msg = m;
            list_del(&hdr->ln);
            break;
        }
    }

    purc_rwlock_writer_unlock(&queue->lock);
    return msg;
}

int
purc_inst_post_event(purc_atom_t inst_to, pcrdr_msg *msg)
{
    if (!msg || msg->type != PCRDR_MSG_TYPE_EVENT) {
        return -1;
    }

    if (inst_to == PURC_EVENT_TARGET_SELF) {
        if (msg->target != PCRDR_MSG_TARGET_COROUTINE) {
            return 0;
        }

        struct pcintr_heap *heap = pcintr_get_heap();
        if (!heap) {
            return 0;
        }

        struct list_head *crtns;
        pcintr_coroutine_t p, q;
        if (PURC_EVENT_TARGET_BROADCAST != msg->targetValue) {
            crtns = &heap->crtns;
            list_for_each_entry_safe(p, q, crtns, ln) {
                pcintr_coroutine_t co = p;
                if (co->cid == msg->targetValue) {
                    return pcinst_msg_queue_append(co->mq, msg);
                }
            }

            crtns = &heap->stopped_crtns;
            list_for_each_entry_safe(p, q, crtns, ln) {
                pcintr_coroutine_t co = p;
                if (co->cid == msg->targetValue) {
                    return pcinst_msg_queue_append(co->mq, msg);
                }
            }
        }
        else {
            crtns = &heap->crtns;
            list_for_each_entry_safe(p, q, crtns, ln) {
                pcintr_coroutine_t co = p;
                pcrdr_msg *my_msg = pcrdr_clone_message(msg);
                my_msg->targetValue = co->cid;
                pcinst_msg_queue_append(co->mq, my_msg);
            }

            crtns = &heap->stopped_crtns;
            list_for_each_entry_safe(p, q, crtns, ln) {
                pcintr_coroutine_t co = p;
                pcrdr_msg *my_msg = pcrdr_clone_message(msg);
                my_msg->targetValue = co->cid;
                pcinst_msg_queue_append(co->mq, my_msg);
            }
            pcrdr_release_message(msg);
        }

        return 0;
    }

    int ret = 0;
    if (purc_inst_move_message(inst_to, msg) == 0) {
        pcrdr_release_message(msg);
        ret = -1;
    }
    return ret;
}

int
pcinst_broadcast_event(pcrdr_msg_event_reduce_opt reduce_op,
        purc_variant_t source_uri, purc_variant_t observed,
        const char *event_type, const char *event_sub_type,
        purc_variant_t data)
{
    if (!event_type) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    size_t n = strlen(event_type) + 1;
    if (event_sub_type) {
        n = n +  strlen(event_sub_type) + 2;
    }

    char *p = (char*)malloc(n);
    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }
    if (event_sub_type) {
        sprintf(p, "%s:%s", event_type, event_sub_type);
    }
    else {
        sprintf(p, "%s", event_type);
    }

    purc_variant_t event_name = purc_variant_make_string_reuse_buff(p,
            strlen(p), true);
    if (!event_name) {
        free(p);
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    pcrdr_msg *msg = pcinst_get_message();
    if (msg == NULL) {
        purc_variant_unref(event_name);
        return -1;
    }

    msg->type = PCRDR_MSG_TYPE_EVENT;
    msg->target = PCRDR_MSG_TARGET_COROUTINE;
    msg->targetValue = PURC_EVENT_TARGET_BROADCAST;
    msg->reduceOpt = reduce_op;

    if (source_uri) {
        msg->sourceURI = source_uri;
        purc_variant_ref(msg->sourceURI);
    }

    msg->elementType = PCRDR_MSG_ELEMENT_TYPE_VARIANT;
    msg->elementValue = observed;
    purc_variant_ref(msg->elementValue);

    msg->eventName = event_name;

    if (data) {
        msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
        msg->data = data;
        purc_variant_ref(msg->data);
    }

    return purc_inst_post_event(PURC_EVENT_TARGET_BROADCAST, msg);
}

size_t
pcinst_msg_queue_count(struct pcinst_msg_queue *queue)
{
    size_t nr = 0;
    purc_rwlock_writer_lock(&queue->lock);
    nr = queue->nr_msgs;
    purc_rwlock_writer_unlock(&queue->lock);
    return nr;
}

