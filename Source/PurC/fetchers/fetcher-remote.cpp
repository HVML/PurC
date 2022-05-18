/*
 * @file fetcher-remote.cpp
 * @author XueShuming
 * @date 2021/11/16
 * @brief The impl for fetcher remote.
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
 * but WITHOUT ANY WARRANTY
{
}
 without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "fetcher-internal.h"
#include "fetcher-process.h"

#if 0           /* { */
#endif          /* } */

#if ENABLE(REMOTE_FETCHER)

struct pcfetcher_remote {
    struct pcfetcher base;
    PcFetcherProcess* process;
    char* base_uri;
};

int pcfetcher_remote_term(struct pcfetcher* fetcher)
{
    struct pcfetcher_remote* remote = (struct pcfetcher_remote*)fetcher;
    delete remote->process;

    if (remote->base_uri) {
        free(remote->base_uri);
    }
    free(remote);

    return 0;
}

const char* pcfetcher_remote_set_base_url(struct pcfetcher* fetcher,
        const char* base_url)
{
    if (!fetcher) {
        return NULL;
    }

    struct pcfetcher_remote* remote = (struct pcfetcher_remote*)fetcher;
    if (remote->base_uri) {
        free(remote->base_uri);
    }

    if (base_url == NULL) {
        remote->base_uri = NULL;
        return NULL;
    }

    remote->base_uri = strdup(base_url);
    return remote->base_uri;
}

void pcfetcher_cookie_remote_set(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name,
        const char* content, time_t expire_time, bool secure)
{
    UNUSED_PARAM(fetcher);
    UNUSED_PARAM(domain);
    UNUSED_PARAM(path);
    UNUSED_PARAM(name);
    UNUSED_PARAM(content);
    UNUSED_PARAM(expire_time);
    UNUSED_PARAM(secure);
}

const char* pcfetcher_cookie_remote_get(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name,
        time_t *expire, bool *secure)
{
    UNUSED_PARAM(fetcher);
    UNUSED_PARAM(domain);
    UNUSED_PARAM(path);
    UNUSED_PARAM(name);
    UNUSED_PARAM(expire);
    UNUSED_PARAM(secure);
    return NULL;
}

const char* pcfetcher_cookie_remote_remove(struct pcfetcher* fetcher,
        const char* domain, const char* path, const char* name)
{
    UNUSED_PARAM(fetcher);
    UNUSED_PARAM(domain);
    UNUSED_PARAM(path);
    UNUSED_PARAM(name);
    return NULL;
}


purc_variant_t pcfetcher_remote_request_async(
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        pcfetcher_response_handler handler,
        void* ctxt)
{
    struct pcfetcher_remote* remote = (struct pcfetcher_remote*)fetcher;
    return remote->process->requestAsync(
            remote->base_uri,
            url, method, params, timeout, handler, ctxt);
}


purc_rwstream_t pcfetcher_remote_request_sync(
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header)
{
    struct pcfetcher_remote* remote = (struct pcfetcher_remote*)fetcher;
    return remote->process->requestSync(
            remote->base_uri,
            url, method, params, timeout, resp_header);
}

void pcfetcher_remote_cancel_async(struct pcfetcher* fetcher,
        purc_variant_t request)
{
    struct pcfetcher_remote* remote = (struct pcfetcher_remote*)fetcher;
    remote->process->cancelAsyncRequest(request);
}

int pcfetcher_remote_check_response(struct pcfetcher* fetcher,
        uint32_t timeout_ms)
{
    struct pcfetcher_remote* remote = (struct pcfetcher_remote*)fetcher;
    return remote->process->checkResponse(timeout_ms);
}


pcfetcher_req_t pcfetcher_remote_make_req(
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        pcfetcher_response_handler handler,
        void* ctxt)
{
    UNUSED_PARAM(fetcher);
    UNUSED_PARAM(url);
    UNUSED_PARAM(method);
    UNUSED_PARAM(params);
    UNUSED_PARAM(timeout);
    UNUSED_PARAM(handler);
    UNUSED_PARAM(ctxt);
    abort();
}

void pcfetcher_remote_cancel_req(
        struct pcfetcher* fetcher,
        pcfetcher_req_t req)
{
    UNUSED_PARAM(fetcher);
    UNUSED_PARAM(req);
    abort();
}



struct pcfetcher* pcfetcher_remote_init(size_t max_conns, size_t cache_quota)
{
    struct pcfetcher_remote* remote = (struct pcfetcher_remote*)calloc(1,
            sizeof(struct pcfetcher_remote));

    struct pcfetcher* fetcher = (struct pcfetcher*) remote;
    fetcher->max_conns = max_conns;
    fetcher->cache_quota = cache_quota;
    fetcher->init = pcfetcher_remote_init;
    fetcher->term = pcfetcher_remote_term;
    fetcher->set_base_url = pcfetcher_remote_set_base_url;
    fetcher->cookie_set = pcfetcher_cookie_remote_set;
    fetcher->cookie_get = pcfetcher_cookie_remote_get;
    fetcher->cookie_remove = pcfetcher_cookie_remote_remove;
    fetcher->request_async = pcfetcher_remote_request_async;
    fetcher->request_sync = pcfetcher_remote_request_sync;
    fetcher->cancel_async = pcfetcher_remote_cancel_async;
    fetcher->check_response = pcfetcher_remote_check_response;

    fetcher->make_req = pcfetcher_remote_make_req;
    fetcher->cancel_req = pcfetcher_remote_cancel_req;

    remote->process = new PcFetcherProcess(fetcher);
    remote->process->connect();
    remote->base_uri = NULL;

    return (struct pcfetcher*)remote;
}


#endif // ENABLE(REMOTE_FETCHER)
