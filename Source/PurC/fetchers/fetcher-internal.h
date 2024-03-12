/*
 * @file fetcher-internal.h
 * @author XueShuming
 * @date 2021/11/16
 * @brief The interfaces for fetcher internal.
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

#ifndef PURC_FETCHER_INTERNAL_H
#define PURC_FETCHER_INTERNAL_H

#include "private/fetcher.h"

#define PCFETCHER_INITIAL_PROGRESS  0.1

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct pcfetcher;

typedef struct pcfetcher* (*pcfetcher_init_fn)(size_t max_conns,
        size_t cache_quota);

typedef int (*pcfetcher_term_fn)(struct pcfetcher* fetcher);

typedef const char* (*pcfetcher_set_base_url_fn)(struct pcfetcher* fetcher,
        const char* base_url);

typedef void (*pcfetcher_cookie_set_fn)(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name,
        const char* content, time_t expire_time, bool secure);

typedef const char* (*pcfetcher_cookie_get_fn)(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name,
        time_t *expire, bool *secure);

typedef const char* (*pcfetcher_cookie_remove_fn)(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name);

typedef purc_variant_t (*pcfetcher_request_async_fn)(
        struct pcfetcher_session *session,
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        pcfetcher_response_handler handler,
        void* ctxt,
        pcfetcher_progress_tracker tracker,
        void* tracker_ctxt);

typedef purc_rwstream_t (*pcfetcher_request_sync_fn)(
        struct pcfetcher_session *session,
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header);

typedef void (*pcfetcher_cancel_async_fn)(struct pcfetcher* fetcher,
        purc_variant_t request);

typedef int (*pcfetcher_check_response_fn)(struct pcfetcher* fetcher,
        uint32_t timeout_ms);

struct pcfetcher {
    size_t max_conns;
    size_t cache_quota;

    pcfetcher_init_fn init;
    pcfetcher_term_fn term;
    pcfetcher_cookie_set_fn cookie_set;
    pcfetcher_cookie_get_fn cookie_get;
    pcfetcher_cookie_remove_fn cookie_remove;
    pcfetcher_request_async_fn request_async;
    pcfetcher_request_sync_fn request_sync;
    pcfetcher_cancel_async_fn cancel_async;
    pcfetcher_check_response_fn check_response;
};

struct pcfetcher_callback_info {
    struct pcfetcher_session *session;
    struct pcfetcher_resp_header header;
    purc_rwstream_t rws;
    purc_variant_t req_id;
    volatile bool dispatched;
    volatile bool cancelled;

    pcfetcher_response_handler handler;
    void *ctxt;

    pcfetcher_progress_tracker tracker;
    void *tracker_ctxt;
};

struct pcfetcher* pcfetcher_local_init(size_t max_conns, size_t cache_quota);

int pcfetcher_local_term(struct pcfetcher* fetcher);

void pcfetcher_cookie_local_set(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name,
        const char* content, time_t expire_time, bool secure);

const char* pcfetcher_cookie_local_get(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name,
        time_t *expire, bool *secure);

const char* pcfetcher_cookie_loccal_remove(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name);

purc_variant_t pcfetcher_local_request_async(
        struct pcfetcher_session *session,
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        pcfetcher_response_handler handler,
        void* ctxt,
        pcfetcher_progress_tracker tracker,
        void* tracker_ctxt);

purc_rwstream_t pcfetcher_local_request_sync(
        struct pcfetcher_session *session,
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header);

void pcfetcher_local_cancel_async(struct pcfetcher* fetcher,
        purc_variant_t request);

int pcfetcher_local_check_response(struct pcfetcher* fetcher,
        uint32_t timeout_ms);

#if ENABLE(REMOTE_FETCHER)

struct pcfetcher* pcfetcher_remote_init(size_t max_conns, size_t cache_quota);

int pcfetcher_remote_term(struct pcfetcher* fetcher);

const char* pcfetcher_remote_set_base_url(struct pcfetcher* fetcher,
        const char* base_url);

void pcfetcher_cookie_remote_set(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name,
        const char* content, time_t expire_time, bool secure);

const char* pcfetcher_cookie_remote_get(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name,
        time_t *expire, bool *secure);

const char* pcfetcher_cookie_remote_remove(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name);


purc_variant_t pcfetcher_remote_request_async(
        struct pcfetcher_session *session,
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        pcfetcher_response_handler handler,
        void* ctxt,
        pcfetcher_progress_tracker tracker,
        void* tracker_ctxt);

purc_rwstream_t pcfetcher_remote_request_sync(
        struct pcfetcher_session *session,
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        unsigned int timeout,
        struct pcfetcher_resp_header *resp_header);

void pcfetcher_remote_cancel_async(struct pcfetcher* fetcher,
        purc_variant_t request);

int pcfetcher_remote_check_response(struct pcfetcher* fetcher,
        uint32_t timeout_ms);

#endif // ENABLE(REMOTE_FETCHER)

struct pcfetcher_callback_info *pcfetcher_create_callback_info();
void pcfetcher_destroy_callback_info(struct pcfetcher_callback_info *info);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_FETCHER_INTERNAL_H */


